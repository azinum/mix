// instrument.c

#define VOLUME_DEFAULT 0.1f

Instrument instruments[MAX_INSTRUMENT_ID] = {
  [INSTRUMENT_WAVE_SHAPER] = { .title = "waveshaper", .init = waveshaper_init, .ui_new = waveshaper_ui_new, .update = waveshaper_update, .process = waveshaper_process, .free = waveshaper_free, },
};

static void instrument_init_default(Instrument* ins);

void instrument_init_default(Instrument* ins) {
  ins->buffer = NULL;
  ins->frames = 0;
  ins->volume = VOLUME_DEFAULT;
  ins->latency = 0;
  ins->audio_latency = 0;
}

Instrument instrument_new(Instrument_id id) {
  Instrument ins = {0};
  if (id < 0 || id > MAX_INSTRUMENT_ID) {
    return ins;
  }
  ins = instruments[id];
  instrument_init_default(&ins);
  return ins;
}

Instrument instrument_new_from_path(const char* path) {
  Instrument ins = {0};
  NOT_IMPLEMENTED();
  return ins;
}

void instrument_init(Instrument* ins, Audio_engine* audio) {
  const size_t frames = audio->frames_per_buffer * audio->channel_count;
  ins->buffer = memory_calloc(frames, sizeof(f32));
  if (ins->buffer) {
    ins->frames = frames;
  }
  ins->init(ins);
}

Element instrument_ui_new(Instrument* ins) {
  Element container = ui_container(ins->title);
  container.border = true;
  container.scissor = true;
  container.placement = PLACEMENT_BLOCK;
  container.background = true;
  ins->ui_new(ins, &container);
  return container;
}

void instrument_update(Instrument* ins, struct Mix* mix) {
  TIMER_START();
  ins->update(ins, mix);
  ins->latency = TIMER_END();
}

void instrument_process(Instrument* ins, struct Mix* mix, Audio_engine* audio, f32 dt) {
  TIMER_START();
  ins->blocking = true;
  ins->process(ins, mix, audio, dt);
  ins->audio_latency = TIMER_END();
  ins->blocking = false;
}

void instrument_free(Instrument* ins) {
  while (ins->blocking) {
    spin_wait();
  }
  ins->free(ins);
  memory_free(ins->buffer);
  ins->buffer = NULL;
  ins->frames = 0;
  memory_free(ins->userdata);
  ins->userdata = NULL;
}
