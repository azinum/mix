// wave_shaper.c

#define ARENA_SIZE 1024 * 2
#define MAX_TEXT_SIZE 512

static void waveshaper_canvas_onrender(Element* e);
static void waveshaper_reset_onclick(Element* e);

void waveshaper_canvas_onrender(Element* e) {
  TIMER_START();
  Instrument* ins = (Instrument*)e->userdata;
  Waveshaper* w = (Waveshaper*)ins->userdata;

  if (!w->render) {
    return;
  }

  i32 width = e->box.w - 2 * e->border_thickness;
  i32 height = e->box.h;
  i32 x = e->box.x + e->border_thickness;
  i32 y = e->box.y + height / 2;

  Color color_map[2] = {
    lerp_color(COLOR_RGB(40, 255, 40), warmer_color(UI_BUTTON_COLOR, 40), 0.6f),
    lerp_color(COLOR_RGB(40, 255, 40), warmer_color(UI_BUTTON_COLOR, 30), 0.5f),
  };

  for (i32 i = 0; i < (i32)ins->frames && i < width; i += 1) {
    f32 frame = CLAMP(ins->buffer[i], -1.0f, 1.0f);
    DrawLine(
      x + i,               // x1
      y,                   // y1
      x + i,               // x2
      y + (height/2 * frame), // y2
      color_map[(i % 2) == 0]
    );
  }

  ins->latency += TIMER_END();
}

void waveshaper_reset_onclick(Element* e) {
  Instrument* ins = (Instrument*)e->userdata;
  Waveshaper* w = (Waveshaper*)ins->userdata;
  w->tick = 0;
  w->freq_target = 55;
  w->lfo_target = 0;
  ins->volume = 0.1f;
}

void waveshaper_init(Instrument* ins) {
  Waveshaper* w = memory_alloc(sizeof(Waveshaper));
  ins->userdata = w;
  *w = (Waveshaper) {
    .tick = 0,
    .freq = 55,
    .freq_target = 55,
    .lfo = 0.0f,
    .lfo_target = 0.0f,
    .reshape = true,
    .mute = false,
    .arena = arena_new(ARENA_SIZE),
    .text = NULL,
    .render = true,
  };
  w->text = arena_alloc(&w->arena, MAX_TEXT_SIZE);
}

void waveshaper_ui_new(Instrument* ins, Element* container) {
  Waveshaper* w = (Waveshaper*)ins->userdata;
  {
    Element e = ui_text(w->text);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_canvas(true);
    e.sizing = SIZING_PERCENT(100, 30);
    e.userdata = ins;
    e.border_thickness = 1.0f;
    e.background_color = lerp_color(UI_BACKGROUND_COLOR, COLOR_RGB(0, 0, 0), 0.1f);
    e.onrender = waveshaper_canvas_onrender;
    ui_attach_element(container, &e);
  }
  const i32 button_height = 48;
  const i32 slider_height = 38;
  {
    Element e = ui_toggle_ex(&w->mute, "mute");
    e.box = BOX(0, 0, 0, button_height);
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_toggle_ex(&w->reshape, "reshape");
    e.box = BOX(0, 0, 0, button_height);
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_toggle_ex(&w->render, "render");
    e.box = BOX(0, 0, 0, button_height);
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_button("reset");
    e.box = BOX(0, 0, 0, button_height);
    e.sizing = SIZING_PERCENT(50, 0);
    e.onclick = waveshaper_reset_onclick;
    e.userdata = ins;
    ui_attach_element(container, &e);
  }

  f32 deadzone = 0.01f;
  {
    Element e = ui_text("volume");
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_text("lfo");
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider(&ins->volume, SLIDER_FLOAT, RANGE_FLOAT(0.0f, 1.0f));
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(50, 0);
    e.data.slider.deadzone = deadzone;
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider(&w->lfo_target, SLIDER_FLOAT, RANGE_FLOAT(-50.f, 50.0f));
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(50, 0);
    e.data.slider.deadzone = deadzone;
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_text("frequency");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider(&w->freq_target, SLIDER_FLOAT, RANGE_FLOAT(0.0f, 440.0f));
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(50, 0);
    e.data.slider.deadzone = deadzone;
    ui_attach_element(container, &e);
  }
}

void waveshaper_update(Instrument* ins, struct Mix* mix) {
  Waveshaper* w = (Waveshaper*)ins->userdata;
  arena_reset(&w->arena);
  stb_snprintf(w->text, MAX_TEXT_SIZE, "freq: %g\nlfo: %g\nvolume: %g\nlatency: %g ms\naudio_latency: %g ms", w->freq, w->lfo, ins->volume, 1000 * ins->latency, 1000 * ins->audio_latency);

  if (IsKeyPressed(KEY_W)) {
    w->freq_target += 1;
  }
  if (IsKeyPressed(KEY_S)) {
    w->freq_target -= 1;
  }
  if (IsKeyPressed(KEY_E)) {
    w->lfo_target += 1;
  }
  if (IsKeyPressed(KEY_D)) {
    w->lfo_target -= 1;
  }
  if (IsKeyPressed(KEY_SPACE)) {
    w->mute = !w->mute;
  }
  if (IsKeyPressed(KEY_Q)) {
    w->tick = 0;
  }
  if (IsKeyPressed(KEY_H)) {
    w->render = !w->render;
  }
}

void waveshaper_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt) {
  Waveshaper* w = (Waveshaper*)ins->userdata;
  const i32 sample_rate = audio->sample_rate;
  const i32 channel_count = audio->channel_count;
  f32 volume = ins->volume;
  if (w->mute) {
    volume = 0.0f;
  }

  if (w->reshape) {
    for (size_t i = 0; i < ins->frames; i += 2) {
      ins->buffer[i] = volume * sinf(
        (w->tick * PI32 * channel_count * (w->freq + sinf((w->tick * w->lfo * PI32) / (f32)sample_rate)))
        / (f32)sample_rate
      );
      ins->buffer[i + 1] = volume * sinf(
        (w->tick * PI32 * channel_count * (w->freq + sinf((w->tick * w->lfo * PI32) / (f32)sample_rate)))
        / (f32)sample_rate
      );
      w->tick += 2;
      w->freq = lerp_f32(w->freq, w->freq_target, dt * 2.0f);
      w->lfo = lerp_f32(w->lfo, w->lfo_target, dt * 2.0f);
    }
  }
}

void waveshaper_free(struct Instrument* ins) {
  Waveshaper* w = (Waveshaper*)ins->userdata;
  arena_free(&w->arena);
}

#undef ARENA_SIZE
#undef MAX_TEXT_SIZE
