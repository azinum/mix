// control_panel.c

static void control_panel_waveform_onrender(Element* e);
static void control_panel_change_audio_setting(Element* e);

void control_panel_waveform_onrender(Element* e) {
  TIMER_START();
  Audio_engine* audio = &audio_engine;
  const f32* buffer = audio->out_buffer;
  const size_t samples = (size_t)audio->frames_per_buffer;

  i32 width = e->box.w - 2 * e->border_thickness;
  i32 height = e->box.h;
  i32 x = e->box.x + e->border_thickness;
  i32 y = e->box.y + height / 2;

  Color color_map[2] = {
    lerp_color(COLOR_RGB(130, 235, 100), warmer_color(UI_BUTTON_COLOR, 40), 0.2f),
    lerp_color(COLOR_RGB(130, 235, 100), warmer_color(UI_BUTTON_COLOR, 30), 0.1f),
  };

  DrawLine(x, y, x + width, y, COLOR(255, 255, 255, 50));
  f32 sample = 0;
  f32 prev_sample = 0;
  f32 sample_index = 0;
  f32 sample_step = samples / (f32)width;
  for (i32 i = 0; i < width && sample_index < (f32)samples; ++i) {
    prev_sample = sample;
    sample = CLAMP(buffer[(size_t)sample_index], -1, 1);
    DrawLine(
      x + i,
      y + (height/2 * prev_sample),
      x + i + 1,
      y + (height/2 * sample),
      color_map[i % 2]
    );
    sample_index += sample_step;
  }
  f32 dt = TIMER_END();
  (void)dt;
}

void control_panel_change_audio_setting(Element* e) {
  (void)e;
  audio_engine_restart();
}

void control_panel_ui_new(Mix* mix, Element* container) {
  const i32 button_height = FONT_SIZE;
  {
    Element e = ui_canvas(true);
    e.box.w = 256;
    e.sizing = SIZING_PERCENT(20, 100);
    e.onrender = control_panel_waveform_onrender;
    e.background_color = lerp_color(UI_BACKGROUND_COLOR, COLOR_RGB(0, 0, 0), 0.1f);
    ui_attach_element(container, &e);
  }
  Element* rhs_container = NULL;
  {
    Element e = ui_container(NULL);
    e.sizing = SIZING_PERCENT(80, 100);
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
}
