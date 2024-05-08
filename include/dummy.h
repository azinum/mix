// dummy.h

#ifndef _DUMMY_H
#define _DUMMY_H

void dummy_init(Instrument* ins, struct Mix* mix);
void dummy_ui_new(Instrument* ins, Element* container);
void dummy_update(Instrument* ins, struct Mix* mix);
void dummy_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt);
void dummy_noteon(struct Instrument* ins, u8 note, f32 velocity);
void dummy_noteoff(struct Instrument* ins, u8 note);
void dummy_destroy(struct Instrument* ins);

#endif
