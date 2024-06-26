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

#ifdef TARGET_ANDROID
  #define DRUMPAD_ROWS 5
  #define DRUMPAD_COLS 16
  #define MOD_TABLE_LENGTH 8
#else
  #define DRUMPAD_ROWS 5
  #define DRUMPAD_COLS 16
  #define MOD_TABLE_LENGTH 16
#endif

void waveshaper_init(Instrument* ins, struct Mix* mix);
void waveshaper_ui_new(Instrument* ins, Element* container);
void waveshaper_update(Instrument* ins, struct Mix* mix);
void waveshaper_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt);
void waveshaper_noteon(struct Instrument* ins, u8 note, f32 velocity);
void waveshaper_noteoff(struct Instrument* ins, u8 note);
void waveshaper_destroy(struct Instrument* ins);

#endif // _WAVE_SHAPER_H
