// fx_filter.c

#define FREQ_RANGE (20000.0f)

static void fx_filter_default(Filter* filter);
static void lowpass_filter(f32* input, f32* output, size_t samples, f32 dt_step, f32 dt);
static void smooth(f32* input, f32* output, size_t samples);

void fx_filter_default(Filter* filter) {
  filter->cutoff = 2000;
  filter->buffer = NULL;
}

void fx_filter_init(Instrument* ins) {
  MEMORY_TAG("fx_filter: userdata");
  Filter* filter = memory_alloc(sizeof(Filter));
  ASSERT(filter != NULL);
  ins->userdata = filter;
  fx_filter_default(filter);
  filter->buffer = memory_alloc(sizeof(f32) * ins->samples);
}

void lowpass_filter(f32* input, f32* output, size_t samples, f32 dt_step, f32 dt) {
  if (dt + dt_step <= 0.0001f) {
    return;
  }
  const f32 alpha = dt_step / (dt + dt_step);
  output[0] = alpha * input[0];
  for (size_t i = 1; i < samples; ++i) {
    output[i] = output[i - 1] + alpha * (input[i] - output[i - 1]);
  }
}

void smooth(f32* input, f32* output, size_t samples) {
  f32 prev = 0;
  f32 sample = input[0];
  const f32 bias = 0.6f;
  const f32 remainder = 1.0f - bias;

  output[0] = bias * input[0] + remainder * input[1];
  for (size_t i = 1; i < samples; ++i) {
    prev = sample;
    sample = input[i];
    output[i] = bias * sample + remainder * prev;
  }
}

void fx_filter_ui_new(Instrument* ins, Element* container) {
  Filter* filter = (Filter*)ins->userdata;

  const i32 slider_height = FONT_SIZE;
  const i32 input_height = slider_height;

  {
    Element e = ui_text("cutoff");
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_input_float("cutoff", &filter->cutoff);
    e.box = BOX(0, 0, 0, input_height);
    e.sizing = SIZING_PERCENT(15, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider(&filter->cutoff, VALUE_TYPE_FLOAT, RANGE_FLOAT(0.0f, FREQ_RANGE));
    e.name = "cutoff";
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(35, 0);
    ui_attach_element(container, &e);
  }
}

void fx_filter_update(Instrument* ins, struct Mix* mix) {
  (void)ins;
  (void)mix;
}

void fx_filter_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt) {
  Filter* filter = (Filter*)ins->userdata;
  f32 cutoff = dt * (filter->cutoff / FREQ_RANGE);
  lowpass_filter(ins->out_buffer, filter->buffer, ins->samples, cutoff, dt);
  smooth(filter->buffer, ins->out_buffer, ins->samples);
#if 0
    i32 peaks = 0;

    for (size_t i = 0; i < ins->samples - 2; ++i) {
      const f32 a = fabs(ins->out_buffer[i + 0]);
      const f32 b = fabs(ins->out_buffer[i + 1]);
      const f32 c = fabs(ins->out_buffer[i + 2]);
      peaks += (a < b) && (c < b);
    }

    const f32 cutoff = filter->cutoff;
    for (size_t i = 0; i < ins->samples - 1; ++i) {
      const f32 a = fabs(ins->out_buffer[i + 0]);
      const f32 b = fabs(ins->out_buffer[i + 1]);
      const f32 divisor_map[] = {
        a,
        b
      };
      const f32 div = divisor_map[a < b];
      if (div != 0) {
        f32 d = a / div;
        if (d > cutoff) {
          ins->out_buffer[i] /= d;
        }
      }
    }
  }
#endif
}

void fx_filter_destroy(struct Instrument* ins) {
  Filter* filter = (Filter*)ins->userdata;
  memory_free(filter->buffer);
  filter->buffer = NULL;
}