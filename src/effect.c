// effect.c

#define DEFINE_EFFECT(ID, NAME, TITLE) [ID] = { .title = TITLE, .init = NAME##_init, .ui_new = NAME##_ui_new, .update = NAME##_update, .process = NAME##_process, .destroy = NAME##_destroy, }

Effect effects[MAX_EFFECT_ID] = {
  DEFINE_EFFECT(EFFECT_CLIP_DISTORTION, fx_clip_distortion, "clip distortion"),
  DEFINE_EFFECT(EFFECT_FILTER, fx_filter, "filter"),
  DEFINE_EFFECT(EFFECT_DELAY, fx_delay, "delay"),
  DEFINE_EFFECT(EFFECT_SMOOTH, fx_smooth, "smooth"),
  DEFINE_EFFECT(EFFECT_INTERPOLATOR, fx_interpolator, "interpolator"),
  DEFINE_EFFECT(EFFECT_REVERB, fx_reverb, "reverb"),
};

Effect effect_new(Effect_id id) {
  Effect effect = {0};
  if (id < 0 || id > MAX_EFFECT_ID) {
    return effect;
  }
  effect = effects[id];
  instrument_init_default(&effect);
  effect.volume = 1;
  effect.wet = 0.4f;
  return effect;
}
