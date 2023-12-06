// wave_shaper.c

#define ARENA_SIZE 1024 * 2
#define MAX_TEXT_SIZE 512

static void waveshaper_onrender(Element* e);
static void waveshaper_reset_onclick(Element* e);

void waveshaper_onrender(Element* e) {
  TIMER_START();
  Waveshaper* w = (Waveshaper*)e->userdata;
  ASSERT(w != NULL);
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

  for (i32 i = 0; i < (i32)w->size && i < width; ++i) {
    DrawLine(
      x + i,               // x1
      y,                   // y1
      x + i,               // x2
      y + (height*w->buffer[i]), // y2
      color_map[(i % 2) == 0]
    );
  }

  w->latency += TIMER_END();
}

void waveshaper_reset_onclick(Element* e) {
  Waveshaper* w = (Waveshaper*)e->userdata;
  w->tick = 0;
  w->freq_target = 55;
  w->lfo_target = 0;
}

Waveshaper waveshaper_new(size_t size) {
  Waveshaper w = (Waveshaper) {
    .buffer = memory_calloc(size, sizeof(f32)),
    .size = size,
    .tick = 0,
    .freq = 55,
    .freq_target = 55,
    .latency = 0,
    .audio_latency = 0,
    .lfo = 0.0f,
    .lfo_target = 0.0f,
    .volume = 0.1f,
    .reshape = true,
    .mute = false,
    .arena = arena_new(ARENA_SIZE),
    .text = NULL,
    .render = true,
  };
  w.text = arena_alloc(&w.arena, MAX_TEXT_SIZE);
  if (!w.buffer) {
    w.size = 0;
  }
  return w;
}

Element waveshaper_ui_new(Waveshaper* w) {
  Element container = ui_container("waveshaper");
  container.border = true;
  container.scissor = true;
  container.placement = PLACEMENT_BLOCK;
  container.background = true;
  {
    Element e = ui_text(w->text);
    e.sizing = (Sizing) {
      .mode = SIZE_MODE_PERCENT,
      .x = 100,
      .y = 0,
    };
    ui_attach_element(&container, &e);
  }
  {
    Element e = ui_toggle_ex(&w->mute, "mute");
    e.box = BOX(0, 0, 0, 54);
    e.sizing = (Sizing) {
      .mode = SIZE_MODE_PERCENT,
        .x = 50,
        .y = 0,
    };
    ui_attach_element(&container, &e);
  }
  {
    Element e = ui_toggle_ex(&w->reshape, "reshape");
    e.box = BOX(0, 0, 0, 54);
    e.sizing = (Sizing) {
      .mode = SIZE_MODE_PERCENT,
        .x = 50,
        .y = 0,
    };
    ui_attach_element(&container, &e);
  }
  {
    Element e = ui_toggle_ex(&w->render, "render");
    e.box = BOX(0, 0, 0, 54);
    e.sizing = (Sizing) {
      .mode = SIZE_MODE_PERCENT,
      .x = 50,
      .y = 0,
    };
    ui_attach_element(&container, &e);
  }
  {
    Element e = ui_button("reset");
    e.box = BOX(0, 0, 0, 54);
    e.sizing = (Sizing) {
      .mode = SIZE_MODE_PERCENT,
      .x = 50,
      .y = 0,
    };
    e.onclick = waveshaper_reset_onclick;
    e.userdata = w;
    ui_attach_element(&container, &e);
  }
  {
    Element e = ui_canvas(true);
    e.box = BOX(0, 0, 0, 0);
    e.sizing = (Sizing) {
      .mode = SIZE_MODE_PERCENT,
      .x = 100,
      .y = 30,
    };
    e.userdata = w;
    e.background = false;
    e.onrender = waveshaper_onrender;
    ui_attach_element(&container, &e);
  }
  return container;
}

void waveshaper_update(Mix* m, Waveshaper* w) {
  (void)m;
  TIMER_START();

  arena_reset(&w->arena);
  w->text = arena_alloc(&w->arena, MAX_TEXT_SIZE);
  stb_snprintf(w->text, MAX_TEXT_SIZE, "freq: %g\nfreq_target: %g\nreshape: %s\nlfo: %g\nlfo_target: %g\nvolume: %g\nlatency: %g ms\naudio_latency: %g ms\nrender: %s", w->freq, w->freq_target, bool_str[w->reshape == true], w->lfo, w->lfo_target, w->volume, 1000 * w->latency, 1000 * w->audio_latency, bool_str[w->render == true]);

  w->latency = 0;

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
    w->reshape = !w->reshape;
  }
  if (IsKeyPressed(KEY_Q)) {
    w->tick = 0;
  }
  if (IsKeyPressed(KEY_H)) {
    w->render = !w->render;
  }
  w->latency += TIMER_END();
}

void waveshaper_process(Mix* m, Waveshaper* w, f32 dt) {
  (void)m;
  TIMER_START();
  Audio_engine* e = &audio_engine;
  const i32 sample_rate = e->sample_rate;
  const i32 channel_count = e->channel_count;
  f32 volume = w->volume;
  if (w->mute) {
    volume = 0;
  }

  if (w->reshape) {
    for (size_t i = 0; i < w->size; i += 2) {
      w->buffer[i] = volume * sinf(
        (w->tick * PI32 * channel_count * (w->freq + sinf((w->tick * w->lfo * PI32) / (f32)sample_rate)))
        / (f32)sample_rate
      );
      w->buffer[i + 1] = volume * sinf(
        (w->tick * PI32 * channel_count * (w->freq + cosf((w->tick * w->lfo * PI32) / (f32)sample_rate)))
        / (f32)sample_rate
      );

      w->tick += 2;
#if 1
      w->freq = lerpf32(w->freq, w->freq_target, dt * 2.0f);
      w->lfo = lerpf32(w->lfo, w->lfo_target, dt * 2.0f);
#else
      w->freq = w->freq_target;
      w->lfo = w->lfo_target;
#endif
    }
  }
  w->audio_latency = TIMER_END();
}

void waveshaper_free(Waveshaper* w) {
  memory_free(w->buffer);
  arena_free(&w->arena);
  w->buffer = NULL;
  w->size = 0;
}
