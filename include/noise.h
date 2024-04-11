// noise.h

#ifndef _NOISE_H
#define _NOISE_H

void noise_init(Instrument* ins);
void noise_ui_new(Instrument* ins, Element* container);
void noise_update(Instrument* ins, struct Mix* mix);
void noise_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt);
void noise_noteon(struct Instrument* ins, u8 note, f32 velocity);
void noise_noteoff(struct Instrument* ins, u8 note);
void noise_destroy(struct Instrument* ins);

#endif // _NOISE_H
