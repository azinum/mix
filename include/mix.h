// mix.h

#ifndef _MIX_H
#define _MIX_H

struct Mix;

#include <raylib.h>
// #include <raymath.h>

#ifdef TARGET_ANDROID
  #include "raymob.h"
  #define open(...) fd_open(__VA_ARGS__)
#endif

#include <time.h>
#include <math.h>
#include <fcntl.h>

#define COLOR_RGB(R, G, B) ((Color) { .r = R, .g = G, .b = B, .a = 255, })
#define COLOR(R, G, B, A)  ((Color) { .r = R, .g = G, .b = B, .a = A, })

#define OVERLAP(x0, y0, x1, y1, w, h) \
  (x0 >= x1 && x0 < x1 + w) && \
  (y0 >= y1 && y0 < y1 + h)

#define TIMER_START(...) \
  struct timespec _end = {0}; \
  struct timespec _start = {0}; \
  clock_gettime(CLOCK_REALTIME, &_start); \
  __VA_ARGS__

#define TIMER_END() (clock_gettime(CLOCK_REALTIME, &_end), ((((_end.tv_sec - _start.tv_sec) * 1000000000.0f) + _end.tv_nsec) - (_start.tv_nsec)) / 1000000000.0f)

#define FPS_MIN 10
#define FPS_MAX 10000
#define DT_MIN (1.0f / FPS_MAX)
#define DT_MAX (1.0f / FPS_MIN)
#ifndef DATA_PATH
  #define DATA_PATH "data"
#endif
#define CONFIG_PATH DATA_PATH "/init.lua"
#define BPM_DEFAULT 80
#define BPM_MIN 1
#define BPM_MAX 999
#define SUBTICKS 8

typedef struct Mix Mix;
typedef struct Audio_engine Audio_engine;
typedef struct Instrument Instrument;

#include "common.h"
#include "flags.h"
#include "platform.h"
#include "memory.h"
#include "thread.h"
#include "glob.h"
#include "hash.h"
#include "random.h"
#include "buffer.h"
#include "config.h"
#include "misc.h"
#include "lut.h"
#include "colors.h"
#include "log.h"
#include "arena.h"
#include "module.h"
#include "ui.h"
#include "settings.h"
#include "midi.h"
#include "midi_settings.h"
#include "keyboard.h"

// instruments
#include "instrument.h"

// effects/filters
#include "effect.h"
// #include "fx_clip_distortion.h"
// #include "fx_filter.h"
// #include "fx_delay.h"
// #include "fx_smooth.h"
// #include "fx_interpolator.h"
// #include "fx_reverb.h"

#include "instrument_picker.h"
#include "control_panel.h"
#include "effect_chain.h"
#include "effect_picker.h"
#include "audio.h"
#include "ui_audio.h"

#ifdef TEST_UI
  #include "test_ui.h"
#endif

#define MAX_MIDI_EVENTS 128

typedef struct Mix {
  Vector2 mouse;
  f32 fps;
  f32 dt;
  size_t tick;
  size_t timed_tick;
  f32 tick_delta;
  i32 bpm;
  f32 timer;
  i32 paused;

  Element* ins_container;
  Element* effect_chain;

  Midi_event midi_events[MAX_MIDI_EVENTS];
  size_t midi_event_count;
} Mix;

typedef struct Assets {
  Font font;
  Shader sdf;
} Assets;

extern Mix mix_state;
extern Assets assets;

i32 mix_main(i32 argc, char** argv);
Result mix_restart_audio_engine(void);
void mix_assets_load(void);
void mix_assets_unload(void);
void mix_send_midi_event(Midi_event event);
void mix_pause(void);
void mix_play(void);
void mix_stop(void);
void mix_set_bpm(i32 bpm);
void mix_reset_tick(void);
size_t mix_get_tick(void);
void mix_reload_ui(void);

#endif // _MIX_H
