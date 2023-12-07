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
    .sample_rate = sample_rate,
    .frames_per_buffer = frames_per_buffer,
    .channel_count = channel_count,
    .buffer = memory_calloc(frames_per_buffer * channel_count, sizeof(f32)),
    .dt = DT_MIN,
    .waveshaper = waveshaper_new(frames_per_buffer * channel_count),
    .quit = false,
    .done = false,
  };
}

Result audio_engine_start(Audio_engine* e) {
  return audio_new(e);
}

void audio_engine_exit(Audio_engine* e) {
  e->quit = true;
  u32 spin = 0;
  const u32 max_spin = 1000000;
  const u32 spin_warn = max_spin / 2;
  TIMER_START();
  while (!e->done && spin < max_spin) {
    spin_wait();
    spin += 1;
  }
  f32 dt = TIMER_END();
  Log_tag log_tag = LOG_TAG_INFO;
  if (spin > spin_warn) {
    log_tag = LOG_TAG_WARN;
  }
  log_print(STDOUT_FILENO, log_tag, "%s: waited %g ms (%u iterations)\n", __FUNCTION_NAME__, 1000 * dt, spin);
  waveshaper_free(&e->waveshaper);
  audio_exit(e);
}

// TODO(lucas): handle variable sample_count, number of ready samples may not be predictable
Result audio_engine_process(const void* in, void* out, i32 sample_count) {
  (void)in;
  (void)sample_count;
  TIMER_START();

  Mix* m = &mix;
  Audio_engine* e = &audio_engine;
  if (e->quit) {
    e->done = true;
    return Error;
  }

  f32* buffer = (f32*)out;
  const i32 frames_per_buffer = e->frames_per_buffer;
  const i32 channel_count = e->channel_count;

  // clear master audio buffer
  for (i32 i = 0; i < frames_per_buffer * channel_count; ++i) {
    e->buffer[i] = 0;
  }

  // process instruments and effects
  waveshaper_process(m, &e->waveshaper, e->dt);

  // sum all audio buffers
  for (i32 i = 0; i < frames_per_buffer * channel_count; ++i) {
    e->buffer[i] += e->waveshaper.buffer[i];
  }

  // write to output buffer
  for (i32 i = 0; i < frames_per_buffer * channel_count; ++i) {
    buffer[i] = e->buffer[i];
  }
  e->dt = TIMER_END();
  return Ok;
}
