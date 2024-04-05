// midi.h

#ifndef _MIDI_H
#define _MIDI_H

typedef enum Midi_message {
  MIDI_NOTE_ON = 0x90,
  MIDI_NOTE_OFF = 0x80,
} Midi_message;

typedef struct Midi_event {
  Midi_message message;
  f32 velocity;
  u8 note;
  u8 channel;
} Midi_event;

void midi_init(void);
Result midi_open_device(const char* path);
size_t midi_read_events(Midi_event* events, const size_t max_events);
void midi_close_device(void);
void midi_close(void);

#endif // _MIDI_H
