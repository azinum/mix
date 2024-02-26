// audio_null.c

Result audio_new(Audio_engine* audio) {
  (void)audio;
  return Ok;
}

void audio_exit(Audio_engine* audio) {
  (void)audio;
}

#define AUDIO_NULL
