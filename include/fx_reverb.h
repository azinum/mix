// fx_reverb.h

#ifndef _FX_REVERB_H
#define _FX_REVERB_H

void fx_reverb_init(Instrument* ins, struct Mix* mix);
void fx_reverb_ui_new(Instrument* ins, Element* container);
void fx_reverb_update(Instrument* ins, struct Mix* mix);
void fx_reverb_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt);
void fx_reverb_destroy(struct Instrument* ins);

#endif // _FX_REVERB_H
