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
  const char* connection_name;
} Lfo;

#define LFO_NO_CONNECTION "none"

#define DRUMPAD_ROWS 4
#define DRUMPAD_COLS 16

struct Waveshaper;

typedef struct Drumpad {
  i32 pad[DRUMPAD_COLS][DRUMPAD_ROWS];
  void (*event[DRUMPAD_ROWS])(struct Waveshaper* w);
  void (*process[DRUMPAD_ROWS])(struct Audio_engine* audio, struct Instrument* ins, f32* buffer, size_t samples);
  size_t sample_index[DRUMPAD_ROWS];
  size_t index;
} Drumpad;

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
  f32 gain;
  Arena arena;
  char* text;
  i32 render;
  Lfo lfo;
  char* lfo_connection;
  Drumpad drumpad;
} Waveshaper;

void waveshaper_init(Instrument* ins);
void waveshaper_ui_new(Instrument* ins, Element* container);
void waveshaper_update(Instrument* ins, struct Mix* mix);
void waveshaper_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt);
void waveshaper_free(struct Instrument* ins);

#endif // _WAVE_SHAPER_H
