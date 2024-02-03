// mix.h

#ifndef _MIX_H
#define _MIX_H

struct Mix;

#include <raylib.h>

#ifdef TARGET_ANDROID
  #include "raymob.h"
#endif

#include <time.h>
#include <math.h>
#include <fcntl.h>

#define COLOR_RGB(R, G, B) ((Color) { .r = R, .g = G, .b = B, .a = 255, })
#define COLOR(R, G, B, A)  ((Color) { .r = R, .g = G, .b = B, .a = A, })
#define MIN(x, y) (x < y ? x : y)
#define MAX(x, y) (x > y ? x : y)
#define CLAMP(x, x_min, x_max) MIN(MAX(x_min, x), x_max)

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
#define DATA_PATH "data"
#define CONFIG_PATH DATA_PATH "/init.lua"
#define BPM 80
#define SUBTICKS 8

#include "common.h"
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
#include "instrument.h"
#include "wave_shaper.h"
#include "dummy.h"
#include "instrument_picker.h"
#include "control_panel.h"
#include "audio.h"

#ifdef TEST_UI
  #include "test_ui.h"
#endif

typedef struct Mix {
  Vector2 mouse;
  f32 fps;
  f32 dt;
  size_t tick;
  size_t timed_tick;
  i32 bpm;
  f32 timer;
  f32 timer_start;
  i32 paused;

  Element* ins_container;
} Mix;

typedef struct Assets {
  Font font;
} Assets;

extern Mix mix;
extern Assets assets;

i32 mix_main(i32 argc, char** argv);
Result mix_restart_audio_engine(void);

#endif // _MIX_H
