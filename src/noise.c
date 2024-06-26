// noise.c

void noise_init(Instrument* ins, Mix* mix) {
  (void)ins;
  (void)mix;
}

void noise_ui_new(Instrument* ins, Element* container) {
  (void)ins;
  (void)container;
}

void noise_update(Instrument* ins, Mix* mix) {
  (void)ins;
  (void)mix;
}

void noise_process(Instrument* ins, Mix* mix, Audio_engine* audio, f32 dt) {
  (void)mix;
  (void)audio;
  (void)dt;
  for (size_t i = 0; i < ins->samples; ++i) {
    f32 sample = ins->volume * (2.0f * (random_f32() - 0.5f));
    ins->out_buffer[i] = sample;
  }
}

void noise_noteon(Instrument* ins, u8 note, f32 velocity) {
  (void)ins; (void)note; (void)velocity;
}

void noise_noteoff(Instrument* ins, u8 note) {
  (void)ins; (void)note;
}

void noise_destroy(Instrument* ins) {
  (void)ins;
}
