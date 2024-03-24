// fx_interpolator.c

typedef struct Interpolator {
  f32 amount;
  i32 interval; // s[i] = linear_interpolate(s[i], s[i+interval], amount)
} Interpolator;

static void fx_interpolator_default(Interpolator* interp);

void fx_interpolator_default(Interpolator* interp) {
  interp->amount = 1.0f;
  interp->interval = 4;
}

void fx_interpolator_init(Instrument* ins) {
  MEMORY_TAG("fx_interpolator: userdata");
  Interpolator* interp = memory_alloc(sizeof(Interpolator));
  ins->userdata = interp;
  fx_interpolator_default(interp);
}

void fx_interpolator_ui_new(Instrument* ins, Element* container) {
  Interpolator* interp = (Interpolator*)ins->userdata;

  i32 slider_height = FONT_SIZE;
  {
    Element e = ui_text("amount");
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_text("interval");
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider_float(&interp->amount, 0, 1);
    e.name = "amount";
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider_int(&interp->interval, 1, 64);
    e.name = "interval";
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
}

void fx_interpolator_update(Instrument* ins, Mix* mix) {
  (void)ins; (void)mix;
}

void fx_interpolator_process(Instrument* ins, Mix* mix, Audio_engine* audio, f32 dt) {
  (void)mix; (void)audio; (void)dt;
  Interpolator* interp = (Interpolator*)ins->userdata;
  for (size_t i = 0; i < ins->samples; ++i) {
    f32 sample = ins->out_buffer[i];
    for (i32 index = 0; index < interp->interval && index+i < ins->samples; ++index) {
      f32 next = ins->out_buffer[(i + index) % ins->samples];
      ins->out_buffer[(i + index) % ins->samples] = lerp_f32(next, sample, interp->amount);
    }
    i += interp->interval;
  }
}

void fx_interpolator_destroy(Instrument* ins) {
  (void)ins;
}
