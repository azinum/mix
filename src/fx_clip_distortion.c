// fx_clip_distortion.c

typedef struct Clip_distortion {
  f32 input_gain;
  f32 output_gain;
  f32 clip;
  f32 offset;
} Clip_distortion;

static void fx_clip_distortion_default(Clip_distortion* d);

void fx_clip_distortion_default(Clip_distortion* d) {
  d->input_gain   = 1.0f;
  d->output_gain  = 1.0f;
  d->clip         = 0.5f;
  d->offset       = 0.0f;
}

void fx_clip_distortion_init(Instrument* ins, Mix* mix) {
  (void)mix;
  MEMORY_TAG("fx_clip_distortion: userdata");
  Clip_distortion* d = memory_alloc(sizeof(Clip_distortion));
  ASSERT(d != NULL);
  ins->userdata = d;
  fx_clip_distortion_default(d);
}

void fx_clip_distortion_ui_new(Instrument* ins, Element* container) {
  Clip_distortion* d = (Clip_distortion*)ins->userdata;
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
    Element e = ui_slider(&d->clip, VALUE_TYPE_FLOAT, RANGE_FLOAT(0.0f, 1.0f));
    e.name = "clipping";
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_text("offset");
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider(&d->offset, VALUE_TYPE_FLOAT, RANGE_FLOAT(-1.0f, 1.0f));
    e.name = "offset";
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
}

void fx_clip_distortion_update(Instrument* ins, struct Mix* mix) {
  (void)ins;
  (void)mix;
}

void fx_clip_distortion_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt) {
  (void)mix;
  (void)audio;
  (void)dt;

  Clip_distortion* d = (Clip_distortion*)ins->userdata;

  for (size_t i = 0; i < ins->samples; ++i) {
    f32 sample = ins->out_buffer[i] + d->offset;
    ins->out_buffer[i] = CLAMP(sample * d->input_gain, -d->clip, d->clip) * d->output_gain - d->offset;
  }
}

void fx_clip_distortion_destroy(struct Instrument* ins) {
  (void)ins;
}
