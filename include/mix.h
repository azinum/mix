// mix.h

#ifndef _MIX_H
#define _MIX_H

struct Mix;

#include <raylib.h>
#include <sys/time.h>
#include <math.h>
#include <fcntl.h>

#include "hash.h"
#include "random.h"
#include "buffer.h"
#include "config.h"
#include "misc.h"
#include "lut.h"
#include "log.h"
#include "memory.h"
#include "entity.h"
#include "module.h"
#include "wave_shaper.h"
#include "audio.h"
#include "ui.h"

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

#define INIT_ITEMS_SIZE 16
#define list_init(list, desired_size) \
  if ((list)->size < desired_size) { \
    (list)->size = desired_size; \
    (list)->items = memory_realloc((list)->items, (list)->size * sizeof(*(list)->items)); \
    ASSERT((list)->items != NULL && "out of memory"); \
  }

#define list_push(list, item) \
  if ((list)->count >= (list)->size) { \
    if ((list)->size == 0) { \
      (list)->size = INIT_ITEMS_SIZE; \
    } \
    else { \
      (list)->size *= 2; \
    } \
    (list)->items = memory_realloc((list)->items, (list)->size * sizeof(*(list)->items)); \
    ASSERT((list)->items != NULL && "out of memory"); \
  } \
  (list)->items[(list)->count++] = (item)

#define list_free(list) memory_free((list)->items)

#define DT_MIN 0.001f

typedef struct Mix {
  Vector2 mouse;
  f32 fps;
  f32 dt;
} Mix;

typedef struct Assets {
  Font font;
} Assets;

extern Mix mix;
extern Assets assets;

i32 mix_main(i32 argc, char** argv);

#endif // _MIX_H
