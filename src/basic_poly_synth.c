// basic_poly_synth.c

#define MAX_VOICES (28)
#define MIN_ATTACK  0.01f
#define MIN_RELEASE 0.01f

typedef enum Bps_state {
  BPS_STATE_SILENT,
  BPS_STATE_ATTACK,
  BPS_STATE_RELEASE,
} Bps_state;

typedef struct Bps_voice {
  f32 amplitude;
  f32 velocity;
  f32 timer;
  Bps_state state;
  i32 note;
  size_t tick;
  f32 pan;
} Bps_voice;

typedef struct Bps {
  f32 attack;
  f32 release;
  Bps_voice voices[MAX_VOICES];
  f32 voice_blend; // sine -> saw
  i32 vp_octave; // virtual piano
  f32 vp_velocity; // virtual piano
} Bps;

static void basic_poly_synth_default(Bps* bps);
static void basic_poly_synth_reset_voices(Bps* bps);
static void basic_poly_synth_play_note(Bps* bps, i32 note, f32 velocity);
static Bps_voice* basic_poly_synth_find_silent_voice(Bps* bps);

void basic_poly_synth_default(Bps* bps) {
  bps->attack = 0.05f;
  bps->release = 0.9f;
  bps->voice_blend = 0.2f;
  bps->vp_octave = 2;
  bps->vp_velocity = 0.8f;
  basic_poly_synth_reset_voices(bps);
}

void basic_poly_synth_reset_voices(Bps* bps) {
  Bps_voice voice = (Bps_voice) {
    .amplitude = 0,
    .velocity = 0,
    .timer = 0,
    .state = BPS_STATE_SILENT,
    .note = 0,
    .pan = 0.5f,
  };
  for (size_t i = 0; i < MAX_VOICES; ++i) {
    bps->voices[i] = voice;
  }
}

void basic_poly_synth_play_note(Bps* bps, i32 note, f32 velocity) {
  Bps_voice* voice = basic_poly_synth_find_silent_voice(bps);
  const f32 register_influence = 0.4f;
  const f32 random_influence = 0.2f;
  f32 pan_random = random_influence * (random_f32() - 0.5f); // r * [-.5, .5]
  if (voice) {
    voice->state = BPS_STATE_ATTACK;
    voice->amplitude = 0;
    voice->velocity = velocity;
    voice->note = note;
    voice->pan = lerp_f32(1 - (note / 88.0f), 0.5f + pan_random, 1.0f - register_influence);
  }
}

Bps_voice* basic_poly_synth_find_silent_voice(Bps* bps) {
  for (size_t i = 0; i < MAX_VOICES; ++i) {
    if (bps->voices[i].state == BPS_STATE_SILENT) {
      return &bps->voices[i];
    }
  }
  return NULL;
}

void basic_poly_synth_init(Instrument* ins) {
  Bps* bps = memory_alloc(sizeof(Bps));
  ASSERT(bps != NULL);
  ins->userdata = bps;
  basic_poly_synth_default(bps);
}

void basic_poly_synth_ui_new(Instrument* ins, Element* container) {
  Bps* bps = (Bps*)ins->userdata;
  const i32 button_height = FONT_SIZE * 2;

  ui_attach_element_v2(container, ui_text_line("voices"));
  for (size_t i = 0; i < MAX_VOICES; ++i) {
    Bps_voice* voice = &bps->voices[i];
    Element e = ui_slider_float(&voice->amplitude, 0, 1);
    e.data.slider.slider_type = SLIDER_VERTICAL;
    e.sizing = SIZING_PIXELS(button_height * .5f, button_height);
    ui_attach_element(container, &e);
  }
  ui_attach_element_v2(container, ui_line_break(0));

  ui_attach_element_v2(container, ui_text_line("volume"));
  {
    Element e = ui_input_float("volume", &ins->volume);
    e.sizing = (Sizing) {
      .x_mode = SIZE_MODE_PERCENT,
      .y_mode = SIZE_MODE_PIXELS,
      .x = 20,
      .y = button_height,
    };
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider_float(&ins->volume, 0, 1);
    e.sizing = (Sizing) {
      .x_mode = SIZE_MODE_PERCENT,
      .y_mode = SIZE_MODE_PIXELS,
      .x = 80,
      .y = button_height,
    };
    ui_attach_element(container, &e);
  }
  ui_attach_element_v2(container, ui_text_line("attack"));
  {
    Element e = ui_input_float("attack", &bps->attack);
    e.sizing = (Sizing) {
      .x_mode = SIZE_MODE_PERCENT,
      .y_mode = SIZE_MODE_PIXELS,
      .x = 20,
      .y = button_height,
    };
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider_float(&bps->attack, MIN_ATTACK, 4);
    e.sizing = (Sizing) {
      .x_mode = SIZE_MODE_PERCENT,
      .y_mode = SIZE_MODE_PIXELS,
      .x = 80,
      .y = button_height,
    };
    ui_attach_element(container, &e);
  }
  ui_attach_element_v2(container, ui_text_line("release"));
  {
    Element e = ui_input_float("release", &bps->release);
    e.sizing = (Sizing) {
      .x_mode = SIZE_MODE_PERCENT,
      .y_mode = SIZE_MODE_PIXELS,
      .x = 20,
      .y = button_height,
    };
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider_float(&bps->release, MIN_RELEASE, 4);
    e.sizing = (Sizing) {
      .x_mode = SIZE_MODE_PERCENT,
      .y_mode = SIZE_MODE_PIXELS,
      .x = 80,
      .y = button_height,
    };
    ui_attach_element(container, &e);
  }

  ui_attach_element_v2(container, ui_text_line("voice blend"));
  {
    Element e = ui_input_float("voice blend", &bps->voice_blend);
    e.sizing = (Sizing) {
      .x_mode = SIZE_MODE_PERCENT,
      .y_mode = SIZE_MODE_PIXELS,
      .x = 20,
      .y = button_height,
    };
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider_float(&bps->voice_blend, 0, 1);
    e.sizing = (Sizing) {
      .x_mode = SIZE_MODE_PERCENT,
      .y_mode = SIZE_MODE_PIXELS,
      .x = 80,
      .y = button_height,
    };
    ui_attach_element(container, &e);
  }

  ui_attach_element_v2(container, ui_text_line("octave (virtual piano)"));
  {
    Element e = ui_input_int("octave", &bps->vp_octave);
    e.sizing = (Sizing) {
      .x_mode = SIZE_MODE_PERCENT,
      .y_mode = SIZE_MODE_PIXELS,
      .x = 100,
      .y = button_height,
    };
    ui_attach_element(container, &e);
  }

  ui_attach_element_v2(container, ui_text_line("velocity (virtual piano)"));
  {
    Element e = ui_input_float("velocity", &bps->vp_velocity);
    e.sizing = (Sizing) {
      .x_mode = SIZE_MODE_PERCENT,
      .y_mode = SIZE_MODE_PIXELS,
      .x = 20,
      .y = button_height,
    };
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_slider_float(&bps->vp_velocity, 0, 1);
    e.sizing = (Sizing) {
      .x_mode = SIZE_MODE_PERCENT,
      .y_mode = SIZE_MODE_PIXELS,
      .x = 80,
      .y = button_height,
    };
    ui_attach_element(container, &e);
  }
}

static const i32 keycode_to_note_map[100] = {
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

void basic_poly_synth_update(Instrument* ins, struct Mix* mix) {
  (void)ins; (void)mix;
  Bps* bps = (Bps*)ins->userdata;

  bool mod_key = IsKeyDown(KEY_LEFT_CONTROL);
  if (!mod_key && !ui_input_interacting()) {
    i32 key = 0;
    while ((key = GetKeyPressed()) != 0) {
      if (key < (i32)LENGTH(keycode_to_note_map)) {
        i32 note = keycode_to_note_map[key];
        if (note != 0) {
          note -= 1;
          note += bps->vp_octave * 12;
          basic_poly_synth_play_note(bps, note, bps->vp_velocity);
        }
      }
      if (key == KEY_X) {
        bps->vp_octave = CLAMP(bps->vp_octave + 1, 0, 12);
      }
      if (key == KEY_Z) {
        bps->vp_octave = CLAMP(bps->vp_octave - 1, 0, 12);
      }
      if (key == KEY_V) {
        bps->vp_velocity = CLAMP(bps->vp_velocity + 0.1f, 0, 1);
      }
      if (key == KEY_C) {
        bps->vp_velocity = CLAMP(bps->vp_velocity - 0.1f, 0, 1);
      }
    }
  }
}

void basic_poly_synth_process(struct Instrument* ins, struct Mix* mix, struct Audio_engine* audio, f32 dt) {
  (void)mix;
  Bps* bps = (Bps*)ins->userdata;

  // sanity checks
  if (bps->attack < MIN_ATTACK) {
    bps->attack = MIN_ATTACK;
  }
  if (bps->release < MIN_RELEASE) {
    bps->release = MIN_RELEASE;
  }

  for (size_t i = 0; i < audio->midi_event_count; ++i) {
    Midi_event e = audio->midi_events[i];
    if (e.message == MIDI_NOTE_ON && e.velocity > 0) {
      i32 note = CLAMP(e.note - 24, 0, (i32)LENGTH(freq_table));
      basic_poly_synth_play_note(bps, note, e.velocity);
    }
  }
  f32 sample_dt = dt / (f32)ins->samples;
  f32 saw_amplitude = bps->voice_blend;
  f32 sine_amplitude = 1 - saw_amplitude;
  const i32 channel_count = audio->channel_count;

  memset(ins->out_buffer, 0, sizeof(f32) * ins->samples);

  for (size_t voice_index = 0; voice_index < MAX_VOICES; ++voice_index) {
    Bps_voice* voice = &bps->voices[voice_index];
    f32 pan_map[2] = {
      voice->pan,     // channel 1 (left)
      1 - voice->pan, // channel 2 (right)
    };
    if (voice->state != BPS_STATE_SILENT) {
      for (size_t i = 0; i < ins->samples; i += channel_count) {
        for (i32 channel_index = 0; channel_index < channel_count; ++channel_index) {
          switch (voice->state) {
            case BPS_STATE_ATTACK: {
              voice->amplitude = lerp_f32(voice->amplitude, 1, sample_dt * (1.0f / bps->attack));
              if (voice->amplitude >= 0.99) {
                voice->state = BPS_STATE_RELEASE;
              }
              break;
            }
            case BPS_STATE_RELEASE: {
              voice->amplitude = lerp_f32(voice->amplitude, 0, sample_dt * (1.0f / bps->release));
              if (voice->amplitude <= 0.01) {
                voice->state = BPS_STATE_SILENT;
                voice->timer = 0;
                voice->tick = 0;
                goto voice_finished;
              }
              break;
            }
            default: {
              voice->timer = 0;
              voice->tick = 0;
              goto voice_finished;
            }
          }
          f32 freq = freq_table[voice->note % LENGTH(freq_table)];
          size_t sample_index = (size_t)(voice->tick * freq);
          ins->out_buffer[i + channel_index] += pan_map[channel_index % LENGTH(pan_map)] *
            ((ins->volume * voice->amplitude * voice->velocity * sine_amplitude * sine[sample_index % LENGTH(sine)]) +
            (ins->volume * voice->amplitude * voice->velocity * saw_amplitude * saw[sample_index % LENGTH(saw)]));

          voice->timer += sample_dt;
          voice->tick += 1;
        }
      }
voice_finished: {}
    }
  }
}

void basic_poly_synth_destroy(struct Instrument* ins) {
  (void)ins;
}

// a = pan
// b = 1 - a

// a = 0.6
// b = 0.4
#undef MAX_VOICES
#undef MIN_ATTACK
#undef MIN_RELEASE
