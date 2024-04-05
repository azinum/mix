// audio_input.h

#ifndef _AUDIO_INPUT_H
#define _AUDIO_INPUT_H

void audio_input_init(Instrument* ins);
void audio_input_ui_new(Instrument* ins, Element* container);
void audio_input_update(Instrument* ins, struct Mix* mix);
void audio_input_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt);
void audio_input_destroy(struct Instrument* ins);

#endif // _AUDIO_INPUT_H
