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
  Audio_engine* e = &audio_engine;
  f32* buffer = (f32*)out;
  static size_t tick = 0;
  if (sample_count < 0) {
    sample_count = e->frames_per_buffer;
  }
  for (i32 i = 0; i < sample_count * e->channel_count; ++i) {
    *buffer++ = 0.1f * sine[tick % LENGTH(sine)];
    tick += 1;
  }
  return Ok;
}
