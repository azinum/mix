// effect.h
// TODO:
//  - add a plugin that sends midi events

#ifndef _EFFECT_H
#define _EFFECT_H

#define DECLARE_EFFECT(NAME) \
  void NAME##_init(Instrument* ins, struct Mix* mix); \
  void NAME##_ui_new(Instrument* ins, Element* container); \
  void NAME##_update(Instrument* ins, struct Mix* mix); \
  void NAME##_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt); \
  void NAME##_destroy(struct Instrument* ins)

typedef enum {
  EFFECT_CLIP_DISTORTION,
  EFFECT_FILTER,
  EFFECT_DELAY,
  EFFECT_SMOOTH,
  EFFECT_INTERPOLATOR,
  EFFECT_REVERB,

  MAX_EFFECT_ID,
} Effect_id;

typedef Instrument Effect;

extern Effect effects[MAX_EFFECT_ID];

DECLARE_EFFECT(fx_clip_distortion);
DECLARE_EFFECT(fx_filter);
DECLARE_EFFECT(fx_delay);
DECLARE_EFFECT(fx_smooth);
DECLARE_EFFECT(fx_interpolator);
DECLARE_EFFECT(fx_reverb);

Effect effect_new(Effect_id id);

#endif // _EFFECT_H
