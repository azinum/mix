// keyboard.h

#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#define MAX_KEYBOARD_EVENT (32)

void keyboard_init(void);
void keyboard_update(void);
u32 keyboard_query_event(Midi_event* event);

#endif // _KEYBOARD_H
