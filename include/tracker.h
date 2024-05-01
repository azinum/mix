// tracker.h

#ifndef _TRACKER_H
#define _TRACKER_H

void tracker_init(Instrument* ins);
void tracker_ui_new(Instrument* ins, Element* container);
void tracker_update(Instrument* ins, struct Mix* mix);
void tracker_process(Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt);
void tracker_noteon(Instrument* ins, u8 note, f32 velocity);
void tracker_noteoff(Instrument* ins, u8 note);
void tracker_destroy(Instrument* ins);

#endif // _TRACKER_H
