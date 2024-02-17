// audio.c

Audio_engine audio_engine = {0};
extern Result audio_new(Audio_engine* e);
extern void audio_exit(Audio_engine* e);

#if defined(USE_MINIAUDIO)
  #include "audio_miniaudio.c"
#elif defined(USE_PORTAUDIO)
  #include "audio_pa.c"
#else
  #include "audio_null.c"
#endif

Audio_engine audio_engine_new(i32 sample_rate, i32 frames_per_buffer, i32 channel_count) {
  frames_per_buffer = ALIGN(frames_per_buffer, 64);
  MEMORY_TAG("audio engine output buffer");
  f32* out_buffer = memory_calloc(frames_per_buffer * channel_count, sizeof(f32));
  MEMORY_TAG("audio engine input buffer");
  f32* in_buffer = memory_calloc(frames_per_buffer * channel_count, sizeof(f32));

  i16* record_buffer = NULL;
  size_t record_buffer_size = 0;
#ifndef NO_RECORD_BUFFER
  record_buffer_size = ALIGN(RECORD_BUFFER_LENGTH_SECS * channel_count * sample_rate, frames_per_buffer);
  MEMORY_TAG("audio engine record buffer");
  record_buffer = memory_alloc(sizeof(i16) * record_buffer_size);
  if (!record_buffer) {
    log_print(STDERR_FILENO, LOG_TAG_ERROR, "failed to allocate record buffer\n");
    record_buffer_size = 0;
  }
#endif

  return (Audio_engine) {
    .sample_rate          = sample_rate,
    .frames_per_buffer    = frames_per_buffer,
    .channel_count        = channel_count,
    .out_buffer           = out_buffer,
    .in_buffer            = in_buffer,
    .dt                   = DT_MIN,
    .instrument           = {0},
    .quit                 = false,
    .done                 = false,
    .restart              = false,
    .record_buffer        = record_buffer,
    .record_buffer_size   = record_buffer_size,
    .record_buffer_index  = 0,
  };
}

Result audio_engine_start(Audio_engine* audio) {
  return audio_new(audio);
}

Result audio_engine_start_new(Audio_engine* audio) {
  *audio = audio_engine_new(SAMPLE_RATE, FRAMES_PER_BUFFER, CHANNEL_COUNT);
  return audio_new(audio);
}

Result audio_engine_detach_instrument(void) {
  Audio_engine* audio = &audio_engine;
  instrument_destroy(&audio->instrument);
  return Ok;
}

Instrument* audio_engine_attach_instrument(Instrument* ins) {
  Audio_engine* audio = &audio_engine;
  if (audio->instrument.initialized) {
    audio_engine_detach_instrument();
  }
  instrument_init(ins, audio);
  audio->instrument = *ins;
  return &audio->instrument;
}

Effect* audio_engine_attach_effect(Effect* effect) {
  Audio_engine* audio = &audio_engine;
  if (audio->effect_count < MAX_EFFECTS) {
    instrument_init(effect, audio);
    audio->effect_chain[audio->effect_count] = *effect;
    return &audio->effect_chain[audio->effect_count++];
  }
  return NULL;
}

void audio_engine_clear_effects(void) {
  Audio_engine* audio = &audio_engine;
  for (size_t i = 0; i < audio->effect_count; ++i) {
    Effect* effect = &audio->effect_chain[i];
    instrument_destroy(effect);
  }
  audio->effect_count = 0;
}

void audio_engine_restart(void) {
  Audio_engine* audio = &audio_engine;
  audio->restart = true;
}

void audio_engine_exit(Audio_engine* audio) {
  TIMER_START();
  f32 dt = 0;

#ifndef AUDIO_NULL
  const f32 wait_time_max = 5.0f; // seconds
  audio->quit = true;
  while (!audio->done && dt < wait_time_max) {
    spin_wait();
    dt = TIMER_END();
  }
#endif
  instrument_destroy(&audio->instrument);
  audio_engine_clear_effects();
  memory_free(audio->out_buffer);
  memory_free(audio->in_buffer);
#ifndef NO_RECORD_BUFFER
  Wave wave = {
    .frameCount = audio->record_buffer_size / CHANNEL_COUNT,
    .sampleRate = SAMPLE_RATE,
    .sampleSize = 8 * sizeof(i16),
    .channels   = CHANNEL_COUNT,
    .data = audio->record_buffer,
  };
  ExportWave(wave, "record.wav");
#endif
  memory_free(audio->record_buffer);
  audio_exit(audio);
  dt = TIMER_END();
  log_print(STDOUT_FILENO, LOG_TAG_INFO, "shut down audio engine in %g ms\n", 1000 * dt);
}

Result audio_engine_process(const void* in, void* out, i32 frames) {
  TIMER_START();
  Mix* mix = &mix_state;
  Audio_engine* audio = &audio_engine;
  if (audio->quit) {
    audio->done = true;
    return Error;
  }
  if (frames != audio->frames_per_buffer) {
    FRAMES_PER_BUFFER = frames;
    audio->restart = true;
    return Error;
  }

  f32* buffer = (f32*)out;
  const i32 frames_per_buffer = audio->frames_per_buffer;
  const i32 channel_count = audio->channel_count;
  const i32 sample_count = frames_per_buffer * channel_count;
  const f32 process_dt = sample_count / (f32)audio->sample_rate;

  // clear master audio buffer
  for (i32 i = 0; i < sample_count; ++i) {
    audio->out_buffer[i] = 0;
  }

  if (!mix->paused) {
    if (in) {
      for (i32 i = 0; i < sample_count; ++i) {
        audio->in_buffer[i] = ((f32*)in)[i];
      }
    }

    Instrument* ins = &audio->instrument;

    if (ins->initialized) {
      // process instruments and effects
      instrument_process(ins, mix, audio, process_dt);

      // check to see if all effects are ready
      bool effects_ready = true;
      for (size_t i = 0; i < audio->effect_count; ++i) {
        Effect* effect = &audio->effect_chain[i];
        if (!effect->initialized) {
          effects_ready = false;
          break;
        }
      }
      if (effects_ready) {
        Effect* last = NULL;
        for (size_t i = 0; i < audio->effect_count; ++i) {
          Effect* effect = &audio->effect_chain[i];
          if (i == 0) {
            memcpy(effect->out_buffer, ins->out_buffer, ins->samples * sizeof(f32));
          }
          last = effect;
          instrument_process(effect, mix, audio, process_dt);
          if (i + 1 < audio->effect_count) {
            Effect* next = &audio->effect_chain[i + 1];
            memcpy(next->out_buffer, effect->out_buffer, ins->samples * sizeof(f32));
          }
        }
        if (last) {
          memcpy(ins->out_buffer, last->out_buffer, ins->samples * sizeof(f32));
        }
      }
      // sum all audio buffers (only one instrument for now)
      for (i32 i = 0; i < sample_count; ++i) {
        audio->out_buffer[i] += ins->out_buffer[i];
      }
    }
  }

  // write to output buffer
  for (i32 i = 0; i < sample_count; ++i) {
    buffer[i] = audio->out_buffer[i];
  }
  // write to record buffer
#ifndef NO_RECORD_BUFFER
  if (audio->recording) {
    for (i32 i = 0; i < sample_count; ++i) {
      size_t index = audio->record_buffer_index;
      audio->record_buffer[index] = (i16)(audio->out_buffer[i] * INT16_MAX);
      audio->record_buffer_index = (audio->record_buffer_index + 1) % audio->record_buffer_size;
    }
  }
#endif
  audio->dt = TIMER_END();
  return Ok;
}
