// keyboard.c

Keyboard keyboard = (Keyboard) {
  .event_count = 0,
  .octave = 2,
  .velocity = 0.8,
};

static const i32 keycode_to_note_map[90] = {
  [KEY_A]           = 1 + 0,   // C
    [KEY_W]         = 1 + 1,   // C#
  [KEY_S]           = 1 + 2,   // D
    [KEY_E]         = 1 + 3,   // D#
  [KEY_D]           = 1 + 4,   // E
  [KEY_F]           = 1 + 5,   // F
    [KEY_T]         = 1 + 6,   // F#
  [KEY_G]           = 1 + 7,   // G
    [KEY_Y]         = 1 + 8,   // G#
  [KEY_H]           = 1 + 9,   // A
    [KEY_U]         = 1 + 10,  // A#
  [KEY_J]           = 1 + 11,  // B
  [KEY_K]           = 1 + 12,  // C
    [KEY_O]         = 1 + 13,  // C#
  [KEY_L]           = 1 + 14,  // D
    [KEY_P]         = 1 + 15,  // D#
  [KEY_SEMICOLON]   = 1 + 16,  // E
  [KEY_APOSTROPHE]  = 1 + 17,  // F
};

static void keyboard_add_event(const Midi_event* event);

void keyboard_add_event(const Midi_event* event) {
  ASSERT(event != NULL);
  if (keyboard.event_count < MAX_KEYBOARD_EVENT) {
    keyboard.events[keyboard.event_count++] = *event;
  }
}

void keyboard_init(void) {
  keyboard.event_count = 0;
  keyboard.octave = 2;
  keyboard.velocity = 0.8f;
  keyboard.channel = 0;
}

void keyboard_update(void) {
  if (ui_input_interacting() || IsKeyDown(KEY_LEFT_CONTROL)) {
    return;
  }

  keyboard.event_count = 0;
  i32 key = 0;
  while ((key = GetKeyPressed()) != 0) {
    if (key < (i32)LENGTH(keycode_to_note_map)) {
      i32 note = keycode_to_note_map[key];
      if (note != 0) {
        note -= 1;
        note += keyboard.octave * 12;
        Midi_event event = (Midi_event) {
          .message = MIDI_NOTE_ON,
          .velocity = keyboard.velocity,
          .note = note,
          .channel = keyboard.channel,
        };
        keyboard_add_event(&event);
      }
    }
    if (key == KEY_X) {
      keyboard.octave = CLAMP(keyboard.octave + 1, 0, 6);
    }
    if (key == KEY_Z) {
      keyboard.octave = CLAMP(keyboard.octave - 1, 0, 6);
    }
    if (key == KEY_V) {
      keyboard.velocity = CLAMP(keyboard.velocity + 0.1f, 0, 1);
    }
    if (key == KEY_C) {
      keyboard.velocity = CLAMP(keyboard.velocity - 0.1f, 0, 1);
    }
  }
}

u32 keyboard_query_event(Midi_event* event) {
  ASSERT(event != NULL);
  u32 result = keyboard.event_count;
  if (result > 0) {
    *event = keyboard.events[--keyboard.event_count];
  }
  return result;
}
