// mix.h

#ifndef _MIX_H
#define _MIX_H

struct Mix;

#include "buffer.h"
#include "config.h"
#include "misc.h"
#include "lut.h"
#include "log.h"
#include "memory.h"
#include "entity.h"
#include "module.h"
#include "audio.h"
#include "wave_shaper.h"

#include <raylib.h>
#include <sys/time.h>
#include <math.h>
#include <fcntl.h>

#define COLOR_RGB(R, G, B) ((Color) { .r = R, .g = G, .b = B, .a = 255, })
#define COLOR(R, G, B, A)  ((Color) { .r = R, .g = G, .b = B, .a = A, })

#define OVERLAP(x0, y0, x1, y1, w, h) \
  (x0 >= x1 && x0 < x1 + w) && \
  (y0 >= y1 && y0 < y1 + h)

#define TIMER_START(...) \
  struct timeval _end = {0}; \
  struct timeval _start = {0}; \
  gettimeofday(&_start, NULL); \
  __VA_ARGS__

#define TIMER_END(...) (gettimeofday(&_end, NULL), ((((_end.tv_sec - _start.tv_sec) * 1000000.0f) + _end.tv_usec) - (_start.tv_usec)) / 1000000.0f)

#define DT_MIN 0.001f

typedef struct Mix {
  Vector2 mouse;
  Vector2 grab_offset;
  bool grab;
  f32 fps;
  f32 dt;
  Waveshaper waveshaper;
} Mix;

extern Mix mix;

i32 mix_main(i32 argc, char** argv);

#endif // _MIX_H
