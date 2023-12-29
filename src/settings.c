// settings.c

void config_store_onclick(Element* e);

void config_store_onclick(Element* e) {
  (void)e;
  config_store(CONFIG_PATH);
}

Element settings_ui_new(Mix* mix) {
  Element settings = ui_container("settings");
  settings.border = true;
  settings.scissor = true;
  settings.placement = PLACEMENT_BLOCK;
  settings.background = true;
  const i32 button_height = 48;
  const i32 button_height_small = FONT_SIZE;
  {
    Element e = ui_text("window width");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_input_ex2("window width", &WINDOW_WIDTH, INPUT_NUMBER, VALUE_TYPE_INTEGER);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_text("window height");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_input_ex2("window height", &WINDOW_HEIGHT, INPUT_NUMBER, VALUE_TYPE_INTEGER);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_toggle_ex(&WINDOW_RESIZABLE, "resizable window");
    e.box = BOX(0, 0, 0, button_height_small);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_toggle_ex(&WINDOW_FULLSCREEN, "fullscreen");
    e.box = BOX(0, 0, 0, button_height_small);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_toggle_ex(&VSYNC, "vsync");
    e.box = BOX(0, 0, 0, button_height_small);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_toggle_ex(&MSAA_4X, "msaa 4x");
    e.box = BOX(0, 0, 0, button_height_small);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_text("font size");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_input_ex2("font size", &FONT_SIZE, INPUT_NUMBER, VALUE_TYPE_INTEGER);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_text("target fps");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_input_ex2("target fps", &TARGET_FPS, INPUT_NUMBER, VALUE_TYPE_INTEGER);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_text("frames per buffer");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_input_ex2("frames per buffer", &FRAMES_PER_BUFFER, INPUT_NUMBER, VALUE_TYPE_INTEGER);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_text("sample rate");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_input_ex2("sample rate", &SAMPLE_RATE, INPUT_NUMBER, VALUE_TYPE_INTEGER);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_text("channel count");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_input_ex2("channel count", &CHANNEL_COUNT, INPUT_NUMBER, VALUE_TYPE_INTEGER);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_button("save settings");
    e.box = BOX(0, 0, 0, button_height);
    e.sizing = SIZING_PERCENT(100, 0);
    e.onclick = config_store_onclick;
    e.tooltip = "save settings to " CONFIG_PATH;
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_text("bpm");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_input_ex2("bpm", &mix->bpm, INPUT_NUMBER, VALUE_TYPE_INTEGER);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  return settings;
}
