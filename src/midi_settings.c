// midi_settings.c
// midi/keyboard piano settings

// TODO:
//  - clamp values

Element midi_settings_ui_new(Mix* mix) {
  (void)mix;

  Element settings = ui_container("midi settings");
  settings.border = true;
  settings.scissor = true;
  settings.placement = PLACEMENT_BLOCK;
  settings.background = true;
  const i32 button_height = 2 * FONT_SIZE;

  ui_attach_element_v2(&settings, ui_text_line("octave"));
  {
    Element e = ui_input_int("octave", &keyboard.octave);
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 100, .y = button_height, };
    ui_attach_element(&settings, &e);
  }
  ui_attach_element_v2(&settings, ui_text_line("velocity"));
  {
    Element e = ui_input_float("velocity", &keyboard.velocity);
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 100, .y = button_height, };
    ui_attach_element(&settings, &e);
  }
  ui_attach_element_v2(&settings, ui_text_line("channel"));
  {
    Element e = ui_input_int8("channel", (i8*)&keyboard.channel);
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 100, .y = button_height, };
    ui_attach_element(&settings, &e);
  }
  return settings;
}
