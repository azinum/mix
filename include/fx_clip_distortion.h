// fx_clip_distortion.h

#ifndef _FX_CLIP_DISTORTION_H
#define _FX_CLIP_DISTORTION_H

void fx_clip_distortion_init(Instrument* ins, struct Mix* mix);
void fx_clip_distortion_ui_new(Instrument* ins, Element* container);
void fx_clip_distortion_update(Instrument* ins, struct Mix* mix);
void fx_clip_distortion_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt);
void fx_clip_distortion_destroy(struct Instrument* ins);

#endif // _FX_CLIP_DISTORTION_H
