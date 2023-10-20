// mix.h

#ifndef _MIX_H
#define _MIX_H

#include "config.h"
#include "lut.h"
#include "log.h"
#include "memory.h"
#include "entity.h"
#include "module.h"
#include "audio.h"

#include <raylib.h>

#define COLOR_RGB(R, G, B) ((Color) { .r = R, .g = G, .b = B, .a = 255, })
#define COLOR(R, G, B, A)  ((Color) { .r = R, .g = G, .b = B, .a = A, })

#define OVERLAP(x0, y0, x1, y1, w, h) \
  (x0 >= x1 && x0 < x1 + w) && \
  (y0 >= y1 && y0 < y1 + h)

#define MAX_ENTITY 512

typedef struct Mix {
  Entity null;
  Entity entities[MAX_ENTITY];
  size_t id;
  Entity* select;
  Entity* hover;
  Vector2 mouse;
  Vector2 grab_offset;
  bool grab;
} Mix;

i32 mix_main(i32 argc, char** argv);

#endif // _MIX_H
