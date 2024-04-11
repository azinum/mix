// midi.c

struct {
  i32 fd;
  bool active;
} midi_state = {
  .fd = -1,
  .active = false,
};

// taken from lmms src/core/midi/MidiClient.cpp
static const i32 lengths_f0_f6[] = {
  0,  // 0xf0
  2,  // 0Xf1
  3,  // 0Xf2
  2,  // 0Xf3
  2,  // 0Xf4 (undefined)
  2,  // 0Xf5 (undefined)
  1,  // 0Xf6
};

static const i32 lengths_80_e0[] = {
  3,  // 0x8x note off
  3,  // 0x9x note on
  3,  // 0xax poly-key pressure
  3,  // 0xbx control change
  2,  // 0xcx program change
  2,  // 0xdx channel pressure
  3,  // 0xex pitch-bend change
};

static i32 midi_event_length(u8 event);

i32 midi_event_length(u8 event) {
  if (event < 0xf0) {
    return lengths_80_e0[((event - 0x80) >> 4) & 0x0f];
  }
  else if (event < 0xf7) {
    return lengths_f0_f6[event - 0xf0];
  }
  return 1;
}

void midi_init(void) {
  if (midi_state.fd > 0) {
    midi_close();
  }
  midi_state.fd = -1;
  midi_state.active = false;
}

Result midi_open_device(const char* path) {
#ifdef TARGET_WINDOWS
  i32 flags = O_RDONLY | O_BINARY;
#else
  i32 flags = O_RDONLY | O_NOCTTY | O_NDELAY;
#endif
  i32 fd = open(path, flags);
  if (fd < 0) {
    return Error;
  }
  midi_state.fd = fd;
  midi_state.active = true;
  return Ok;
}

size_t midi_read_events(Midi_event* events, const size_t max_events) {
  if (midi_state.fd < 0 || midi_state.active == false || !events) {
    return 0;
  }
  size_t event_count = 0;

  for (size_t i = 0; i < max_events; ++i) {
    u8 midi_buffer[16] = {0};
    i32 read_bytes = read(midi_state.fd, midi_buffer, sizeof(midi_buffer));
    if (read_bytes <= 0) {
      break;
    }
    Midi_event event = {0};
    for (i32 midi_index = 0; midi_index < read_bytes; ++midi_index) {
      u8 c = midi_buffer[midi_index];
      if (c >= 0xf0) {
        // ignore reset and status messages
        continue;
      }
      if (c & 0x80) {
        u8 channel = c & 0x0f;
        u8 status  = c & 0xf0;
        i32 bytes  = midi_event_length(c) - 1;
        if (status == 0) {
          break;
        }
        if (midi_index + bytes <= read_bytes) {
          switch (status) {
            case MIDI_NOTE_OFF:
            case MIDI_NOTE_ON: {
              event.message = c;
              event.velocity = midi_buffer[midi_index + 2] / (f32)INT8_MAX;
              event.note = CLAMP(midi_buffer[midi_index + 1] - 24, 0, INT8_MAX);
              event.channel = channel;
              events[event_count++] = event;
              break;
            }
            default:
              break;
          }
        }
        else {
          // lost event :(
          log_print(STDOUT_FILENO, LOG_TAG_WARN, "lost a midi event\n");
        }
      }
    }
  }

  return event_count;
}

void midi_close_device(void) {
  if (midi_state.fd > 0) {
    close(midi_state.fd);
    midi_state.active = false;
  }
}

void midi_close(void) {
  midi_close_device();
  midi_state.active = false;
}
