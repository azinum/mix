// wave_shaper.h

#ifndef _WAVE_SHAPER_H
#define _WAVE_SHAPER_H

typedef struct Waveshaper {
  f32* buffer;
  size_t size;
  size_t tick;
  f32 freq;
  f32 freq_target;
  f32 latency; // overall latency
  f32 audio_latency; // audio processing latency
  f32 lfo;
  f32 lfo_target;
  f32 volume;
  i32 reshape;
  i32 mute;
  Arena arena;
  char* text;
  i32 render;
} Waveshaper;

Waveshaper waveshaper_new(size_t size);
Element waveshaper_ui_new(Waveshaper* w);
void waveshaper_update(struct Mix* m, Waveshaper* w);
void waveshaper_process(struct Mix* m, Waveshaper* w, f32 dt);
void waveshaper_free(Waveshaper* w);

#endif // _WAVE_SHAPER_H
