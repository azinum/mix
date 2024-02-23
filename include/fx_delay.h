// fx_delay.h

#ifndef _FX_DELAY_H
#define _FX_DELAY_H

void fx_delay_init(Instrument* ins);
void fx_delay_ui_new(Instrument* ins, Element* container);
void fx_delay_update(Instrument* ins, struct Mix* mix);
void fx_delay_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt);
void fx_delay_destroy(struct Instrument* ins);

#endif // _FX_DELAY_H
