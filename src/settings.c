// settings.c
// TODO:
//  - use the same hooks here as in config

void config_store_onclick(Element* e);

void config_store_onclick(Element* e) {
  (void)e;
  config_store(CONFIG_PATH);
}

Element settings_ui_new(Mix* mix) {
  (void)mix;
  Element settings = ui_container("settings");
  settings.border = true;
  settings.scissor = true;
  settings.placement = PLACEMENT_BLOCK;
  settings.background = true;
  const i32 button_height = 2 * FONT_SIZE;

  ui_attach_element_v2(&settings, ui_text_line("window width"));
  {
    Element e = ui_input_int("window width", &WINDOW_WIDTH);
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 100, .y = button_height, };
    ui_attach_element(&settings, &e);
  }
  ui_attach_element_v2(&settings, ui_text_line("window height"));
  {
    Element e = ui_input_int("window height", &WINDOW_HEIGHT);
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 100, .y = button_height, };
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_toggle_ex(&WINDOW_RESIZABLE, "resizable window");
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 100, .y = button_height, };
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_toggle_ex(&WINDOW_FULLSCREEN, "fullscreen");
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 100, .y = button_height, };
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_toggle_ex(&VSYNC, "vsync");
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 100, .y = button_height, };
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_toggle_ex(&MSAA_4X, "msaa 4x");
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 100, .y = button_height, };
    ui_attach_element(&settings, &e);
  }
  ui_attach_element_v2(&settings, ui_text_line("font size"));
  {
    Element e = ui_input_int("font size", &FONT_SIZE);
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 100, .y = button_height, };
    ui_attach_element(&settings, &e);
  }
  ui_attach_element_v2(&settings, ui_text_line("target fps"));
  {
    Element e = ui_input_int("target fps", &TARGET_FPS);
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 100, .y = button_height, };
    ui_attach_element(&settings, &e);
  }
  ui_attach_element_v2(&settings, ui_text_line("frames per buffer"));
  {
    Element e = ui_input_int("frames per buffer", &FRAMES_PER_BUFFER);
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 100, .y = button_height, };
    ui_attach_element(&settings, &e);
  }
  ui_attach_element_v2(&settings, ui_text_line("sample rate"));
  {
    Element e = ui_input_int("sample rate", &SAMPLE_RATE);
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 100, .y = button_height, };
    ui_attach_element(&settings, &e);
  }
  ui_attach_element_v2(&settings, ui_text_line("channel count"));
  {
    Element e = ui_input_int("channel count", &CHANNEL_COUNT);
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 100, .y = button_height, };
    ui_attach_element(&settings, &e);
  }
  ui_attach_element_v2(&settings, ui_line_break(UI_LINE_SPACING * 2));
  {
    Element e = ui_button("save settings");
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 100, .y = button_height, };
    e.onclick = config_store_onclick;
    e.tooltip = "save settings to " CONFIG_PATH;
    ui_attach_element(&settings, &e);
  }
  return settings;
}
