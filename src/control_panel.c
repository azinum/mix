// control_panel.c

static void control_panel_waveform_onrender(Element* e);

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

  for (size_t i = 0; i < samples; ++i) {
    f32 sample = CLAMP(buffer[i], -1.0f, 1.0f);
    i32 x_pos = x + ((f32)i/samples) * width;
    DrawLine(
      x_pos,               // x1
      y,                   // y1
      x_pos,               // x2
      y + (height/2 * sample), // y2
      color_map[(i % 2) == 0]
    );
  }
  f32 dt = TIMER_END();
  (void)dt;
}

void control_panel_ui_new(Mix* mix, Element* container) {
  Audio_engine* audio = &audio_engine;

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
    rhs_container = ui_attach_element(container, &e);
  }

  {
    Element e = ui_input_int("bpm", &mix->bpm);
    e.box.w = FONT_SIZE * 4;
    e.box.h = button_height;
    ui_attach_element(rhs_container, &e);
  }
  {
    Element e = ui_text_ex("bpm", false);
    e.box.h = button_height;
    ui_attach_element(rhs_container, &e);
  }
}
