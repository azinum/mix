// wave_shaper.c

#define ARENA_SIZE 1024
#define INFO_TEXT_SIZE 256
#define LFO_CONNECTION_STR_SIZE 64

// #define EXPERIMENTAL

static Color color_connected = COLOR_RGB(40, 140, 40);
static Color color_disconnected = COLOR_RGB(120, 40, 40);

static void waveshaper_canvas_onrender(Element* e);
static void waveshaper_reset_onclick(Element* e);
static void waveshaper_default(Waveshaper* w);
static void waveshaper_bind_lfo(Element* e, Element* target);
static void waveshaper_update_lfo(Element* e);
static bool waveshaper_connection_filter(Element* e, Element* target);
static void waveshaper_drumpad_init(Drumpad* d);
static void waveshaper_update_drumpad(Element* e);

static void waveshaper_drumpad_event0(Waveshaper* w);
static void waveshaper_drumpad_event1(Waveshaper* w);
static void waveshaper_drumpad_event2(Waveshaper* w);
static void waveshaper_drumpad_event3(Waveshaper* w);

static void waveshaper_drumpad_process0(Audio_engine* audio, Instrument* ins, f32* buffer, size_t samples);
static void waveshaper_drumpad_process1(Audio_engine* audio, Instrument* ins, f32* buffer, size_t samples);
static void waveshaper_drumpad_process2(Audio_engine* audio, Instrument* ins, f32* buffer, size_t samples);
static void waveshaper_drumpad_process3(Audio_engine* audio, Instrument* ins, f32* buffer, size_t samples);

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
    lerp_color(COLOR_RGB(100, 225, 100), warmer_color(UI_BUTTON_COLOR, 40), 0.4f),
    lerp_color(COLOR_RGB(100, 225, 100), warmer_color(UI_BUTTON_COLOR, 30), 0.5f),
  };

  for (size_t i = 0; i < ins->samples; ++i) {
    f32 sample = CLAMP(ins->buffer[i], -1.0f, 1.0f);
    i32 x_pos = x + ((f32)i/ins->samples) * width;
    DrawLine(
      x_pos,               // x1
      y,                   // y1
      x_pos,               // x2
      y + (height/2 * sample), // y2
      color_map[(i % 2) == 0]
    );
  }
  ins->latency += TIMER_END();
}

void waveshaper_reset_onclick(Element* e) {
  Instrument* ins = (Instrument*)e->userdata;
  Waveshaper* w = (Waveshaper*)ins->userdata;
  waveshaper_default(w);
}

void waveshaper_default(Waveshaper* w) {
  w->tick             = 0;
  w->volume_target    = 0.1f;
  w->freq             = 55;
  w->freq_target      = 55;
  w->freq_mod         = 0;
  w->freq_mod_target  = 0;
  w->interp_speed     = 4.0f;
  w->freeze           = false;
  w->mute             = false;
  w->speed            = 2;
  w->flipflop         = false;
  w->distortion       = false;
  w->render           = true;
  w->gain             = 1.0f;
  w->lfo = (Lfo) {
    .lfo_target = NULL,
    .lfo = 0,
    .amplitude = 0.5f,
    .hz = 2.0f,
    .tick = 0,
    .connection_name = LFO_NO_CONNECTION,
  };
  waveshaper_drumpad_init(&w->drumpad);
}

void waveshaper_bind_lfo(Element* e, Element* target) {
  Instrument* ins = (Instrument*)e->userdata;
  Waveshaper* w = (Waveshaper*)ins->userdata;
  if (target->type == ELEMENT_SLIDER) {
    if (target->name != NULL) {
      w->lfo.connection_name = target->name;
    }
    switch (target->data.slider.type) {
      case VALUE_TYPE_FLOAT: {
        f32* binding = target->data.slider.v.f;
        w->lfo.lfo_target = binding;
        return;
      }
      default:
        break;
    }
  }
  w->lfo.lfo_target = NULL;
  w->lfo.connection_name = LFO_NO_CONNECTION;
}

void waveshaper_update_lfo(Element* e) {
  Instrument* ins = (Instrument*)e->userdata;
  Waveshaper* w = (Waveshaper*)ins->userdata;
  if (w->lfo.lfo_target) {
    e->background_color = color_connected;
    return;
  }
  e->background_color = color_disconnected;
}

bool waveshaper_connection_filter(Element* e, Element* target) {
  (void)e;
  if (target->type == ELEMENT_SLIDER) {
    return target->data.slider.v.f != NULL;
  }
  return false;
}

void waveshaper_drumpad_init(Drumpad* d) {
  memset(d->pad, 0, sizeof(d->pad));
  d->event[0] = waveshaper_drumpad_event0;
  d->event[1] = waveshaper_drumpad_event1;
  d->event[2] = waveshaper_drumpad_event2;
  d->event[3] = waveshaper_drumpad_event3;
  d->process[0] = waveshaper_drumpad_process0;
  d->process[1] = waveshaper_drumpad_process1;
  d->process[2] = waveshaper_drumpad_process2;
  d->process[3] = waveshaper_drumpad_process3;
  memset(d->sample_index, 0, sizeof(d->sample_index));
  d->index = 0;
}

void waveshaper_update_drumpad(Element* e) {
  Instrument* ins = (Instrument*)e->userdata;
  Waveshaper* w = (Waveshaper*)ins->userdata;
  Drumpad* d = &w->drumpad;
  if ((i32)d->index == e->v.i) {
    e->border_color = UI_FOCUS_COLOR;
    return;
  }
  e->border_color = UI_BORDER_COLOR;
}

void waveshaper_drumpad_event0(Waveshaper* w) {
  w->drumpad.sample_index[0] = 0;
  static f32 speed = 0.1f;
  const f32 min_value = 0.0f;
  const f32 max_value = 1.0f;
  w->freq_mod_target += speed;
  if ((w->freq_mod_target <= min_value) || (w->freq_mod_target >= max_value)) {
    speed = -speed;
    w->freq_mod_target = CLAMP(w->freq_mod_target, min_value, max_value);
  }
}

void waveshaper_drumpad_event1(Waveshaper* w) {
  w->drumpad.sample_index[1] = 0;
}

void waveshaper_drumpad_event2(Waveshaper* w) {
  w->drumpad.sample_index[2] = 0;
}

void waveshaper_drumpad_event3(Waveshaper* w) {
  w->drumpad.sample_index[3] = 0;
}

void waveshaper_drumpad_process0(Audio_engine* audio, Instrument* ins, f32* buffer, size_t samples) {
  for (size_t i = 0; i < samples; ++i) {
  }
}

void waveshaper_drumpad_process1(Audio_engine* audio, Instrument* ins, f32* buffer, size_t samples) {
  Waveshaper* w = (Waveshaper*)ins->userdata;
  size_t* sample_index = &w->drumpad.sample_index[1];
  for (size_t i = 0; i < samples; ++i, *sample_index += 1) {
    if (*sample_index >= LENGTH(hihat)) {
      return;
    }
    buffer[i] += hihat[*sample_index % LENGTH(hihat)];
  }
}

void waveshaper_drumpad_process2(Audio_engine* audio, Instrument* ins, f32* buffer, size_t samples) {
  Waveshaper* w = (Waveshaper*)ins->userdata;
  size_t* sample_index = &w->drumpad.sample_index[2];
  for (size_t i = 0; i < samples; ++i, *sample_index += 1) {
    if (*sample_index >= LENGTH(snare)) {
      return;
    }
    buffer[i] += snare[*sample_index % LENGTH(snare)];
  }
}

void waveshaper_drumpad_process3(Audio_engine* audio, Instrument* ins, f32* buffer, size_t samples) {
  Waveshaper* w = (Waveshaper*)ins->userdata;
  size_t* sample_index = &w->drumpad.sample_index[3];
  for (size_t i = 0; i < samples; ++i, *sample_index += 1) {
    if (*sample_index >= LENGTH(bassdrum)) {
      return;
    }
    buffer[i] += bassdrum[*sample_index % LENGTH(bassdrum)];
  }
}

void waveshaper_init(Instrument* ins) {
  Waveshaper* w = memory_alloc(sizeof(Waveshaper));
  ASSERT(w != NULL);

  ins->userdata = w;
  *w = (Waveshaper) {
    .arena = arena_new(ARENA_SIZE),
    .text = NULL,
  };
  w->text = arena_alloc(&w->arena, INFO_TEXT_SIZE);
  w->lfo_connection = arena_alloc(&w->arena, LFO_CONNECTION_STR_SIZE);
  waveshaper_default(w);
}

void waveshaper_ui_new(Instrument* ins, Element* container) {
  ui_set_slider_deadzone(0.01f);
  ui_set_connection_filter(waveshaper_connection_filter);
  Waveshaper* w = (Waveshaper*)ins->userdata;
  {
    Element e = ui_text(w->text);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_canvas(true);
    e.box.h = 84;
    e.sizing = SIZING_PERCENT(100, 0);
    e.userdata = ins;
    e.background_color = lerp_color(UI_BACKGROUND_COLOR, COLOR_RGB(0, 0, 0), 0.1f);
    e.onrender = waveshaper_canvas_onrender;
    ui_attach_element(container, &e);
  }
  Element line_break = ui_line_break(FONT_SIZE);

  const i32 button_height = 48;
  const i32 slider_height = 38;
  {
    Element e = ui_toggle_ex(&w->mute, "mute");
    e.box = BOX(0, 0, 0, button_height);
    e.sizing = SIZING_PERCENT(50, 0);
    e.tooltip = "mute (SPACEBAR)";
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_toggle_ex(&w->freeze, "freeze");
    e.box = BOX(0, 0, 0, button_height);
    e.sizing = SIZING_PERCENT(50, 0);
    e.tooltip = "freeze the waveform (F)";
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_toggle_ex(&w->flipflop, "flipflop");
    e.box = BOX(0, 0, 0, button_height);
    e.sizing = SIZING_PERCENT(50, 0);
    e.tooltip = "flip the counting direction of the internal clock";
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_toggle_ex(&w->distortion, "distortion");
    e.box = BOX(0, 0, 0, button_height);
    e.sizing = SIZING_PERCENT(50, 0);
    e.tooltip = "distort the audio signal";
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_button("reset");
    e.box = BOX(0, 0, 0, button_height);
    e.sizing = SIZING_PERCENT(100, 0);
    e.background_color = warmer_color(e.background_color, 80);
    e.onclick = waveshaper_reset_onclick;
    e.userdata = ins;
    e.tooltip = "reset instrument parameters (Q)";
    ui_attach_element(container, &e);
  }

  ui_attach_element(container, &line_break);

  {
    Element e = ui_text("volume");
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_text("frequency modulation");
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider(&w->volume_target, VALUE_TYPE_FLOAT, RANGE_FLOAT(0.0f, 1.0f));
    e.name = "volume";
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider(&w->freq_mod_target, VALUE_TYPE_FLOAT, RANGE_FLOAT(0, 25.0f));
    e.name = "frequency modulation";
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(50, 0);
    e.userdata = ins;
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_text("frequency");
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_text("interpolation speed");
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider(&w->freq_target, VALUE_TYPE_FLOAT, RANGE_FLOAT(0.0f, 440.0f));
    e.name = "frequency";
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider(&w->interp_speed, VALUE_TYPE_FLOAT, RANGE_FLOAT(0.05f, 20.0f));
    e.box = BOX(0, 0, 0, slider_height);
    e.name = "interpolation speed";
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }

  {
    Element e = ui_text("speed");
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_text("gain");
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider(&w->speed, VALUE_TYPE_INTEGER, RANGE(1, 12));
    e.name = "speed";
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider(&w->gain, VALUE_TYPE_FLOAT, RANGE_FLOAT(0.0f, 5.0f));
    e.name = "gain";
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }

  ui_attach_element(container, &line_break);

  {
    Element e = ui_text("DRUMPAD");
    ui_attach_element(container, &e);
  }
  Element* grid = NULL;
  {
    Element e = ui_grid(DRUMPAD_COLS, true);
    e.box = BOX(0, 0, 0, DRUMPAD_ROWS * button_height);
    e.sizing = SIZING_PERCENT(100, 0);
    e.padding = UI_BORDER_THICKNESS + 1;
    grid = ui_attach_element(container, &e);
  }
  for (size_t y = 0; y < DRUMPAD_ROWS; ++y) {
    for (size_t x = 0; x < DRUMPAD_COLS; ++x) {
      i32* value = &w->drumpad.pad[x][y];
      Element e = ui_toggle(value);
      e.userdata = ins;
      e.v.i = x;
      if (!((x + 1) % 2)) {
        e.background_color = lerp_color(e.background_color, COLOR_RGB(255, 255, 255), 0.05f);
      }
      else {
        e.background_color = lerp_color(e.background_color, COLOR_RGB(0, 0, 0), 0.05f);
      }
      e.onupdate = waveshaper_update_drumpad;
      ui_attach_element(grid, &e);
    }
  }

  ui_attach_element(container, &line_break);

  {
    Element e = ui_text("LFO");
    ui_attach_element(container, &e);
  }

  Element* lfo_container = NULL;
  {
    Element e = ui_container_ex(NULL, false);
    e.scissor = false;
    e.background = true;
    e.placement = PLACEMENT_BLOCK;
    e.border = true;
    e.background = true;
    e.background_color = lerp_color(e.background_color, COLOR_RGB(255, 255, 255), 0.05f);
    e.sizing = SIZING_PERCENT(100, 0);
    e.box.h = 4 * button_height;
    lfo_container = ui_attach_element(container, &e);
  }
  {
    Element e = ui_text("amplitude");
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(lfo_container, &e);
  }
  {
    Element e = ui_text("hz");
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(lfo_container, &e);
  }
  {
    Element e = ui_slider(&w->lfo.amplitude, VALUE_TYPE_FLOAT, RANGE_FLOAT(0.0f, 1.0f));
    e.name = "LFO amplitude";
    e.box.h = slider_height;
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(lfo_container, &e);
  }
  {
    Element e = ui_slider(&w->lfo.hz, VALUE_TYPE_FLOAT, RANGE_FLOAT(0.0f, 25.0f));
    e.name = "LFO hz";
    e.box.h = slider_height;
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(lfo_container, &e);
  }
  {
    Element e = ui_text("offset");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(lfo_container, &e);
  }
  {
    Element e = ui_slider(&w->lfo.offset, VALUE_TYPE_FLOAT, RANGE_FLOAT(-1.0f, 1.0f));
    e.name = "LFO offset";
    e.box.h = slider_height;
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(lfo_container, &e);
  }
  {
    Element e = ui_none();
    e.render = true;
    e.background = true;
    e.background_color = color_disconnected;
    e.border = true;
    e.border_color = UI_BORDER_COLOR;
    e.roundness = UI_BUTTON_ROUNDNESS;
    e.box.w = e.box.h = FONT_SIZE;
    e.userdata = ins;
    e.onconnect = waveshaper_bind_lfo;
    e.onupdate = waveshaper_update_lfo;
    e.tooltip = "hold ctrl+left mouse click to connect the LFO\nto one of the range sliders";
    ui_attach_element(lfo_container, &e);
  }
  {
    Element e = ui_text(w->lfo_connection);
    ui_attach_element(lfo_container, &e);
  }
}

void waveshaper_update(Instrument* ins, struct Mix* mix) {
  (void)mix;
  Waveshaper* w = (Waveshaper*)ins->userdata;
  arena_reset(&w->arena);
  w->text = arena_alloc(&w->arena, INFO_TEXT_SIZE);
  stb_snprintf(w->text, INFO_TEXT_SIZE, "freq: %g\nfreq_mod: %g\ninterp_speed: %g\nvolume: %g\nlatency: %g ms\naudio_latency: %g ms", w->freq, w->freq_mod, w->interp_speed, ins->volume, 1000 * ins->latency, 1000 * ins->audio_latency);

  w->lfo_connection = arena_alloc(&w->arena, LFO_CONNECTION_STR_SIZE);
  stb_snprintf(w->lfo_connection, LFO_CONNECTION_STR_SIZE, "connected to: %s", w->lfo.connection_name);

  size_t prev_index = w->drumpad.index;
  w->drumpad.index = mix->timed_tick;
  w->drumpad.index = w->drumpad.index % DRUMPAD_COLS;

  if (prev_index != w->drumpad.index) {
    for (size_t y = 0; y < DRUMPAD_ROWS; ++y) {
      size_t x = w->drumpad.index;
      if (w->drumpad.pad[x][y]) {
        w->drumpad.event[y](w);
      }
    }
  }

  if (ui_no_input()) {
    if (IsKeyPressed(KEY_W)) {
      w->freq_target += 1;
    }
    if (IsKeyPressed(KEY_S)) {
      w->freq_target -= 1;
    }
    if (IsKeyPressed(KEY_E)) {
      w->freq_mod_target += 1;
    }
    if (IsKeyPressed(KEY_D)) {
      w->freq_mod_target -= 1;
    }
    if (IsKeyPressed(KEY_F)) {
      w->freeze = !w->freeze;
    }
    if (IsKeyPressed(KEY_SPACE)) {
      w->mute = !w->mute;
    }
    if (IsKeyPressed(KEY_Q)) {
      waveshaper_default(w);
    }
    if (IsKeyPressed(KEY_H)) {
      w->render = !w->render;
    }
  }
}

void waveshaper_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt) {
  (void)mix;

  Waveshaper* w = (Waveshaper*)ins->userdata;
  const i32 sample_rate = audio->sample_rate;
  const i32 channel_count = audio->channel_count;
  f32 volume = ins->volume;
  if (w->mute) {
    volume = 0.0f;
  }

  if (!w->freeze) {
    for (size_t i = 0; i < ins->samples; ++i) {
      ins->buffer[i] = 0.0f;
    }

    for (size_t y = 0; y < DRUMPAD_ROWS; ++y) {
      size_t x = w->drumpad.index;
      if (w->drumpad.pad[x][y]) {
        w->drumpad.process[y](audio, ins, ins->buffer, ins->samples);
      }
    }

    for (size_t i = 0; i < ins->samples; i += 2) {
      w->lfo.lfo = w->lfo.offset + w->lfo.amplitude * sinf((w->lfo.hz * w->lfo.tick * 2 * PI32) / (f32)sample_rate);
      w->lfo.tick += 1;
      if (w->lfo.lfo_target != NULL) {
        *w->lfo.lfo_target = w->lfo.lfo;
      }

      ins->buffer[i] += volume * sinf(
        (w->tick * PI32 * channel_count * (w->freq + sinf((w->tick * w->freq_mod * PI32) / (f32)sample_rate)))
        / (f32)sample_rate
      );
      ins->buffer[i + 1] += volume * cosf(
        (w->tick * PI32 * channel_count * (w->freq + cosf((w->tick * w->freq_mod * PI32) / (f32)sample_rate)))
        / (f32)sample_rate
      );
      w->tick += w->speed;
      w->freq = lerp_f32(w->freq, w->freq_target, dt * w->interp_speed);
      w->freq_mod = lerp_f32(w->freq_mod, w->freq_mod_target, dt * w->interp_speed);
      ins->volume = lerp_f32(ins->volume, w->volume_target, dt * w->interp_speed);
    }
    if (w->distortion) {
#ifdef EXPERIMENTAL
      static i32 tmp_index = 0;
      static f32 tmp_buffer[256] = {0};
      for (size_t i = 0; i < LENGTH(tmp_buffer) && i < ins->samples; ++i) {
        tmp_buffer[i] = ins->buffer[i];
      }
      for (size_t i = 0; i < ins->samples; ++i) {
        tmp_index = (tmp_index + 1) % LENGTH(tmp_buffer);
        ins->buffer[i] = 0.5f * ins->buffer[i] + 0.5f * tmp_buffer[(tmp_index & 6) % LENGTH(tmp_buffer)];
      }
#endif
      for (size_t i = 0; i < ins->samples; ++i) {
        ins->buffer[i] *= 8.0f;
        ins->buffer[i] = CLAMP(ins->buffer[i], -1.0f, 1.0f);
        ins->buffer[i] *= 1/4.0f;
      }
    }
    if (w->gain >= 0.0f) {
      for (size_t i = 0; i < ins->samples; ++i) {
        ins->buffer[i] *= w->gain;
      }
    }
  }
  if (w->flipflop) {
    w->speed = -w->speed;
  }
}

void waveshaper_free(struct Instrument* ins) {
  Waveshaper* w = (Waveshaper*)ins->userdata;
  arena_free(&w->arena);
}

#undef ARENA_SIZE
#undef INFO_TEXT_SIZE
