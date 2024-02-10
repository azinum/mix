// effect.h

#ifndef _EFFECT_H
#define _EFFECT_H

typedef enum {
  EFFECT_DISTORTION,
  EFFECT_FILTER,

  MAX_EFFECT_ID,
} Effect_id;

typedef Instrument Effect;

extern Effect effects[MAX_EFFECT_ID];

Effect effect_new(Effect_id id);

#endif // _EFFECT_H
