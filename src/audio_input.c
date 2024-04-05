// audio_input.c

void audio_input_init(Instrument* ins) {
  (void)ins;
}

void audio_input_ui_new(Instrument* ins, Element* container) {
  (void)ins; (void)container;
}

void audio_input_update(Instrument* ins, struct Mix* mix) {
  (void)ins; (void)mix;
}

void audio_input_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt) {
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

void audio_input_destroy(struct Instrument* ins) {
  (void)ins;
}
