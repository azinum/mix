// sine_generator.c
// cc sine_generator.c -o sine_generator.so -shared -lmix

#include <mix/mix.h>

static void init(Instrument* ins);
static void ui_new(Instrument* ins, Element* container);
static void update(Instrument* ins, Mix* mix);
static void process(Instrument* ins, Mix* mix, Audio_engine* audio, f32 dt);
static void destroy(Instrument* ins);

void init(Instrument* ins) {

}

void ui_new(Instrument* ins, Element* container) {

}

void update(Instrument* ins, Mix* mix) {

}

void process(Instrument* ins, Mix* mix, Audio_engine* audio, f32 dt) {
  for (size_t i = 0; i < ins->samples; ++i) {
    ins->out_buffer[i] = ins->volume * sinf((55.0f * i * audio->channel_count * PI32) / audio->sample_rate);
  }
}

void destroy(Instrument* ins) {

}
