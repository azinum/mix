// keyboard.h

#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#define MAX_KEYBOARD_EVENT (32)

typedef struct Keyboard {
  Midi_event events[MAX_KEYBOARD_EVENT];
  u32 event_count;
  i32 octave;
  f32 velocity;
  u8 channel;
} Keyboard;

extern Keyboard keyboard;

void keyboard_init(void);
void keyboard_update(void);
u32 keyboard_query_event(Midi_event* event);

#endif // _KEYBOARD_H
