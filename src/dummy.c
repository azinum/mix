// dummy.c

typedef struct Dummy {
  i32 pluck;
  f32 velocity;
  f32 decay;
  f32 frequency;
  size_t tick;
  f32* feedback_buffer;
  size_t feedback_buffer_size;
  i32 feedback;
  f32 feedback_amount;
  i32 feedback_offset;
  f32 noise_amount;
  Audio_source source;
} Dummy;

static void dummy_default(Dummy* dummy);
static void dummy_randomize_settings(Element* e);
static void pluck(Element* e);

void dummy_default(Dummy* dummy) {
  dummy->pluck = false;
  dummy->velocity = 0.0f;
  dummy->decay = 10;
  dummy->frequency = 55.0f;
  dummy->tick = 0;
  MEMORY_TAG("dummy feedback buffer");
  size_t feedback_buffer_size = SAMPLE_RATE / (0.4f * CHANNEL_COUNT); // 400 ms
  dummy->feedback_buffer = memory_calloc(sizeof(f32), feedback_buffer_size);
  if (dummy->feedback_buffer) {
    dummy->feedback_buffer_size = feedback_buffer_size;
  }
  dummy->feedback = false;
  dummy->feedback_amount = 0.1f;
  dummy->feedback_offset = 0;
  dummy->noise_amount = 0.5f;
  Audio_source source = {
    .buffer = (f32*)&sine[0],
    .samples = LENGTH(sine),
    .channel_count = 2,
    .ready = true,
    .internal = true,
  };
  dummy->source = source;
}

void dummy_randomize_settings(Element* e) {
  Dummy* dummy = (Dummy*)e->userdata;
  dummy->velocity = random_f32();
  dummy->decay = random_f32() * 30;
  dummy->frequency = freq_table[random_number() % (LENGTH(freq_table) / 2)];
  dummy->tick = random_number() % 10000;
  dummy->feedback_amount = random_f32() * 0.99f;
  dummy->feedback_offset = random_number() % dummy->feedback_buffer_size;
  dummy->noise_amount = random_f32();

  dummy->pluck = true;
}

void pluck(Element* e) {
  Instrument* ins = (Instrument*)e->userdata;
  Dummy* dummy = (Dummy*)ins->userdata;
  dummy->pluck = true;
}

void dummy_init(Instrument* ins) {
  Dummy* dummy = memory_alloc(sizeof(Dummy));
  ASSERT(dummy != NULL);
  ins->userdata = dummy;
  dummy_default(dummy);
}

void dummy_ui_new(Instrument* ins, Element* container) {
  Dummy* dummy = (Dummy*)ins->userdata;

  const i32 button_height = FONT_SIZE * 2;
  {
    Element e = ui_button("pluck");
    e.box.h = button_height;
    e.sizing = SIZING_PERCENT(100, 0);
    e.onclick = pluck;
    e.userdata = ins;
    e.tooltip = "pluck the instrument (E)";
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_toggle_ex(&dummy->feedback, "feedback");
    e.box.h = button_height;
    e.sizing = SIZING_PERCENT(50, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_button("randomize");
    e.box.h = button_height;
    e.sizing = SIZING_PERCENT(50, 0);
    e.tooltip = "randomize settings and pluck the instrument";
    e.onclick = dummy_randomize_settings;
    e.userdata = dummy;
    ui_attach_element(container, &e);
  }

  {
    Element e = ui_text("volume");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(container, &e);
  }

  {
    Element e = ui_input_float("volume", &ins->volume);
    e.sizing = SIZING_PERCENT(20, 0);
    e.box.h = button_height;
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider_float(&ins->volume, 0, 1);
    e.sizing = SIZING_PERCENT(80, 0);
    e.box.h = button_height;
    ui_attach_element(container, &e);
  }

  {
    Element e = ui_text("feedback amount");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(container, &e);
  }

  {
    Element e = ui_input_float("feedback amount", &dummy->feedback_amount);
    e.sizing = SIZING_PERCENT(20, 0);
    e.box.h = button_height;
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider_float(&dummy->feedback_amount, 0.001f, 0.99f);
    e.sizing = SIZING_PERCENT(80, 0);
    e.box.h = button_height;
    ui_attach_element(container, &e);
  }

  {
    Element e = ui_text("feedback offset");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_input_int("feedback offset", &dummy->feedback_offset);
    e.sizing = SIZING_PERCENT(20, 0);
    e.box.h = button_height;
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider_int(&dummy->feedback_offset, 0, dummy->feedback_buffer_size);
    e.sizing = SIZING_PERCENT(80, 0);
    e.box.h = button_height;
    ui_attach_element(container, &e);
  }

  {
    Element e = ui_text("decay");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_input_float("decay", &dummy->decay);
    e.sizing = SIZING_PERCENT(20, 0);
    e.box.h = button_height;
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider_float(&dummy->decay, 0.001f, 30.0f);
    e.sizing = SIZING_PERCENT(80, 0);
    e.box.h = button_height;
    ui_attach_element(container, &e);
  }

  {
    Element e = ui_text("frequency");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_input_float("frequency", &dummy->frequency);
    e.sizing = SIZING_PERCENT(20, 0);
    e.box.h = button_height;
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider_float(&dummy->frequency, 0.0f, 440.0f);
    e.sizing = SIZING_PERCENT(80, 0);
    e.box.h = button_height;
    ui_attach_element(container, &e);
  }

  {
    Element e = ui_text("noise");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_input_float("noise", &dummy->noise_amount);
    e.sizing = SIZING_PERCENT(20, 0);
    e.box.h = button_height;
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider_float(&dummy->noise_amount, 0.0f, 1.0f);
    e.sizing = SIZING_PERCENT(80, 0);
    e.box.h = button_height;
    ui_attach_element(container, &e);
  }
}

void dummy_update(Instrument* ins, struct Mix* mix) {
  (void)ins;
  (void)mix;
  Dummy* dummy = (Dummy*)ins->userdata;
  bool mod_key = IsKeyDown(KEY_LEFT_CONTROL);

  if (!ui_input_interacting() && !mod_key) {
    if (IsKeyPressed(KEY_E)) {
      dummy->pluck = true;
    }
  }
  if (IsFileDropped()) {
    FilePathList files = LoadDroppedFiles();
    if (files.count > 0) {
      char* path = files.paths[0];
      Audio_source loaded_source = audio_load_audio(path);
      if (loaded_source.buffer != NULL) {
        audio_unload_audio(&dummy->source);
        dummy->source = loaded_source;
      }
      else {
        ui_alert("failed to load audio file\n%s", path);
      }
    }
    UnloadDroppedFiles(files);
  }
}

void dummy_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt) {
  (void)ins;
  (void)mix;
  (void)audio;
  (void)dt;

  Dummy* dummy = (Dummy*)ins->userdata;
  const i32 sample_rate = audio->sample_rate;
  const i32 channel_count = audio->channel_count;
  const f32 sample_dt = dt / (f32)ins->samples;
  f32 volume = ins->volume;

  size_t delta_to_sample_index = (size_t)(ins->samples * mix->tick_delta);
  bool plucked = false;
  for (size_t i = 0; i < ins->samples; ++i, ++dummy->tick) {
    if (dummy->pluck) {
      dummy->pluck = false;
      plucked = true;
    }
    if (i == delta_to_sample_index && plucked) {
      dummy->velocity = 1.0f;
      dummy->tick = 0;
    }
    f32 source_sample = dummy->source.buffer[(size_t)(dummy->tick * dummy->frequency) % dummy->source.samples];
    f32 noise = 2 * (random_f32() - 0.5f);
    f32 sample = volume * dummy->velocity * (dummy->noise_amount * noise + (1 - dummy->noise_amount) * source_sample);
    dummy->velocity = lerp_f32(dummy->velocity, 0, dummy->decay * sample_dt);
    ins->out_buffer[i] = sample;
    if (dummy->feedback) {
      ins->out_buffer[i] += dummy->feedback_buffer[dummy->tick % dummy->feedback_buffer_size];
      f32 feedback_sample = dummy->feedback_amount * ins->out_buffer[i];
      dummy->feedback_buffer[(dummy->tick + abs(dummy->feedback_offset)) % dummy->feedback_buffer_size] = feedback_sample;
    }
  }
}

void dummy_destroy(struct Instrument* ins) {
  Dummy* dummy = (Dummy*)ins->userdata;
  memory_free(dummy->feedback_buffer);
  audio_unload_audio(&dummy->source);
}
