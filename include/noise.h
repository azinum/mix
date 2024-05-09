// noise.h

#ifndef _NOISE_H
#define _NOISE_H

void noise_init(Instrument* ins, Mix* mix);
void noise_ui_new(Instrument* ins, Element* container);
void noise_update(Instrument* ins, Mix* mix);
void noise_process(Instrument* ins, Mix* mix, Audio_engine* audio, f32 dt);
void noise_noteon(Instrument* ins, u8 note, f32 velocity);
void noise_noteoff(Instrument* ins, u8 note);
void noise_destroy(Instrument* ins);

#endif // _NOISE_H
