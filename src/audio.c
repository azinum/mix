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

Audio_engine audio_engine_new(i32 sample_rate, i32 frames_per_buffer) {
  return (Audio_engine) {
    .sample_rate = sample_rate,
    .frames_per_buffer = frames_per_buffer,
    .channel_count = CHANNEL_COUNT,
    .buffer = memory_calloc(frames_per_buffer * CHANNEL_COUNT, sizeof(f32)),
    .dt = DT_MIN,
  };
}

Result audio_engine_start(Audio_engine* e) {
  return audio_new(e);
}

void audio_engine_exit(Audio_engine* e) {
  audio_exit(e);
}

Result audio_engine_process(const void* in, void* out, i32 sample_count) {
  (void)in;
  TIMER_START();

  Mix* m = &mix;
  Audio_engine* e = &audio_engine;

  f32* buffer = (f32*)out;
  const i32 frames_per_buffer = m->audio->frames_per_buffer;
  const i32 channel_count = m->audio->channel_count;

  // clear master audio buffer
  for (i32 i = 0; i < frames_per_buffer * channel_count; ++i) {
    e->buffer[i] = 0;
  }

  // process instruments and effects
  waveshaper_process(m, &m->waveshaper, e->dt);

  // sum all audio buffers
  for (i32 i = 0; i < frames_per_buffer * channel_count; ++i) {
    e->buffer[i] += m->waveshaper.buffer[i];
  }

  // write to output buffer
  for (i32 i = 0; i < frames_per_buffer * channel_count; ++i) {
    buffer[i] = e->buffer[i];
  }
  e->dt = TIMER_END();
  return Ok;
}
