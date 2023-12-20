// wave_shaper.h

#ifndef _WAVE_SHAPER_H
#define _WAVE_SHAPER_H

typedef struct Lfo {
  f32* lfo_target;
  f32 lfo;
  f32 amplitude;
  f32 hz;
  f32 offset;
  size_t tick;
  i32 additive; // multiplicative is default
  const char* connection_name;
} Lfo;

#define LFO_NO_CONNECTION "none"

typedef struct Waveshaper {
  ssize_t tick;
  f32 volume_target;
  f32 freq;
  f32 freq_target;
  f32 freq_mod;
  f32 freq_mod_target;
  f32 interp_speed;
  i32 freeze;
  i32 mute;
  i32 speed;
  i32 flipflop;
  i32 distortion;
  Arena arena;
  char* text;
  i32 render;
  Lfo lfo;
  char* lfo_connection;
} Waveshaper;

void waveshaper_init(Instrument* ins);
void waveshaper_ui_new(Instrument* ins, Element* container);
void waveshaper_update(Instrument* ins, struct Mix* mix);
void waveshaper_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt);
void waveshaper_free(struct Instrument* ins);

#endif // _WAVE_SHAPER_H
