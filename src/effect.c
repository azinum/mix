// effect.c

Effect effects[MAX_EFFECT_ID] = {
  [EFFECT_DISTORTION] = { .title = "distortion", .init = fx_distortion_init, .ui_new = fx_distortion_ui_new, .update = fx_distortion_update, .process = fx_distortion_process, .destroy = fx_distortion_destroy, },
};

Effect effect_new(Effect_id id) {
  Effect effect = {0};
  if (id < 0 || id > MAX_EFFECT_ID) {
    return effect;
  }
  effect = effects[id];
  instrument_init_default(&effect);
  return effect;
}
