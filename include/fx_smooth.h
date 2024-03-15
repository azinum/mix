// fx_smooth.h

#ifndef _FX_SMOOTH_H
#define _FX_SMOOTH_H

void fx_smooth_init(Instrument* ins);
void fx_smooth_ui_new(Instrument* ins, Element* container);
void fx_smooth_update(Instrument* ins, struct Mix* mix);
void fx_smooth_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt);
void fx_smooth_destroy(struct Instrument* ins);

#endif // _FX_SMOOTH_H
