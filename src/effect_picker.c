// effect_picker.c

static void add_effect(Element* e);
static void clear_effects(Element* e);

void add_effect(Element* e) {
  (void)e;
  i32 id = e->v.i;
  Mix* mix = (Mix*)e->userdata;
  ASSERT(mix != NULL);
  if (id >= 0 && id < MAX_EFFECT_ID) {
    Effect ef = effect_new(id);
    Effect* effect = audio_engine_attach_effect(&ef);
    if (!effect) {
      ui_alert("could not add effect, exceeded the maximum number (%d) of effects", MAX_EFFECTS);
      return;
    }
    if (mix->effect_chain) {
      Element e = ui_container(effect->title);
      e.border = true;
      e.scissor = false;
      e.placement = PLACEMENT_BLOCK;
      e.background = true;
      Hsv hsv = rgb_to_hsv(UI_BUTTON_COLOR);
      hsv.s = 0.4f;
      hsv.h += 0.05f;
      e.background_color = lerp_color(e.background_color, hsv_to_rgb(hsv), 0.1f);
#ifdef TARGET_ANDROID
      e.sizing = SIZING_PERCENT(100, 50);
#else
      e.sizing = SIZING_PERCENT(100, 25);
#endif
      Element* effect_container = ui_attach_element(mix->effect_chain, &e);
      instrument_ui_new(effect, effect_container);
    }
  }
}

void clear_effects(Element* e) {
  (void)e;
  Mix* mix = (Mix*)e->userdata;
  ASSERT(mix != NULL);
  audio_engine_clear_effects();
  if (!mix->effect_chain) {
    return;
  }
  ui_detach_elements(mix->effect_chain);
  ui_set_title(mix->effect_chain, "effect chain");
}

void effect_picker_ui_new(struct Mix* mix, Element* container) {
  i32 button_height = FONT_SIZE;
  const i32 line_break_height = FONT_SIZE / 2;
  Element line_break = ui_line_break(line_break_height);

  {
    Element e = ui_text("built-in effects");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(container, &e);
  }

  for (size_t i = 0; i < MAX_EFFECT_ID; ++i) {
    const Effect* ins = &effects[i];
    {
      Element e = ui_text(ins->title);
      e.sizing = SIZING_PERCENT(70, 0);
      ui_attach_element(container, &e);
    }
    {
      Element e = ui_button("load");
      e.box.h = button_height;
      e.sizing = SIZING_PERCENT(30, 0);
      e.v.i = (i32)i;
      e.onclick = add_effect;
      e.userdata = mix;
      ui_attach_element(container, &e);
    }
  }

  ui_attach_element(container, &line_break);

  button_height = FONT_SIZE * 2;
  {
    Element e = ui_button("clear effects");
    e.box.h = button_height;
    e.sizing = SIZING_PERCENT(100, 0);
    e.background_color = warmer_color(e.background_color, 80);
    e.tooltip = "remove all effects/filters";
    e.onclick = clear_effects;
    e.userdata = mix;
    ui_attach_element(container, &e);
  }
}
