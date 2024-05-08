// fx_delay.c
// TODO:
//  - wet/dry signal

typedef struct Delay {
  f32* feedback_left;
  f32* feedback_right;
  size_t feedback_buffer_size;
  size_t tick;

  f32 amount_left;
  i32 offset_left;

  f32 amount_right;
  i32 offset_right;
} Delay;

static void fx_delay_default(Delay* delay);

void fx_delay_default(Delay* delay) {
  const size_t feedback_buffer_size = MS_TO_SAMPLES(SAMPLE_RATE, CHANNEL_COUNT, 1000);

  delay->feedback_left  = memory_calloc(sizeof(f32), feedback_buffer_size);
  delay->feedback_right = memory_calloc(sizeof(f32), feedback_buffer_size);

  delay->feedback_buffer_size = feedback_buffer_size;
  delay->tick = 0;

  delay->amount_left = 0.3f;
  delay->offset_left = (i32)MS_TO_SAMPLES(SAMPLE_RATE, CHANNEL_COUNT, 400);

  delay->amount_right = 0.3f;
  delay->offset_right = (i32)MS_TO_SAMPLES(SAMPLE_RATE, CHANNEL_COUNT, 500);
}

void fx_delay_init(Instrument* ins, Mix* mix) {
  (void)mix;
  Delay* delay = memory_alloc(sizeof(Delay));
  ASSERT(delay != NULL); // out of memory, handle this gracefully

  ins->userdata = delay;
  fx_delay_default(delay);
}

void fx_delay_ui_new(Instrument* ins, Element* container) {
  const i32 button_height = 2 * FONT_SIZE;
  Delay* delay = (Delay*)ins->userdata;

  {
    Element e = ui_text("feedback - left channel");
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_text("feedback - right channel");
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }

  {
    Element e = ui_input_float("left amount", &delay->amount_left);
    e.sizing = SIZING_PERCENT(20, 0);
    e.box.h = button_height;
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider_float(&delay->amount_left, 0.0f, 0.99f);
    e.sizing = SIZING_PERCENT(30, 0);
    e.box.h = button_height;
    ui_attach_element(container, &e);
  }

  {
    Element e = ui_input_float("right amount", &delay->amount_right);
    e.sizing = SIZING_PERCENT(20, 0);
    e.box.h = button_height;
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider_float(&delay->amount_right, 0.0f, 0.99f);
    e.sizing = SIZING_PERCENT(30, 0);
    e.box.h = button_height;
    ui_attach_element(container, &e);
  }

  {
    Element e = ui_text("offset - left channel");
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_text("offset - right channel");
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }

  {
    Element e = ui_input_int("left offset", &delay->offset_left);
    e.sizing = SIZING_PERCENT(20, 0);
    e.box.h = button_height;
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider_int(&delay->offset_left, 0, delay->feedback_buffer_size);
    e.sizing = SIZING_PERCENT(30, 0);
    e.box.h = button_height;
    ui_attach_element(container, &e);
  }

  {
    Element e = ui_input_int("right offset", &delay->offset_right);
    e.sizing = SIZING_PERCENT(20, 0);
    e.box.h = button_height;
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider_int(&delay->offset_right, 0, delay->feedback_buffer_size);
    e.sizing = SIZING_PERCENT(30, 0);
    e.box.h = button_height;
    ui_attach_element(container, &e);
  }
}

void fx_delay_update(Instrument* ins, struct Mix* mix) {
  (void)ins; (void)mix;
}

void fx_delay_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt) {
  (void)dt;
  (void)audio;
  (void)mix;
  Delay* delay = (Delay*)ins->userdata;

  for (size_t i = 0; i < ins->samples; i += 2) {
    ins->out_buffer[i + 0] += delay->feedback_left[(delay->tick + 0) % delay->feedback_buffer_size];
    ins->out_buffer[i + 1] += delay->feedback_right[(delay->tick + 1) % delay->feedback_buffer_size];

    f32 left_feedback   = delay->amount_left  * ins->out_buffer[i + 0];
    f32 right_feedback  = delay->amount_right * ins->out_buffer[i + 1];

    delay->feedback_left[(delay->tick + abs(delay->offset_left)) % delay->feedback_buffer_size] = left_feedback;
    delay->feedback_right[(delay->tick + abs(delay->offset_right) + 1) % delay->feedback_buffer_size] = right_feedback;

    delay->tick = (delay->tick + 2) % delay->feedback_buffer_size;
  }
}

void fx_delay_destroy(struct Instrument* ins) {
  Delay* delay = (Delay*)ins->userdata;
  memory_free(delay->feedback_left);
  memory_free(delay->feedback_right);
}
