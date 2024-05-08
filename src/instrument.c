// instrument.c
// TODO:
//  - generalize resetting of instruments

#define DEFINE_INSTRUMENT(ID, NAME, TITLE) [ID] = { .title = TITLE, .init = NAME##_init, .ui_new = NAME##_ui_new, .update = NAME##_update, .process = NAME##_process, .noteon = NAME##_noteon, .noteoff = NAME##_noteoff, .destroy = NAME##_destroy, }

Instrument instruments[MAX_INSTRUMENT_ID] = {
  DEFINE_INSTRUMENT(INSTRUMENT_WAVE_SHAPER, waveshaper, "waveshaper"),
  DEFINE_INSTRUMENT(INSTRUMENT_DUMMY, dummy, "dummy"),
  DEFINE_INSTRUMENT(INSTRUMENT_NOISE, noise, "noise"),
  DEFINE_INSTRUMENT(INSTRUMENT_AUDIO_INPUT, audio_input, "audio input"),
  DEFINE_INSTRUMENT(INSTRUMENT_BASIC_POLY_SYNTH, basic_poly_synth, "basic poly synth"),
  DEFINE_INSTRUMENT(INSTRUMENT_TRACKER, tracker, "tracker"),
};

void instrument_init_default(Instrument* ins) {
  ins->in_buffer = NULL;
  ins->out_buffer = NULL;
  ins->samples = 0;
  ins->volume = INSTRUMENT_VOLUME_DEFAULT;
  ins->latency = 0;
  ins->audio_latency = 0;
  ins->blocking = true;
  ins->blocking_mutex = ticket_mutex_new();
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

void instrument_init(Instrument* ins, Audio_engine* audio, Mix* mix) {
  ticket_mutex_begin(&ins->blocking_mutex);
  const size_t samples = audio->frames_per_buffer * audio->channel_count;
  MEMORY_TAG("instrument.instrument_init: audio buffer");
  if (AUDIO_INPUT) {
    ins->in_buffer = memory_calloc(samples, sizeof(f32));
  }
  ins->out_buffer = memory_calloc(samples, sizeof(f32));
  if (ins->out_buffer) {
    ins->samples = samples;
  }
  MEMORY_TAG("instrument userdata");
  ins->init(ins, mix);
  ins->initialized = true;
  ins->blocking = false;
  ticket_mutex_end(&ins->blocking_mutex);
}

void instrument_ui_new(Instrument* ins, Element* container) {
  ASSERT(ins->initialized && "instrument must be initialized before creating the ui for it");
  ui_set_title(container, ins->title);
  container->border = true;
  container->scissor = true;
  container->placement = PLACEMENT_BLOCK;
  container->background = true;
  container->background_color = UI_BACKGROUND_COLOR;
  ins->ui = container;
  ins->ui_new(ins, container);
}

void instrument_update(Instrument* ins, struct Mix* mix) {
  TIMER_START();
  if (LIKELY(ins->initialized)) {
    for (size_t i = 0; i < mix->midi_event_count; ++i) {
      Midi_event event = mix->midi_events[i];
      switch (event.message) {
        case MIDI_NOTE_ON: {
          if (ins->noteon && event.velocity > 0) {
            ins->noteon(ins, event.note, event.velocity);
          }
          break;
        }
        case MIDI_NOTE_OFF: {
          if (ins->noteoff) {
            ins->noteoff(ins, event.note);
          }
          break;
        }
        default:
          break;
      }
    }

    ins->update(ins, mix);
  }
  ins->latency = TIMER_END();
}

void instrument_process(Instrument* ins, struct Mix* mix, Audio_engine* audio, f32 dt) {
  TIMER_START();
  if (UNLIKELY(ins->blocking)) {
    return;
  }
  ticket_mutex_begin(&ins->blocking_mutex);
  ins->blocking = true;
  if (LIKELY(ins->initialized)) {
    ins->process(ins, mix, audio, dt);
    for (size_t i = 0; i < ins->samples; ++i) {
      ins->out_buffer[i] *= ins->volume;
    }
  }
  ins->audio_latency = TIMER_END();
  ins->blocking = false;
  ticket_mutex_end(&ins->blocking_mutex);
}

void instrument_destroy(Instrument* ins) {
  if (!UNLIKELY(ins->initialized)) {
    return;
  }
  ticket_mutex_begin(&ins->blocking_mutex);
  ins->blocking = true;
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
  ticket_mutex_end(&ins->blocking_mutex);
}
