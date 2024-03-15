// fx_smooth.c

typedef struct Smooth {
  f32 smoothness;
  f32 prev_sample;
  f32* buffer;
} Smooth;

static void fx_smooth_default(Smooth* smooth);
f32 fx_smooth(f32* input, f32* output, f32 bias, f32 prev, size_t samples);

void fx_smooth_default(Smooth* smooth) {
  smooth->smoothness = .5f;
  smooth->prev_sample = 0; // sample from previous audio buffer
  smooth->buffer = memory_calloc(sizeof(f32), CHANNEL_COUNT * FRAMES_PER_BUFFER);
}

f32 fx_smooth(f32* input, f32* output, f32 bias, f32 prev, size_t samples) {
  const f32 remainder = 1.0f - bias;

  output[0] = bias * input[0] + remainder * input[1];
  f32 sample = prev;
  for (size_t i = 1; i < samples; ++i) {
    prev = sample;
    sample = input[i];
    output[i] = bias * sample + remainder * prev;
  }
  return prev;
}

void fx_smooth_init(Instrument* ins) {
  MEMORY_TAG("fx_smooth: userdata");
  Smooth* smooth = memory_alloc(sizeof(Smooth));
  ASSERT(smooth != NULL);
  ins->userdata = smooth;
  fx_smooth_default(smooth);
}

void fx_smooth_ui_new(Instrument* ins, Element* container) {
  (void)ins;
  (void)container;

  Smooth* smooth = (Smooth*)ins->userdata;

  const i32 slider_height = FONT_SIZE;
  const i32 input_height = slider_height;

  {
    Element e = ui_text("smoothness");
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_input_float("smoothness", &smooth->smoothness);
    e.box = BOX(0, 0, 0, input_height);
    e.sizing = SIZING_PERCENT(15, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider(&smooth->smoothness, VALUE_TYPE_FLOAT, RANGE_FLOAT(0.0f, 1.0f));
    e.name = "smoothness";
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(35, 0);
    ui_attach_element(container, &e);
  }
}

void fx_smooth_update(Instrument* ins, Mix* mix) {
  (void)ins;
  (void)mix;
}

void fx_smooth_process(Instrument* ins, Mix* mix, Audio_engine* audio, f32 dt) {
  (void)ins;
  (void)mix;
  (void)audio;
  (void)dt;
  Smooth* smooth = (Smooth*)ins->userdata;
  smooth->prev_sample = fx_smooth(ins->out_buffer, ins->out_buffer, smooth->smoothness, smooth->prev_sample, ins->samples);
}

void fx_smooth_destroy(Instrument* ins) {
  Smooth* smooth = (Smooth*)ins->userdata;
  memory_free(smooth->buffer);
}
