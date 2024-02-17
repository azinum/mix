// control_panel.c

static void control_panel_render_waveform(Element* e);
static void control_panel_change_audio_setting(Element* e);

static void control_panel_render_waveform(Element* e) {
  TIMER_START();
  Audio_engine* audio = &audio_engine;
  const f32* buffer = audio->out_buffer;
  const size_t samples = (size_t)audio->frames_per_buffer;

  mix_render_curve(buffer, samples, BOX(e->box.x + e->border_thickness, e->box.y, e->box.w - 2 * e->border_thickness, e->box.h), COLOR_RGB(130, 190, 100));

  f32 dt = TIMER_END();
  (void)dt;
}

void control_panel_change_audio_setting(Element* e) {
  (void)e;
  audio_engine_restart();
}

void control_panel_ui_new(Mix* mix, Element* container) {
  Audio_engine* audio = &audio_engine;
  const i32 button_height = FONT_SIZE;
  {
    Element e = ui_canvas(true);
    e.box.w = 256;
    e.sizing = SIZING_PERCENT(20, 100);
    e.onrender = control_panel_render_waveform;
    e.background_color = lerp_color(UI_BACKGROUND_COLOR, COLOR_RGB(0, 0, 0), 0.1f);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider_float(&audio->db, 0, 1);
    e.data.slider.vertical = true;
    e.sizing = SIZING_PERCENT(4, 100);
    e.secondary_color = COLOR_RGB(70, 170, 60);
    e.readonly = true;
    ui_attach_element(container, &e);
  }
  Element* rhs_container = NULL;
  {
    Element e = ui_container(NULL);
    e.sizing = SIZING_PERCENT(76, 100);
    e.placement = PLACEMENT_BLOCK;
    e.background = true;
    e.background_color = lerp_color(e.background_color, UI_INTERPOLATION_COLOR, 0.05f);
    rhs_container = ui_attach_element(container, &e);
  }

  {
    Element e = ui_text_ex("bpm", false);
    e.box.h = button_height;
    ui_attach_element(rhs_container, &e);
  }
  {
    Element e = ui_input_int("bpm", &mix->bpm);
    e.box.w = FONT_SIZE * 4;
    e.box.h = button_height;
    ui_attach_element(rhs_container, &e);
  }
  {
    Element e = ui_text_ex("timed tick", false);
    e.box.h = button_height;
    ui_attach_element(rhs_container, &e);
  }
  {
    Element e = ui_input_int("timed tick", (i32*)&mix->timed_tick);
    e.box.w = FONT_SIZE * 4;
    e.box.h = button_height;
    ui_attach_element(rhs_container, &e);
  }
  {
    Element e = ui_text_ex("sample rate", false);
    e.box.h = button_height;
    ui_attach_element(rhs_container, &e);
  }
  {
    Element e = ui_input_int("sample rate", &SAMPLE_RATE);
    e.box.w = FONT_SIZE * 4;
    e.box.h = button_height;
    e.tooltip = "changing the sample rate restarts the audio engine";
    e.onenter = control_panel_change_audio_setting;
    ui_attach_element(rhs_container, &e);
  }
#ifndef NO_RECORD_BUFFER
  {
    Audio_engine* audio = &audio_engine;
    Element e = ui_toggle_ex2(&audio->recording, "record", "recording");
    e.box.w = FONT_SIZE * 7;
    e.box.h = button_height;
    e.background_color = COLOR_RGB(175, 80, 85);
    ui_attach_element(rhs_container, &e);
  }
#endif
  {
    Element e = ui_toggle_ex2(&mix->paused, "pause", "play");
    e.box = BOX(0, 0, FONT_SIZE * 7, button_height);
    ui_attach_element(rhs_container, &e);
  }
}
