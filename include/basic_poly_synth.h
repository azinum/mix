// basic_poly_synth.h

#ifndef _BASIC_POLY_SYNTH_H
#define _BASIC_POLY_SYNTH_H

void basic_poly_synth_init(Instrument* ins, Mix* mix);
void basic_poly_synth_ui_new(Instrument* ins, Element* container);
void basic_poly_synth_update(Instrument* ins, Mix* mix);
void basic_poly_synth_process(Instrument* ins, Mix* mix, Audio_engine* audio, f32 dt);
void basic_poly_synth_noteon(Instrument* ins, u8 note, f32 velocity);
void basic_poly_synth_noteoff(Instrument* ins, u8 note);
void basic_poly_synth_destroy(Instrument* ins);

#endif // _BASIC_POLY_SYNTH_H
