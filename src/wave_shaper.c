// wave_shaper.c

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
    .reshape = true,
  };
  if (!w.buffer) {
    w.size = 0;
  }
  return w;
}

void waveshaper_update(Mix* m, Waveshaper* w) {
  (void)m;
  TIMER_START();
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
  if (IsKeyDown(KEY_Q)) {
    w->tick = 0;
  }
  w->latency += TIMER_END();
}

void waveshaper_process(Mix* m, Waveshaper* w, f32 dt) {
  (void)m;
  TIMER_START();
  Audio_engine* e = &audio_engine;
  const i32 sample_rate = e->sample_rate;
  const f32 amp = 0.15f;
  const i32 channel_count = e->channel_count;

  if (w->reshape) {
    for (size_t i = 0; i < w->size; i += 2) {
      w->buffer[i] = amp * sinf(
        (w->tick * PI32 * channel_count * (w->freq + sinf((w->tick * w->lfo * PI32) / (f32)sample_rate)))
        / (f32)sample_rate
      );
      w->buffer[i + 1] = amp * sinf(
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

void waveshaper_render(Mix* m, Waveshaper* const w) {
  (void)m;
  TIMER_START();

  i32 x = 260;
  i32 y = 160;
  i32 width = w->size/2;
  i32 height = 80;
  Color color_map[2] = {
    COLOR_RGB(100, 250, 100),
    COLOR_RGB(20, 100, 30),
  };
  DrawRectangle(x, y-height, width, height*2, COLOR_RGB(70, 70, 75));
  for (i32 i = 0; i < width; ++i) {
    DrawLine(
      x + i,               // x1
      y,                   // y1
      x + i,               // x2
      y + (height*w->buffer[i]), // y2
      color_map[(i%4)==0]
    );
  }
  DrawRectangleLines(x, y-height, width, height*2, COLOR_RGB(225, 225, 225));
  {
    static char text[256] = {0};
    stb_snprintf(text, sizeof(text), "freq: %g\nfreq_target: %g\nreshape: %s\nlatency: %g ms\naudio_latency: %g ms\nlfo: %g\nlfo_target: %g", w->freq, w->freq_target, bool_str[w->reshape == true], 1000 * w->latency, 1000 * w->audio_latency, w->lfo, w->lfo_target);
    DrawText(text, x, y+height + 20, FONT_SIZE_SMALLEST, COLOR_RGB(255, 255, 255));
  }
  w->latency += TIMER_END();
}

void waveshaper_free(Waveshaper* w) {
  memory_free(w->buffer);
  w->buffer = NULL;
  w->size = 0;
}
