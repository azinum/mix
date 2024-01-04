// audio_null.c

Result audio_new(Audio_engine* e) {
  (void)e;
  return Ok;
}

void audio_exit(Audio_engine* e) {
  (void)e;
}

#define AUDIO_NULL
