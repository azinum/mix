// fx_distortion.h

#ifndef _FX_DISTORTION_H
#define _FX_DISTORTION_H

void fx_distortion_init(Instrument* ins);
void fx_distortion_ui_new(Instrument* ins, Element* container);
void fx_distortion_update(Instrument* ins, struct Mix* mix);
void fx_distortion_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt);
void fx_distortion_destroy(struct Instrument* ins);

#endif // _FX_DISTORTION_H
