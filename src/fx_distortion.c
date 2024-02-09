// fx_distortion.c

static void fx_distortion_default(Distortion* d);

void fx_distortion_default(Distortion* d) {
  d->input_gain   = 1.0f;
  d->output_gain  = 1.0f;
  d->clip         = 0.5f;
}

void fx_distortion_init(Instrument* ins) {
  MEMORY_TAG("fx_distortion: userdata");
  Distortion* d = memory_alloc(sizeof(Distortion));
  ASSERT(d != NULL);
  ins->userdata = d;
  fx_distortion_default(d);
}

void fx_distortion_ui_new(Instrument* ins, Element* container) {
  Distortion* d = (Distortion*)ins->userdata;
  i32 slider_height = FONT_SIZE;
  {
    Element e = ui_text("input gain");
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_text("output gain");
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider(&d->input_gain, VALUE_TYPE_FLOAT, RANGE_FLOAT(0.0f, 5.0f));
    e.name = "input gain";
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider(&d->output_gain, VALUE_TYPE_FLOAT, RANGE_FLOAT(0.0f, 5.0f));
    e.name = "output gain";
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }

  {
    Element e = ui_text("clipping");
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider(&d->clip, VALUE_TYPE_FLOAT, RANGE_FLOAT(0.0f, 5.0f));
    e.name = "clipping";
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
}

void fx_distortion_update(Instrument* ins, struct Mix* mix) {
  (void)ins;
  (void)mix;
}

void fx_distortion_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt) {
  (void)mix;
  (void)audio;
  (void)dt;

  Distortion* d = (Distortion*)ins->userdata;

  for (size_t i = 0; i < ins->samples; ++i) {
    f32 sample = ins->out_buffer[i];
    ins->out_buffer[i] = CLAMP(sample * d->input_gain, -d->clip, d->clip) * d->output_gain;
  }
}

void fx_distortion_destroy(struct Instrument* ins) {
  (void)ins;
}
