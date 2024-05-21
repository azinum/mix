// control_panel.c
// TODO:
//  - add time elapsed
//  - add recorded time elapsed
//  - add ability to export recorded audio, instead of exporting on exit

static void control_panel_render_waveform(Element* e);
static void control_panel_change_audio_setting(Element* e);
static void control_panel_stop(Element* e);
static void control_panel_export_recording(Element* e);
static void control_panel_export_update(Element* e);

static void control_panel_render_waveform(Element* e) {
  TIMER_START();
  Audio_engine* audio = &audio_engine;
  const f32* buffer = audio->out_buffer;
  const size_t samples = (size_t)audio->frames_per_buffer;

  ui_audio_render_curve(buffer, samples, BOX(e->box.x + e->border_thickness, e->box.y, e->box.w - 2 * e->border_thickness, e->box.h), COLOR_RGB(130, 190, 100), false, 0);

  f32 dt = TIMER_END();
  (void)dt;
}

void control_panel_change_audio_setting(Element* e) {
  (void)e;
  audio_engine_restart();
}

void control_panel_stop(Element* e) {
  (void)e;
  mix_stop();
}

void control_panel_export_recording(Element* e) {
  (void)e;
  audio_engine_export_recording();
}

void control_panel_export_update(Element* e) {
  Audio_engine* audio = (Audio_engine*)e->userdata;
  e->render = audio->recording;
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
    e.data.slider.slider_type = SLIDER_VERTICAL;
    e.sizing = SIZING_PERCENT(4, 100);
    e.secondary_color = COLOR_RGB(70, 170, 60);
    e.readonly = true;
    e.tooltip = "volume (RMS)";
    ui_attach_element(container, &e);
  }
#ifndef NO_RECORD_BUFFER
  {
    Element e = ui_slider_int(&audio->record_buffer_index, 0, audio->record_buffer_size);
    e.data.slider.slider_type = SLIDER_VERTICAL;
    e.sizing = SIZING_PERCENT(2, 100);
    e.secondary_color = COLOR_RGB(175, 80, 85);
    e.readonly = true;
    e.tooltip = "record buffer";
    ui_attach_element(container, &e);
  }
#endif
  Element* rhs_container = NULL;
  {
    Element e = ui_container(NULL);
    e.sizing = SIZING_PERCENT(74, 100);
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
#ifndef TARGET_ANDROID
  {
    Element e = ui_text_ex("tick", false);
    e.box.h = button_height;
    ui_attach_element(rhs_container, &e);
  }
  {
    Element e = ui_input_int("tick", (i32*)&mix->timed_tick);
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
    e.onmodify = control_panel_change_audio_setting;
    ui_attach_element(rhs_container, &e);
  }
#endif
  {
    Element e = ui_toggle_ex2(&mix->paused, "pause", "play");
    e.box = BOX(0, 0, FONT_SIZE * 7, button_height);
    ui_attach_element(rhs_container, &e);
  }
  {
    Element e = ui_button("stop");
    e.box = BOX(0, 0, FONT_SIZE * 7, button_height);
    e.background_color = lerp_color(UI_BUTTON_COLOR, COLOR_RGB(175, 80, 85), 0.3f);
    e.onclick = control_panel_stop;
    ui_attach_element(rhs_container, &e);
  }
#ifndef NO_RECORD_BUFFER
  {
    Element e = ui_toggle_ex2(&audio->recording, "record", "recording");
    e.box.w = FONT_SIZE * 7;
    e.box.h = button_height;
    e.background_color = COLOR_RGB(175, 80, 85);
    ui_attach_element(rhs_container, &e);
  }
  {
    Element e = ui_button("export");
    e.box.w = FONT_SIZE * 7;
    e.box.h = button_height;
    e.tooltip = "export recorded audio";
    e.onclick = control_panel_export_recording;
    e.onupdate = control_panel_export_update;
    e.userdata = audio;
    ui_attach_element(rhs_container, &e);
  }
#endif
  ui_attach_element_v2(rhs_container, ui_line_break(0));
  {
    Element e = ui_button("?");
    e.readonly = true;
    e.background_color = lerp_color(UI_BUTTON_COLOR, COLOR_RGB(127, 127, 127), 0.4f);
    e.box = BOX(0, 0, button_height, button_height);
    e.tooltip =
      "CONTROLS\n\n"
      "spacebar           - play/pause\n"
      "number keys/numpad - change menu\n"
      "ctrl + spacebar    - stop\n"
      "ctrl + c           - copy (when hovering input elements)\n"
      "ctrl + v           - paste (when hovering input elements)\n"
      "ctrl + scroll      - increment/decrement\n    value (when hovering input or slider elements)\n"
      "ctrl + f           - zoom/unzoom\n"
      "escape             - unzoom (if zoomed), otherwise switch\n    to default menu"
    ;
    ui_attach_element(rhs_container, &e);
  }
}
