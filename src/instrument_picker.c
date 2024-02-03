// instrument_picker.c

static void picker_load_instrument(Element* e);
static void picker_detach_current(Element* e);

void picker_load_instrument(Element* e) {
  i32 id = e->v.i;
  Mix* mix = (Mix*)e->userdata;
  ASSERT(mix != NULL);
  if (id >= 0 && id < MAX_INSTRUMENT_ID) {
    Instrument ins = instrument_new(id);
    Instrument* instrument = audio_engine_attach_instrument(&ins);
    instrument_ui_new(instrument, mix->ins_container);
  }
}

void picker_detach_current(Element* e) {
  (void)e;
  audio_engine_detach_instrument();
}

Element instrument_picker_ui_new(struct Mix* mix) {
  Element picker = ui_container("instrument picker");
  picker.border = true;
  picker.scissor = true;
  picker.placement = PLACEMENT_BLOCK;
  picker.background = true;
  const i32 button_height = FONT_SIZE;
  const i32 line_break_height = FONT_SIZE / 2;
  Element line_break = ui_line_break(line_break_height);

  {
    Element e = ui_text("built-in instruments");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(&picker, &e);
  }

  for (size_t i = 0; i < MAX_INSTRUMENT_ID; ++i) {
    const Instrument* ins = &instruments[i];
    {
      Element e = ui_text(ins->title);
      e.sizing = SIZING_PERCENT(70, 0);
      ui_attach_element(&picker, &e);
    }
    {
      Element e = ui_button("load");
      e.box.h = button_height;
      e.sizing = SIZING_PERCENT(30, 0);
      e.v.i = (i32)i;
      e.onclick = picker_load_instrument;
      e.userdata = mix;
      ui_attach_element(&picker, &e);
    }
  }

  ui_attach_element(&picker, &line_break);

  {
    Element e = ui_button("detach");
    e.box.h = button_height * 2;
    e.sizing = SIZING_PERCENT(100, 0);
    e.onclick = picker_detach_current;
    e.background_color = warmer_color(e.background_color, 80);
    e.tooltip = "detach current instrument";
    ui_attach_element(&picker, &e);
  }
  return picker;
}
