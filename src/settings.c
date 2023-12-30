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
    Element e = ui_input_int("window width", &WINDOW_WIDTH);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_text("window height");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_input_int("window height", &WINDOW_HEIGHT);
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
    Element e = ui_input_int("font size", &FONT_SIZE);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_text("target fps");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_input_int("target fps", &TARGET_FPS);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_text("frames per buffer");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_input_int("frames per buffer", &FRAMES_PER_BUFFER);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_text("sample rate");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_input_int("sample rate", &SAMPLE_RATE);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_text("channel count");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_input_int("channel count", &CHANNEL_COUNT);
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
    Element e = ui_input_int("bpm", &mix->bpm);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&settings, &e);
  }
  return settings;
}
