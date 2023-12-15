// wave_shaper.c

#define ARENA_SIZE 1024 * 2
#define MAX_TEXT_SIZE 512

static void waveshaper_onrender(Element* e);
static void waveshaper_reset_onclick(Element* e);

void waveshaper_onrender(Element* e) {
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

  static Color color_map[2] = {
    COLOR_RGB(100, 250, 100),
    COLOR_RGB(20, 100, 30),
  };

  for (i32 i = 0; i < (i32)ins->frames && i < width; ++i) {
    DrawLine(
      x + i,               // x1
      y,                   // y1
      x + i,               // x2
      y + (height * ins->buffer[i]), // y2
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
    e.background_color = lerpcolor(UI_BACKGROUND_COLOR, COLOR_RGB(0, 0, 0), 0.1f);
    e.onrender = waveshaper_onrender;
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_toggle_ex(&w->mute, "mute");
    e.box = BOX(0, 0, 0, 54);
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_toggle_ex(&w->reshape, "reshape");
    e.box = BOX(0, 0, 0, 54);
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_toggle_ex(&w->render, "render");
    e.box = BOX(0, 0, 0, 54);
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_button("reset");
    e.box = BOX(0, 0, 0, 54);
    e.sizing = SIZING_PERCENT(50, 0);
    e.onclick = waveshaper_reset_onclick;
    e.userdata = ins;
    ui_attach_element(container, &e);
  }
}

void waveshaper_update(Instrument* ins, struct Mix* mix) {
  Waveshaper* w = (Waveshaper*)ins->userdata;
  arena_reset(&w->arena);
  stb_snprintf(w->text, MAX_TEXT_SIZE, "freq: %g\nfreq_target: %g\nlfo: %g\nlfo_target: %g\nvolume: %g\nrender: %s\nlatency: %g ms\naudio_latency: %g ms", w->freq, w->freq_target, w->lfo, w->lfo_target, ins->volume, bool_str[w->render == true], 1000 * ins->latency, 1000 * ins->audio_latency);

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
        (w->tick * PI32 * channel_count * (w->freq + cosf((w->tick * w->lfo * PI32) / (f32)sample_rate)))
        / (f32)sample_rate
      );

      w->tick += 2;
      w->freq = lerpf32(w->freq, w->freq_target, dt * 2.0f);
      w->lfo = lerpf32(w->lfo, w->lfo_target, dt * 2.0f);
    }
  }
}

void waveshaper_free(struct Instrument* ins) {
  Waveshaper* w = (Waveshaper*)ins->userdata;
  arena_free(&w->arena);
}

#undef ARENA_SIZE
#undef MAX_TEXT_SIZE
