// fx_filter.h

#ifndef _FX_FILTER_H
#define _FX_FILTER_H

void fx_filter_init(Instrument* ins);
void fx_filter_ui_new(Instrument* ins, Element* container);
void fx_filter_update(Instrument* ins, struct Mix* mix);
void fx_filter_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt);
void fx_filter_destroy(struct Instrument* ins);

#endif // _FX_FILTER_H
