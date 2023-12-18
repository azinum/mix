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
  return (Audio_engine) {
    .sample_rate       = sample_rate,
    .frames_per_buffer = frames_per_buffer,
    .channel_count     = channel_count,
    .out_buffer        = memory_calloc(frames_per_buffer * channel_count, sizeof(f32)),
    .in_buffer         = memory_calloc(frames_per_buffer * channel_count, sizeof(f32)),
    .dt                = DT_MIN,
    .instrument        = instrument_new(INSTRUMENT_WAVE_SHAPER),
    .quit              = false,
    .done              = false,
    .restart           = false,
  };
}

Result audio_engine_start(Audio_engine* e) {
  return audio_new(e);
}

Result audio_engine_start_new(Audio_engine* e) {
  *e = audio_engine_new(SAMPLE_RATE, FRAMES_PER_BUFFER, CHANNEL_COUNT);
  instrument_init(&e->instrument, e);
  return audio_new(e);
}

void audio_engine_restart(void) {
  Audio_engine* audio = &audio_engine;
  audio->restart = true;
}

void audio_engine_exit(Audio_engine* audio) {
  audio->quit = true;
  u32 spin = 0;
  const u32 max_spin = 1000000;
  while (!audio->done && spin < max_spin) {
    spin_wait();
    spin += 1;
  }
  instrument_free(&audio->instrument);
  memory_free(audio->out_buffer);
  memory_free(audio->in_buffer);
  audio_exit(audio);
}

// TODO(lucas): handle variable sample_count, number of ready samples may not be predictable
Result audio_engine_process(const void* in, void* out, i32 sample_count) {
  TIMER_START();
  Mix* m = &mix;
  Audio_engine* audio = &audio_engine;
  if (audio->quit) {
    audio->done = true;
    return Error;
  }
  if (sample_count != audio->frames_per_buffer) {
    FRAMES_PER_BUFFER = sample_count;
    audio->restart = true;
    return Error;
  }

  f32* buffer = (f32*)out;
  const i32 frames_per_buffer = audio->frames_per_buffer;
  const i32 channel_count = audio->channel_count;

  // clear master audio buffer
  for (i32 i = 0; i < frames_per_buffer * channel_count; ++i) {
    audio->out_buffer[i] = 0;
  }

  if (in) {
    for (i32 i = 0; i < frames_per_buffer * channel_count; ++i) {
      audio->in_buffer[i] = ((f32*)in)[i];
    }
  }

  Instrument* ins = &audio->instrument;

  // process instruments and effects
  instrument_process(ins, m, audio, audio->dt);

  // sum all audio buffers
  for (i32 i = 0; i < frames_per_buffer * channel_count; ++i) {
    audio->out_buffer[i] += ins->buffer[i];
  }

  // write to output buffer
  for (i32 i = 0; i < frames_per_buffer * channel_count; ++i) {
    buffer[i] = audio->out_buffer[i];
  }
  audio->dt = TIMER_END();
  return Ok;
}
