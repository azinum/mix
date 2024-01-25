// dummy.h

#ifndef _DUMMY_H
#define _DUMMY_H

void dummy_init(Instrument* ins);
void dummy_ui_new(Instrument* ins, Element* container);
void dummy_update(Instrument* ins, struct Mix* mix);
void dummy_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt);
void dummy_destroy(struct Instrument* ins);

#endif
