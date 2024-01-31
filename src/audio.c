// audio.c

Audio_engine audio_engine = {0};
extern Result audio_new(Audio_engine* e);
extern void audio_exit(Audio_engine* e);

#ifdef USE_MINIAUDIO
  #include "audio_miniaudio.c"
#elif USE_PORTAUDIO
  #include "audio_pa.c"
#else
  #include "audio_null.c"
#endif

Audio_engine audio_engine_new(i32 sample_rate, i32 frames_per_buffer, i32 channel_count) {
  MEMORY_TAG("audio engine output buffer");
  f32* out_buffer = memory_calloc(frames_per_buffer * channel_count, sizeof(f32));
  MEMORY_TAG("audio engine input buffer");
  f32* in_buffer = memory_calloc(frames_per_buffer * channel_count, sizeof(f32));

  return (Audio_engine) {
    .sample_rate       = sample_rate,
    .frames_per_buffer = frames_per_buffer,
    .channel_count     = channel_count,
    .out_buffer        = out_buffer,
    .in_buffer         = in_buffer,
    .dt                = DT_MIN,
    .instrument        = instrument_new(INSTRUMENT_WAVE_SHAPER),
    .quit              = false,
    .done              = false,
    .restart           = false,
  };
}

Result audio_engine_start(Audio_engine* audio) {
  return audio_new(audio);
}

Result audio_engine_start_new(Audio_engine* audio) {
  *audio = audio_engine_new(SAMPLE_RATE, FRAMES_PER_BUFFER, CHANNEL_COUNT);
  instrument_init(&audio->instrument, audio);
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
  memory_free(audio->out_buffer);
  memory_free(audio->in_buffer);
  audio_exit(audio);
  dt = TIMER_END();
  log_print(STDOUT_FILENO, LOG_TAG_INFO, "shut down audio engine in %g ms\n", 1000 * dt);
}

Result audio_engine_process(const void* in, void* out, i32 frames) {
  TIMER_START();
  Mix* m = &mix;
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

  const f32 process_dt = frames / (f32)audio->sample_rate;

  f32* buffer = (f32*)out;
  const i32 frames_per_buffer = audio->frames_per_buffer;
  const i32 channel_count = audio->channel_count;
  const i32 sample_count = frames_per_buffer * channel_count;

  // clear master audio buffer
  for (i32 i = 0; i < sample_count; ++i) {
    audio->out_buffer[i] = 0;
  }

  if (!m->paused) {
    if (in) {
      for (i32 i = 0; i < sample_count; ++i) {
        audio->in_buffer[i] = ((f32*)in)[i];
      }
    }

    Instrument* ins = &audio->instrument;

    if (ins->initialized) {
      // process instruments and effects
      instrument_process(ins, m, audio, process_dt);

      // sum all audio buffers
      for (i32 i = 0; i < sample_count; ++i) {
        audio->out_buffer[i] += ins->buffer[i];
      }
    }
  }

  // write to output buffer
  for (i32 i = 0; i < sample_count; ++i) {
    buffer[i] = audio->out_buffer[i];
  }
  audio->dt = TIMER_END();
  // stb_printf("dt: %g, max dt: %g\n", 1000 * audio->dt, 1000 * (frames / (f32)audio->sample_rate));
  return Ok;
}
