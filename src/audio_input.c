// audio_input.c

void audio_input_init(Instrument* ins, Mix* mix) {
  (void)ins;
  (void)mix;
}

void audio_input_ui_new(Instrument* ins, Element* container) {
  (void)ins; (void)container;
}

void audio_input_update(Instrument* ins, Mix* mix) {
  (void)ins; (void)mix;
}

void audio_input_process(Instrument* ins, Mix* mix, Audio_engine* audio, f32 dt) {
  (void)mix;
  (void)audio;
  (void)dt;
  if (!ins->in_buffer) {
    return;
  }
  for (size_t i = 0; i < ins->samples; ++i) {
    ins->out_buffer[i] = ins->in_buffer[i];
  }
}

void audio_input_noteon(Instrument* ins, u8 note, f32 velocity) {
  (void)ins; (void)note; (void)velocity;
}

void audio_input_noteoff(Instrument* ins, u8 note) {
  (void)ins; (void)note;
}

void audio_input_destroy(Instrument* ins) {
  (void)ins;
}
