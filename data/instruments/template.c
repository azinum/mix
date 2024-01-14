// template.c
// cc template.c -o template.so -shared -lmix

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

}

void destroy(Instrument* ins) {

}
