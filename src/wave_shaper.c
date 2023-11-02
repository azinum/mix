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

f32 filter(f32 cutoff, f32 sample_rate) {
  f32 rc = 1.0f / (cutoff * 2 * PI32);
  f32 dt = 1.0f / sample_rate;
  f32 alpha = dt / (rc + dt);
  return alpha;
}

void waveshaper_process(Mix* m, Waveshaper* w, f32 dt) {
  TIMER_START();
  Audio_engine* e = &audio_engine;
  const i32 sample_rate = e->sample_rate;
  const f32 amp = 0.2f;

  if (w->reshape) {
    for (size_t i = 0; i < w->size; i += 2) {
      w->buffer[i] = amp * sinf(
        (w->tick * PI32 * 2 * (w->freq + sinf((w->tick * w->lfo * PI32) / (f32)sample_rate)))
        / (f32)sample_rate
      );
      w->buffer[i + 1] = amp * sinf(
        (w->tick * PI32 * 2 * (w->freq + cosf((w->tick * w->lfo * PI32) / (f32)sample_rate)))
        / (f32)sample_rate
      );

      w->tick += 2;
      w->freq = lerpf32(w->freq, w->freq_target, dt * 0.5f);
      w->lfo = lerpf32(w->lfo, w->lfo_target, dt * 0.5f);
    }
  }

  w->audio_latency = TIMER_END();
}

void waveshaper_render(Mix* m, Waveshaper* const w) {
  TIMER_START();

  i32 x = 40;
  i32 y = 160;
  i32 width = w->size/2;
  i32 height = 80;
  Color color_map[] = {
    COLOR_RGB(100, 200, 100),
    COLOR_RGB(20, 70, 30),
  };
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
