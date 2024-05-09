// audio.c

Audio_engine audio_engine = {0};
extern Result audio_new(Audio_engine* e);
extern void audio_exit(Audio_engine* e);

// TODO(lucas): change audio engine `backends` at runtime (shared library loading)
#if defined(USE_MINIAUDIO)
  #include "audio_miniaudio.c"
#elif defined(USE_PORTAUDIO)
  #include "audio_pa.c"
#else
  // TODO(lucas): null audio driver should process audio
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
  if (record_buffer_size != 0) {
    MEMORY_TAG("audio engine record buffer");
    record_buffer = memory_alloc(sizeof(i16) * record_buffer_size);
    if (!record_buffer) {
      log_print(STDERR_FILENO, LOG_TAG_ERROR, "failed to allocate record buffer\n");
      record_buffer_size = 0;
    }
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
    .db                   = 0,
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

Instrument* audio_engine_attach_instrument(Instrument* ins, Mix* mix) {
  Audio_engine* audio = &audio_engine;
  if (audio->instrument.initialized) {
    audio_engine_detach_instrument();
  }
  instrument_init(ins, audio, mix);
  audio->instrument = *ins;
  return &audio->instrument;
}

Effect* audio_engine_attach_effect(Effect* effect, Mix* mix) {
  Audio_engine* audio = &audio_engine;
  if (audio->effect_count < MAX_EFFECTS) {
    instrument_init(effect, audio, mix);
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
  if (audio->record_buffer) {
    Wave wave = {
      .frameCount = audio->record_buffer_index / CHANNEL_COUNT,
      .sampleRate = SAMPLE_RATE,
      .sampleSize = 8 * sizeof(i16),
      .channels   = CHANNEL_COUNT,
      .data       = audio->record_buffer,
    };
    if (wave.frameCount > 0) {
      time_t current_time = time(0);
      struct tm t = *localtime(&current_time);
      char record_path[MAX_PATH_LENGTH] = {0};
      snprintf(record_path, sizeof(record_path), "record-%d-%02d-%02d-%02d-%02d-%02d.wav", 1900 + t.tm_year, 1 + t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
      ExportWave(wave, record_path);
    }
  }
#endif
  memory_free(audio->record_buffer);
  audio_exit(audio);
  dt = TIMER_END();
  log_print(STDOUT_FILENO, LOG_TAG_INFO, "shut down audio engine in %g ms\n", 1000 * dt);
}

// TODO(lucas): queue files to be loaded in seperate thread(s)
Audio_source audio_load_audio(const char* path) {
  Audio_source source = {
    .buffer = NULL,
    .samples = 0,
    .channel_count = 0,
    .ready = false,
    .internal = false,
    .drawable = false,
    .cursor = 0,
    .mutex = ticket_mutex_new(),
  };
  Buffer buffer = buffer_new_from_file(path);
  if (!buffer.data) {
    log_print(STDERR_FILENO, LOG_TAG_ERROR, "failed to load audio file `%s`, because file does not exist or you do not have permissions to read it\n", path);
    return source;
  }
  char* ext = file_extension(path);
  Wave wave = LoadWaveFromMemory(ext, buffer.data, buffer.count);
  buffer_free(&buffer);
  if (wave.data) {
    if (wave.frameCount > 0) {
      size_t samples = wave.frameCount * wave.channels;
      u32 channel_count = (u32)wave.channels;
      f32* audio_buffer = memory_alloc(sizeof(f32) * samples);
      if (audio_buffer) {
        switch (wave.sampleSize) {
          case 8: {
            i8* data = (i8*)wave.data;
            for (size_t i = 0; i < samples; ++i) {
              audio_buffer[i] = data[i] / (f32)INT8_MAX;
            }
            break;
          }
          case 16: {
            i16* data = (i16*)wave.data;
            for (size_t i = 0; i < samples; ++i) {
              audio_buffer[i] = data[i] / (f32)INT16_MAX;
            }
            break;
          }
          case 32: {
            f32* data = (f32*)wave.data;
            for (size_t i = 0; i < samples; ++i) {
              audio_buffer[i] = data[i];
            }
            break;
          }
          default: {
            log_print(STDOUT_FILENO, LOG_TAG_ERROR, "audio file `%s` with %u-bit sample size, can not be loaded\n", path, wave.sampleSize);
            memory_free(audio_buffer);
            goto done;
          }
        }
        source.buffer = audio_buffer;
        source.samples = samples;
        source.channel_count = channel_count;
        source.ready = true;
        log_print(STDOUT_FILENO, LOG_TAG_SUCCESS, "loaded audio file `%s` (%u-bit, %zu samples, %u channels)\n", path, wave.sampleSize, samples, channel_count);
      }
      else {
        log_print(STDERR_FILENO, LOG_TAG_ERROR, "failed to allocate memory for audio source (from file `%s`)\n", path);
      }
    }
done:
    UnloadWave(wave);
  }
  else {
    log_print(STDERR_FILENO, LOG_TAG_ERROR, "failed to load audio file `%s`\n", path);
  }
  return source;
}

void audio_unload_audio(Audio_source* source) {
  ASSERT(source != NULL);
  if (!source->buffer) {
    return;
  }
  if (!source->samples) {
    return;
  }
  if (source->internal) {
    return;
  }
  ticket_mutex_begin(&source->mutex);
  memory_free(source->buffer);
  source->buffer = NULL;
  source->samples = 0;
  source->channel_count = 0;
  source->ready = false;
  source->drawable = false;
  source->cursor = 0;
  ticket_mutex_end(&source->mutex);
}

void audio_copy_split(const f32* input, f32* left_output, f32* right_output, const size_t samples) {
  ASSERT(input && left_output && right_output && !(samples % 2));
  size_t index = 0;
  for (size_t i = 0; i < samples; i += 2, index += 1) {
    left_output[index]  = input[i + 0];
    right_output[index] = input[i + 1];
  }
}

Audio_source audio_source_copy_into_new(const f32* input, const size_t samples, const u32 channel_count) {
  Audio_source source = (Audio_source) {
    .buffer = memory_alloc(sizeof(f32) * samples),
    .samples = samples,
    .channel_count = channel_count,
    .ready = true,
    .internal = false,
    .drawable = true,
    .cursor = 0,
    .mutex = ticket_mutex_new()
  };
  if (!source.buffer) {
    source.samples = 0;
    source.ready = false;
    return source;
  }
  memcpy(source.buffer, input, samples * sizeof(f32));
  return source;
}

Audio_source audio_source_new_from_i16_buffer(const i16* input, const size_t samples, const u32 channel_count) {
  Audio_source source = (Audio_source) {
    .buffer = memory_alloc(sizeof(f32) * samples),
    .samples = samples,
    .channel_count = channel_count,
    .ready = true,
    .internal = false,
    .drawable = true,
    .cursor = 0,
    .mutex = ticket_mutex_new()
  };
  if (!source.buffer) {
    source.samples = 0;
    source.ready = false;
    return source;
  }
  for (size_t i = 0; i < samples; ++i) {
    source.buffer[i] = input[i] / (f32)INT16_MAX;
  }
  return source;
}

void audio_source_move(Audio_source* dest, Audio_source* source) {
  ASSERT(dest != NULL && source != NULL);
  Ticket mutex = dest->mutex; // keep the old ticket
  bool drawable = dest->drawable;
  *dest = *source;
  dest->mutex = mutex;
  dest->drawable = drawable;
  dest->cursor = 0;
}

Audio_source audio_source_empty(void) {
  Audio_source source = (Audio_source) {
    .buffer = NULL,
    .samples = 0,
    .channel_count = 0,
    .ready = true,
    .internal = true,
    .drawable = true,
    .cursor = 0,
    .mutex = ticket_mutex_new()
  };
  return source;
}

f32 audio_calc_rms(f32* buffer, size_t size) {
  ASSERT(buffer != NULL);

  f32 db = 0.0f;
  for (size_t i = 0; i < size; ++i) {
    f32 sample = buffer[i];
    db += sample * sample;
  }
  return sqrtf(db / (f32)size);
}

f32 audio_calc_rms_clamp(f32* buffer, size_t size) {
  return CLAMP(audio_calc_rms(buffer, size), 0, 1);
}

Result audio_engine_process(const void* in, void* out, i32 frames) {
  TIMER_START();
  // TODO(lucas): measure latency of instruments and effects/plugins for
  // limiting cpu usage so that there will be no audio glitches/lag i.e. disable
  // or limit the amount of instruments/effects and/or use lower quality audio processing

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
    Instrument* ins = &audio->instrument;

    if (in && ins->in_buffer) {
      for (i32 i = 0; i < sample_count; ++i) {
        audio->in_buffer[i] = ((f32*)in)[i];
        ins->in_buffer[i]   = ((f32*)in)[i];
      }
    }


    if (ins->initialized && !ins->blocking) {
      // process instruments and effects
      instrument_process(ins, mix, audio, process_dt);

      ticket_mutex_begin(&ins->blocking_mutex);

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
      ASSERT(ins->out_buffer != NULL);
      for (i32 i = 0; i < sample_count; ++i) {
        audio->out_buffer[i] += ins->out_buffer[i];
      }
      ticket_mutex_end(&ins->blocking_mutex);
    }
  }

  f32 db = 0.0f;
  // write to output buffer and calculate RMS (root mean square)
  for (i32 i = 0; i < sample_count; ++i) {
    f32 sample = audio->out_buffer[i];
    buffer[i] = CLAMP(sample, -1, 1);
    db += sample * sample;
  }
  audio->db = sqrtf(db / (f32)sample_count);

  // write to record buffer
#ifndef NO_RECORD_BUFFER
  if (audio->recording && audio->record_buffer) {
    for (i32 i = 0; i < sample_count; ++i) {
      size_t index = audio->record_buffer_index;
      audio->record_buffer[index] = (i16)(audio->out_buffer[i] * INT16_MAX);
      audio->record_buffer_index += 1;
      if (audio->record_buffer_index >= audio->record_buffer_size) {
        audio->recording = false;
        break;
      }
    }
  }
#endif
  audio->dt = TIMER_END();
  return Ok;
}
