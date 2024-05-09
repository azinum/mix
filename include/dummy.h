// dummy.h

#ifndef _DUMMY_H
#define _DUMMY_H

void dummy_init(Instrument* ins, Mix* mix);
void dummy_ui_new(Instrument* ins, Element* container);
void dummy_update(Instrument* ins, Mix* mix);
void dummy_process(Instrument* ins, Mix* mix,  Audio_engine* audio, f32 dt);
void dummy_noteon(Instrument* ins, u8 note, f32 velocity);
void dummy_noteoff(Instrument* ins, u8 note);
void dummy_destroy(Instrument* ins);

#endif
