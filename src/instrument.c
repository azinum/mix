// instrument.c

Instrument instruments[MAX_INSTRUMENT_ID] = {
  [INSTRUMENT_WAVE_SHAPER] = { .title = "waveshaper", .init = waveshaper_init, .ui_new = waveshaper_ui_new, .update = waveshaper_update, .process = waveshaper_process, .destroy = waveshaper_destroy, },
  [INSTRUMENT_DUMMY]       = { .title = "dummy",      .init = dummy_init,      .ui_new = dummy_ui_new,      .update = dummy_update,      .process = dummy_process,      .destroy = dummy_destroy, },
  [INSTRUMENT_NOISE]       = { .title = "noise",      .init = noise_init,      .ui_new = noise_ui_new,      .update = noise_update,      .process = noise_process,      .destroy = noise_destroy, },
};

void instrument_init_default(Instrument* ins) {
  ins->in_buffer = NULL;
  ins->out_buffer = NULL;
  ins->samples = 0;
  ins->volume = INSTRUMENT_VOLUME_DEFAULT;
  ins->latency = 0;
  ins->audio_latency = 0;
  ins->blocking = true;
  ins->initialized = false;
  ins->ui = NULL;
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
  (void)path;
  NOT_IMPLEMENTED();
  return ins;
}

void instrument_init(Instrument* ins, Audio_engine* audio) {
  const size_t samples = audio->frames_per_buffer * audio->channel_count;
  MEMORY_TAG("instrument.instrument_init: audio buffer");
  ins->out_buffer = memory_calloc(samples, sizeof(f32));
  if (ins->out_buffer) {
    ins->samples = samples;
  }
  ins->init(ins);
  ins->initialized = true;
  ins->blocking = false;
}

void instrument_ui_new(Instrument* ins, Element* container) {
  ASSERT(ins->initialized && "instrument must be initialized before creating the ui for it");
  ui_set_title(container, ins->title);
  container->border = true;
  container->scissor = true;
  container->placement = PLACEMENT_BLOCK;
  container->background = true;
  ins->ui_new(ins, container);
  ins->ui = container;
}

void instrument_update(Instrument* ins, struct Mix* mix) {
  TIMER_START();
  if (LIKELY(ins->initialized)) {
    ins->update(ins, mix);
  }
  ins->latency = TIMER_END();
}

void instrument_process(Instrument* ins, struct Mix* mix, Audio_engine* audio, f32 dt) {
  TIMER_START();
  if (UNLIKELY(ins->blocking)) {
    return;
  }
  ins->blocking = true;
  if (LIKELY(ins->initialized)) {
    ins->process(ins, mix, audio, dt);
  }
  ins->audio_latency = TIMER_END();
  ins->blocking = false;
}

void instrument_destroy(Instrument* ins) {
  if (!UNLIKELY(ins->initialized)) {
    return;
  }
  while (ins->blocking) {
    spin_wait();
  }
  ui_detach_elements(ins->ui);
  ui_set_title(ins->ui, "empty");
  ins->destroy(ins);
  memory_free(ins->in_buffer);
  memory_free(ins->out_buffer);
  ins->in_buffer = NULL;
  ins->out_buffer = NULL;
  ins->samples = 0;
  memory_free(ins->userdata);
  ins->userdata = NULL;
  ins->initialized = false;
}
