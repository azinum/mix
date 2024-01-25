// dummy.c

void dummy_init(Instrument* ins) {
  (void)ins;
}

void dummy_ui_new(Instrument* ins, Element* container) {
  (void)ins;
  ui_set_slider_deadzone(0.0f);
  const i32 button_height = FONT_SIZE * 2;
  {
    Element e = ui_button("dummy");
    e.box.h = button_height;
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(container, &e);
  }
}

void dummy_update(Instrument* ins, struct Mix* mix) {
  (void)ins;
  (void)mix;
}

void dummy_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt) {
  (void)ins;
  (void)mix;
  (void)audio;
  (void)dt;
}

void dummy_destroy(struct Instrument* ins) {
  (void)ins;
}
