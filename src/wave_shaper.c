// wave_shaper.c

#define ARENA_SIZE 1024
#define INFO_TEXT_SIZE 256
#define LFO_CONNECTION_STR_SIZE 64

// #define EXPERIMENTAL

typedef struct Waveshaper {
  f32 tick;
  size_t mod_tick;
  f32 volume_target;
  f32 freq;
  f32 freq_target;
  f32 freq_mod;
  f32 freq_mod_target;
  f32 interp_speed;
  i32 freeze;
  i32 mute;
  f32 speed;
  i32 flipflop;
  i32 distortion;
  f32 gain;
  i32 left_offset;
  i32 right_offset;
  f32 mod_table[MOD_TABLE_LENGTH];
  u32 mod_index;
  i32 mod_freq_mod;
  i32 mod_freq;
  f32 mod_freq_mod_scale;
  f32 mod_freq_scale;
  Arena arena;
  Lfo lfo;
  char* lfo_connection;
  Drumpad drumpad;
  Audio_source source;
  Audio_source mod_source;
  Ticket source_mutex;
} Waveshaper;

static Color color_connected = COLOR_RGB(40, 140, 40);
static Color color_disconnected = COLOR_RGB(120, 40, 40);

static void waveshaper_load_sample(Waveshaper* w, const char* path, Audio_source* source);
static void waveshaper_render_source(Element* e);
static void waveshaper_render_mod_source(Element* e);
static void waveshaper_hover_source(Element* e);
static void waveshaper_hover_mod_source(Element* e);
static void waveshaper_reset_onclick(Element* e);
static void waveshaper_default(Waveshaper* w);
static void waveshaper_bind_lfo(Element* e, Element* target);
static void waveshaper_update_lfo(Element* e);
static bool waveshaper_connection_filter(Element* e, Element* target);
static void waveshaper_drumpad_init(Drumpad* d);
static void waveshaper_update_drumpad(Element* e);
static void waveshaper_randomize_stepper(Element* e);

static void waveshaper_drumpad_event0(Waveshaper* w);
static void waveshaper_drumpad_event1(Waveshaper* w);
static void waveshaper_drumpad_event2(Waveshaper* w);
static void waveshaper_drumpad_event3(Waveshaper* w);
static void waveshaper_drumpad_event4(Waveshaper* w);

static void waveshaper_drumpad_process0(Audio_engine* audio, Instrument* ins, f32* buffer, size_t samples);
static void waveshaper_drumpad_process1(Audio_engine* audio, Instrument* ins, f32* buffer, size_t samples);
static void waveshaper_drumpad_process2(Audio_engine* audio, Instrument* ins, f32* buffer, size_t samples);
static void waveshaper_drumpad_process3(Audio_engine* audio, Instrument* ins, f32* buffer, size_t samples);
static void waveshaper_drumpad_process4(Audio_engine* audio, Instrument* ins, f32* buffer, size_t samples);

void waveshaper_load_sample(Waveshaper* w, const char* path, Audio_source* source) {
  Audio_source loaded_source = audio_load_audio(path);
  if (loaded_source.buffer != NULL) {
    ticket_mutex_begin(&w->source_mutex);
    audio_unload_audio(source);
    *source = loaded_source;
    ticket_mutex_end(&w->source_mutex);
  }
  else {
    ui_alert("failed to load audio file\n%s", path);
  }
}

void waveshaper_render_source(Element* e) {
  Waveshaper* w = (Waveshaper*)e->userdata;
  Audio_source* source = &w->source;
  if (!source->buffer) {
    return;
  }
  mix_render_curve(source->buffer, source->samples, e->box, COLOR_RGB(130, 190, 100));
}

void waveshaper_render_mod_source(Element* e) {
  Waveshaper* w = (Waveshaper*)e->userdata;
  Audio_source* source = &w->mod_source;
  if (!source->buffer) {
    return;
  }
  mix_render_curve(source->buffer, source->samples, e->box, COLOR_RGB(130, 190, 100));
}

void waveshaper_hover_source(Element* e) {
  Waveshaper* w = (Waveshaper*)e->userdata;
  char* path = NULL;
  if (IsFileDropped()) {
    FilePathList files = LoadDroppedFiles();
    if (files.count > 0) {
      path = files.paths[0];
    }
    waveshaper_load_sample(w, path, &w->source);
    UnloadDroppedFiles(files);
  }
}

void waveshaper_hover_mod_source(Element* e) {
  Waveshaper* w = (Waveshaper*)e->userdata;
  char* path = NULL;
  if (IsFileDropped()) {
    FilePathList files = LoadDroppedFiles();
    if (files.count > 0) {
      path = files.paths[0];
    }
    waveshaper_load_sample(w, path, &w->mod_source);
    UnloadDroppedFiles(files);
  }
}

void waveshaper_reset_onclick(Element* e) {
  Instrument* ins = (Instrument*)e->userdata;
  Waveshaper* w = (Waveshaper*)ins->userdata;
  waveshaper_default(w);
}

void waveshaper_default(Waveshaper* w) {
  w->tick             = 0.0f;
  w->mod_tick         = 0;
  w->volume_target    = 0.1f;
  w->freq             = 55;
  w->freq_target      = 55;
  w->freq_mod         = 0;
  w->freq_mod_target  = 0;
  w->interp_speed     = 100.0f;
  w->freeze           = false;
  w->mute             = false;
  w->speed            = 2.0f;
  w->flipflop         = false;
  w->distortion       = false;
  w->gain             = 1.0f;
  w->left_offset      = 0;
  w->right_offset     = 0;
  const f32 max_freq_mod = 1.0f;
  f32 freq_mod_step = 2 * max_freq_mod / MOD_TABLE_LENGTH;
  f32 freq_mod = freq_mod_step;
  for (u32 i = 0; i < MOD_TABLE_LENGTH; ++i) {
    w->mod_table[i] = freq_mod;
    if (i == MOD_TABLE_LENGTH / 2) {
      freq_mod_step = -freq_mod_step;
    }
    freq_mod += freq_mod_step;
  }
  w->mod_index    = 0;
  w->mod_freq_mod = true;
  w->mod_freq     = false;
  w->mod_freq_mod_scale = 1.0f;
  w->mod_freq_scale = 55.0f;

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
  d->event[4] = waveshaper_drumpad_event4;
  d->process[0] = waveshaper_drumpad_process0;
  d->process[1] = waveshaper_drumpad_process1;
  d->process[2] = waveshaper_drumpad_process2;
  d->process[3] = waveshaper_drumpad_process3;
  d->process[4] = waveshaper_drumpad_process4;
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

void waveshaper_randomize_stepper(Element* e) {
  Waveshaper* w = (Waveshaper*)e->userdata;
  for (size_t i = 0; i < MOD_TABLE_LENGTH; ++i) {
    w->mod_table[i] = random_f32() * 2;
  }
}

void waveshaper_drumpad_event0(Waveshaper* w) {
  w->drumpad.sample_index[0] = 0;
  if (w->mod_freq_mod) {
    w->freq_mod_target = w->mod_freq_mod_scale * w->mod_table[w->mod_index % MOD_TABLE_LENGTH];
  }
  if (w->mod_freq) {
    w->freq_target = w->mod_freq_scale * w->mod_table[w->mod_index % MOD_TABLE_LENGTH];
  }
  w->mod_index += 1;
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

void waveshaper_drumpad_event4(Waveshaper* w) {
  w->drumpad.sample_index[4] = 0;
  w->freeze = !w->freeze;
}

void waveshaper_drumpad_process0(Audio_engine* audio, Instrument* ins, f32* buffer, size_t samples) {
  (void)audio;
  (void)ins;
  (void)buffer;
  (void)samples;
}

void waveshaper_drumpad_process1(Audio_engine* audio, Instrument* ins, f32* buffer, size_t samples) {
  (void)audio;
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
  (void)audio;
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
  (void)audio;
  Waveshaper* w = (Waveshaper*)ins->userdata;
  size_t* sample_index = &w->drumpad.sample_index[3];
  for (size_t i = 0; i < samples; ++i, *sample_index += 1) {
    if (*sample_index >= LENGTH(kick)) {
      return;
    }
    buffer[i] += kick[*sample_index % LENGTH(kick)];
  }
}

void waveshaper_drumpad_process4(Audio_engine* audio, Instrument* ins, f32* buffer, size_t samples) {
  (void)audio;
  (void)ins;
  (void)buffer;
  (void)samples;
}

void waveshaper_init(Instrument* ins) {
  Waveshaper* w = memory_alloc(sizeof(Waveshaper));
  ASSERT(w != NULL);
  ins->userdata = w;
  MEMORY_TAG("waveshaper: arena");
  *w = (Waveshaper) {
    .arena = arena_new(ARENA_SIZE),
  };
  w->lfo_connection = arena_alloc(&w->arena, LFO_CONNECTION_STR_SIZE);
  waveshaper_default(w);
  w->source = (Audio_source) {
    .buffer = (f32*)&sine[0],
    .samples = LENGTH(sine),
    .channel_count = 2,
    .ready = true,
    .internal = true,
  };
  w->mod_source = (Audio_source) {
    .buffer = (f32*)&sine[0],
    .samples = LENGTH(sine),
    .channel_count = 2,
    .ready = true,
    .internal = true,
  };
  w->source_mutex = ticket_mutex_new();
}

void waveshaper_ui_new(Instrument* ins, Element* container) {
  ui_set_connection_filter(waveshaper_connection_filter);
  Waveshaper* w = (Waveshaper*)ins->userdata;
  Element line_break = ui_line_break(FONT_SIZE / 4);

#ifdef TARGET_ANDROID
  const i32 button_height = 64;
  const i32 small_button_height = 48;
#else
  const i32 button_height = FONT_SIZE * 2;
  const i32 small_button_height = FONT_SIZE * 1.2f;
#endif
  const i32 slider_height = FONT_SIZE * 1.2f;
  const i32 input_height = slider_height;
  {
    Element e = ui_toggle_ex(&w->mute, "mute");
    e.box = BOX(0, 0, 0, button_height);
    e.sizing = SIZING_PERCENT(50, 0);
    e.tooltip = "mute";
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
    e.tooltip = "flip the counting direction of the internal\nclock after processing audio buffer";
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
    Element e = ui_canvas(true);
    ui_set_title(&e, "source");
    e.sizing = (Sizing) {
      .x_mode = SIZE_MODE_PERCENT,
      .y_mode = SIZE_MODE_PIXELS,
      .x      = 50,
      .y      = button_height * 3,
    };
    e.userdata = w;
    e.onrender = waveshaper_render_source;
    e.onhover = waveshaper_hover_source;
    e.tooltip = "drag and drop audio file here";
    ui_attach_element(container, &e);
  }

  {
    Element e = ui_canvas(true);
    ui_set_title(&e, "mod source");
    e.sizing = (Sizing) {
      .x_mode = SIZE_MODE_PERCENT,
      .y_mode = SIZE_MODE_PIXELS,
      .x      = 50,
      .y      = button_height * 3,
    };
    e.userdata = w;
    e.onrender = waveshaper_render_mod_source;
    e.onhover = waveshaper_hover_mod_source;
    e.tooltip = "drag and drop audio file here";
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
    Element e = ui_input_float("volume", &w->volume_target);
    e.box = BOX(0, 0, 0, input_height);
    e.sizing = SIZING_PERCENT(15, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider(&w->volume_target, VALUE_TYPE_FLOAT, RANGE_FLOAT(0.0f, 1.0f));
    e.name = "volume";
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(35, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_input_float("frequency modulation", &w->freq_mod_target);
    e.box = BOX(0, 0, 0, input_height);
    e.sizing = SIZING_PERCENT(15, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider(&w->freq_mod_target, VALUE_TYPE_FLOAT, RANGE_FLOAT(0, 25.0f));
    e.name = "frequency modulation";
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(35, 0);
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
    Element e = ui_input_float("frequency", &w->freq_target);
    e.box = BOX(0, 0, 0, input_height);
    e.sizing = SIZING_PERCENT(15, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider(&w->freq_target, VALUE_TYPE_FLOAT, RANGE_FLOAT(0.0f, 440.0f));
    e.name = "frequency";
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(35, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_input_float("interpolation speed", &w->interp_speed);
    e.box = BOX(0, 0, 0, input_height);
    e.sizing = SIZING_PERCENT(15, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider(&w->interp_speed, VALUE_TYPE_FLOAT, RANGE_FLOAT(0.05f, 200.0f));
    e.box = BOX(0, 0, 0, slider_height);
    e.name = "interpolation speed";
    e.sizing = SIZING_PERCENT(35, 0);
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
    Element e = ui_input_float("speed", &w->speed);
    e.box = BOX(0, 0, 0, input_height);
    e.sizing = SIZING_PERCENT(15, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider_float(&w->speed, 0.001f, 10.0f);
    e.name = "speed";
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(35, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_input_float("gain", &w->gain);
    e.box = BOX(0, 0, 0, input_height);
    e.sizing = SIZING_PERCENT(15, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider_float(&w->gain, 0.0f, 5.0f);
    e.name = "gain";
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(35, 0);
    ui_attach_element(container, &e);
  }

  {
    Element e = ui_text("left offset");
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_text("right offset");
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_input_int("left offset", &w->left_offset);
    e.box = BOX(0, 0, 0, input_height);
    e.sizing = SIZING_PERCENT(15, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider_int(&w->left_offset, 0, 4096);
    e.name = "left offset";
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(35, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_input_int("right offset", &w->right_offset);
    e.box = BOX(0, 0, 0, input_height);
    e.sizing = SIZING_PERCENT(15, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider_int(&w->right_offset, 0, 4096);
    e.name = "right offset";
    e.box = BOX(0, 0, 0, slider_height);
    e.sizing = SIZING_PERCENT(35, 0);
    ui_attach_element(container, &e);
  }

  ui_attach_element(container, &line_break);
  {
    Element e = ui_text_ex("stepper", true);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(container, &e);
  }
  for (size_t i = 0; i < MOD_TABLE_LENGTH; ++i) {
    f32* f = &w->mod_table[i];
    Element e = ui_slider_float(f, 0.0f, 2.0f);
    e.box.w = 20;
    e.box.h = 3 * button_height;
    e.data.slider.slider_type = SLIDER_VERTICAL;
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_line_break(0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_toggle_ex(&w->mod_freq_mod, "freq mod");
    e.sizing = SIZING_PERCENT(20, 0);
    e.box.h = small_button_height;
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_input_float("scale", &w->mod_freq_mod_scale);
    e.box.w = FONT_SIZE * 3;
    e.box.h = small_button_height;
    e.tooltip = "scale the frequency modulation by this value";
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_line_break(0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_toggle_ex(&w->mod_freq, "freq");
    e.sizing = SIZING_PERCENT(20, 0);
    e.box.h = small_button_height;
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_input_float("scale", &w->mod_freq_scale);
    e.box.w = FONT_SIZE * 3;
    e.box.h = small_button_height;
    e.tooltip = "scale the frequency by this value";
    ui_attach_element(container, &e);
  }

  {
    Element e = ui_line_break(0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_button("randomize");
    e.sizing = (Sizing) {
      .x_mode = SIZE_MODE_PERCENT,
      .y_mode = SIZE_MODE_PIXELS,
      .x = 20,
      .y = small_button_height,
    };
    e.userdata = w;
    e.onclick = waveshaper_randomize_stepper;
    ui_attach_element(container, &e);
  }

  ui_attach_element(container, &line_break);

  {
    Element e = ui_text_ex("drumpad", true);
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(container, &e);
  }

  const char* descriptions[DRUMPAD_ROWS] = {
    "stepper",
    "hihat  ",
    "snare  ",
    "kick   ",
    "freeze ",
  };
  const i32 pad_height = (i32)(button_height * 0.75f);
  for (size_t y = 0; y < DRUMPAD_ROWS; ++y) {
    {
      Element e = ui_text((char*)descriptions[y]);
      e.box = BOX(0, 0, 4 * FONT_SIZE, pad_height);
      e.text_color = lerp_color(UI_TEXT_COLOR, invert_color(UI_INTERPOLATION_COLOR), 0.4f);
      ui_attach_element(container, &e);
    }
    for (size_t x = 0; x < DRUMPAD_COLS; ++x) {
      i32* value = &w->drumpad.pad[x][y];
      Element e = ui_toggle(value);
      e.box = BOX(0, 0, pad_height, pad_height);
      e.userdata = ins;
      e.v.i = x;
      if (!((x + 0) % 4)) {
        e.background_color = lerp_color(saturate_color(UI_BUTTON_COLOR, -0.2f), UI_INTERPOLATION_COLOR, 0.1f);
      }
      else {
        e.background_color = UI_BUTTON_COLOR;
      }
      e.onupdate = waveshaper_update_drumpad;
      ui_attach_element(container, &e);
    }
    ui_attach_element_v2(container, ui_line_break(0));
  }

  ui_attach_element(container, &line_break);

  {
    Element e = ui_text_ex("LFO", false);
    ui_attach_element(container, &e);
  }

  Element* lfo_container = NULL;
  {
    Element e = ui_container_ex(NULL, false);
    e.scissor = false;
    e.background = true;
    e.placement = PLACEMENT_BLOCK;
    e.background_color = lerp_color(e.background_color, UI_INTERPOLATION_COLOR, 0.05f);
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
    Element e = ui_text_ex(w->lfo_connection, false);
    ui_attach_element(lfo_container, &e);
  }
}

void waveshaper_update(Instrument* ins, struct Mix* mix) {
  (void)mix;
  Waveshaper* w = (Waveshaper*)ins->userdata;
  arena_reset(&w->arena);

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

  bool mod_key = IsKeyDown(KEY_LEFT_CONTROL);

  if (!ui_input_interacting() && !mod_key) {
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
    if (IsKeyPressed(KEY_Q)) {
      waveshaper_default(w);
    }
  }
}

void waveshaper_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt) {
  (void)mix;

  Waveshaper* w = (Waveshaper*)ins->userdata;
  const i32 sample_rate = audio->sample_rate;
  const i32 channel_count = audio->channel_count;
  const f32 sample_dt = dt / (f32)ins->samples;
  f32 volume = ins->volume;
  if (w->mute) {
    volume = 0.0f;
  }

  if (!w->freeze) {
    for (size_t i = 0; i < ins->samples; ++i) {
      ins->out_buffer[i] = 0.0f;
    }
  }
  for (size_t y = 0; y < DRUMPAD_ROWS; ++y) {
    size_t x = w->drumpad.index;
    if (w->drumpad.pad[x][y]) {
      w->drumpad.process[y](audio, ins, ins->out_buffer, ins->samples);
    }
  }
  for (size_t i = 0; i < ins->samples; i += channel_count) {
    w->lfo.lfo = w->lfo.offset + w->lfo.amplitude * sinf((w->lfo.hz * w->lfo.tick * 2 * PI32) / (f32)sample_rate);
    w->lfo.tick += 1;
    if (w->lfo.lfo_target != NULL) {
      *w->lfo.lfo_target = w->lfo.lfo;
    }
    i32 offsets[2] = {
      w->left_offset,
      w->right_offset
    };

    for (i32 channel_index = 0; channel_index < channel_count; ++channel_index) {
      i32 offset = offsets[(channel_index % 2) == 0];
      size_t mod_sample_index = (size_t)((w->mod_tick + offset) * w->freq_mod);
      f32 mod_sample = w->mod_source.buffer[mod_sample_index % w->mod_source.samples];
      size_t sample_index = (size_t)((w->tick + offset) * (w->freq + mod_sample));
      f32 sample = volume * w->source.buffer[sample_index % w->source.samples];
      if (sample_index >= w->source.samples) {
        w->tick = 0;
      }
      if (mod_sample_index >= w->mod_source.samples) {
        w->mod_tick = 0;
      }
      ins->out_buffer[i + channel_index] += sample;
    }

    w->tick += w->speed;
    w->mod_tick += w->speed;
    w->freq = lerp_f32(w->freq, w->freq_target, sample_dt * w->interp_speed);
    w->freq_mod = lerp_f32(w->freq_mod, w->freq_mod_target, sample_dt * w->interp_speed);
    ins->volume = lerp_f32(ins->volume, w->volume_target, sample_dt * w->interp_speed);
  }
  if (w->distortion) {
#ifdef EXPERIMENTAL
    static i32 tmp_index = 0;
    static f32 tmp_buffer[256] = {0};
    for (size_t i = 0; i < LENGTH(tmp_buffer) && i < ins->samples; ++i) {
      tmp_buffer[i] = ins->out_buffer[i];
    }
    for (size_t i = 0; i < ins->samples; ++i) {
      tmp_index = (tmp_index + 1) % LENGTH(tmp_buffer);
      ins->out_buffer[i] = 0.5f * ins->out_buffer[i] + 0.5f * tmp_buffer[(tmp_index & 6) % LENGTH(tmp_buffer)];
    }
#endif
    for (size_t i = 0; i < ins->samples; ++i) {
      ins->out_buffer[i] = CLAMP(ins->out_buffer[i] * 8.0f, -1.0f, 1.0f) * 0.25f;
    }
  }
  if (w->gain >= 0.0f) {
    for (size_t i = 0; i < ins->samples; ++i) {
      ins->out_buffer[i] *= w->gain;
    }
  }
  if (w->flipflop) {
    w->speed = -w->speed;
  }
}

void waveshaper_destroy(struct Instrument* ins) {
  Waveshaper* w = (Waveshaper*)ins->userdata;
  arena_free(&w->arena);
  audio_unload_audio(&w->source);
  audio_unload_audio(&w->mod_source);
}

#undef ARENA_SIZE
#undef INFO_TEXT_SIZE
