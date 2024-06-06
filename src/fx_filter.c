// fx_filter.c

#define FREQ_RANGE (SAMPLE_RATE*0.5f)

// #define VERSION2

typedef struct Filter {
  f32 cutoff;
  f32* buffer;
} Filter;

static void fx_filter_default(Filter* filter);
static void lowpass_filter(f32* input, f32* output, size_t samples, f32 dt_step, f32 dt);
static void smooth(f32* input, f32* output, size_t samples);
static f32 linear_to_logarithmic(f32 low, f32 high, f32 n);

void fx_filter_default(Filter* filter) {
  filter->cutoff = 2000;
  filter->buffer = NULL;
}

void fx_filter_init(Instrument* ins, Mix* mix) {
  (void)mix;
  MEMORY_TAG("fx_filter: userdata");
  Filter* filter = memory_alloc(sizeof(Filter));
  ASSERT(filter != NULL);
  ins->userdata = filter;
  fx_filter_default(filter);
  filter->buffer = memory_alloc(sizeof(f32) * ins->samples);
}

// https://stackoverflow.com/questions/17674654/generating-set-of-n-numbers-in-an-integer-range-at-logarithmic-distance-in-c
f32 linear_to_logarithmic(f32 low, f32 high, f32 n) {
  if (UNLIKELY(n <= 0)) {
    return 0;
  }
  f32 gap = (log(high) - log(low)) / n;
  return n * exp(gap);
}

void lowpass_filter(f32* input, f32* output, size_t samples, f32 dt_step, f32 dt) {
#ifdef VERSION2
  f32 t = tanf((PI32 * dt_step) / SAMPLE_RATE);
  f32 a = (t - 1.0f) / (t + 1.0f);
  f32 prev = 0;
  for (size_t i = 0; i < samples; ++i) {
    f32 input_sample = input[i];
    f32 filter_sample = a * input_sample + prev;
    prev = input_sample - a * filter_sample;
    output[i] = 0.5f * (input_sample + filter_sample);
  }
#else
  if (dt + dt_step <= 0.0001f) {
    return;
  }
  const f32 alpha = dt_step / (dt + dt_step);
  f32 prev = 0;
  output[0] = alpha * input[0];
  for (size_t i = 1; i < samples; ++i) {
    prev = output[i - 1];
    output[i] = prev + alpha * (input[i] - prev);
  }
#endif
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

void fx_filter_update(Instrument* ins, Mix* mix) {
  (void)ins;
  (void)mix;
}

void fx_filter_process(Instrument* ins, Mix* mix, Audio_engine* audio, f32 dt) {
  (void)linear_to_logarithmic;
  (void)mix;
  (void)audio;
  Filter* filter = (Filter*)ins->userdata;
#ifdef VERSION2
  f32 cutoff = filter->cutoff;
#else
  f32 cutoff = dt * (filter->cutoff / FREQ_RANGE);
#endif
  lowpass_filter(ins->in_buffer, filter->buffer, ins->samples, cutoff, dt);
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

void fx_filter_destroy(Instrument* ins) {
  Filter* filter = (Filter*)ins->userdata;
  memory_free(filter->buffer);
  filter->buffer = NULL;
}
