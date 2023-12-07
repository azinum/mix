// audio.h

#ifndef _AUDIO_H
#define _AUDIO_H

typedef struct Audio_engine {
  i32 sample_rate;
  i32 frames_per_buffer;
  i32 channel_count;
  f32* buffer;
  f32 dt;
  bool quit; // set to true to stop processing audio
  bool done; // done is set to true when audio processing has stopped

  Waveshaper waveshaper;
} Audio_engine;

extern Audio_engine audio_engine;

Audio_engine audio_engine_new(i32 sample_rate, i32 frames_per_buffer, i32 channel_count);
Result audio_engine_start(Audio_engine* e);
void audio_engine_exit(Audio_engine* e);
Result audio_engine_process(const void* in, void* out, i32 sample_count);

#endif // _AUDIO_H
