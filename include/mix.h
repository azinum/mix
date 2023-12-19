// mix.h

#ifndef _MIX_H
#define _MIX_H

struct Mix;

#ifdef INLINE_RAYLIB
  #define PLATFORM_DESKTOP
  #include "../src/ext/raylib/raylib.h"
  #include "../src/ext/raylib/utils.h"
  #include "../src/ext/raylib/utils.c"
  #undef MIN
  #include "../src/ext/raylib/rtextures.c"
  #undef COLOR_EQUAL
  #include "../src/ext/raylib/rtext.c"
  #include "../src/ext/raylib/rshapes.c"
  #undef MIN
  #include "../src/ext/raylib/rcore.c"
  #undef MIN
  #undef MAX
#else
  #include <raylib.h>
#endif

#include <sys/time.h>
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
  struct timeval _end = {0}; \
  struct timeval _start = {0}; \
  gettimeofday(&_start, NULL); \
  __VA_ARGS__

#define TIMER_END(...) (gettimeofday(&_end, NULL), ((((_end.tv_sec - _start.tv_sec) * 1000000.0f) + _end.tv_usec) - (_start.tv_usec)) / 1000000.0f)

#define FPS_MIN 10
#define FPS_MAX 10000
#define DT_MIN (1.0f / FPS_MAX)
#define DT_MAX (1.0f / FPS_MIN)

#include "common.h"
#include "thread.h"
#include "hash.h"
#include "random.h"
#include "buffer.h"
#include "config.h"
#include "misc.h"
#include "lut.h"
#include "log.h"
#include "memory.h"
#include "arena.h"
#include "module.h"
#include "ui.h"
#include "instrument.h"
#include "wave_shaper.h"
#include "audio.h"

typedef struct Mix {
  Vector2 mouse;
  f32 fps;
  f32 dt;
  size_t tick;
} Mix;

typedef struct Assets {
  Font font;
} Assets;

extern Mix mix;
extern Assets assets;

i32 mix_main(i32 argc, char** argv);
Result mix_restart_audio_engine(void);

#endif // _MIX_H
