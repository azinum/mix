// fx_interpolator.h

#ifndef _FX_INTERPOLATOR_H
#define _FX_INTERPOLATOR_H

void fx_interpolator_init(Instrument* ins, struct Mix* mix);
void fx_interpolator_ui_new(Instrument* ins, Element* container);
void fx_interpolator_update(Instrument* ins, struct Mix* mix);
void fx_interpolator_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt);
void fx_interpolator_destroy(struct Instrument* ins);

#endif // _FX_INTERPOLATOR_H
