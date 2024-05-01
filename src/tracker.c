// tracker.c

#define MAX_AUDIO_SOURCE 32
#define MAX_TRACKER_ROW 32
#define MAX_TRACKER_CHANNELS 4

typedef enum {
  SAMPLE_STATE_END,
  SAMPLE_STATE_PLAY,
  MAX_SAMPLE_STATE,
} Sample_state;

typedef struct Tracker_source {
  Audio_source source;
  struct {
    i32 start;
    i32 length;
    i32 pingpong;
  } settings;
} Tracker_source;

typedef struct Tracker_row {
  i32 id;
  i32 note;
  i32 source_id;
} Tracker_row;

typedef struct Tracker_channel {
  Tracker_row rows[MAX_TRACKER_ROW];
  Tracker_row* active_row;
  f32 freq;
  f32 sample_index;
  Sample_state state;
} Tracker_channel;

typedef struct Tracker {
  Tracker_source sources[MAX_AUDIO_SOURCE];
  Tracker_channel channels[MAX_TRACKER_CHANNELS];
  Tracker_source source; // current editable source
  i32 source_id;
  i32 auto_load_source_from_id;
  Color background_color;
  Color highlight_color;
  // input colors
  Color active_color;
  Color inactive_color;
  size_t tick;
  size_t prev_tick;
  size_t active_row;
} Tracker;

static void tracker_default(Tracker* tracker);
static void tracker_channel_init(Tracker_channel* channel);
static void tracker_channels_init(Tracker* tracker);
static void tracker_row_update(Element* e);
static void tracker_sample_input_update(Element* e);
static void tracker_editor_update(Element* e);
static void decrement_source_id(Element* e);
static void increment_source_id(Element* e);
static void change_source_id(Tracker* tracker);
static void load_source(Tracker* tracker);
static void save_source(Tracker* tracker);
static void load_source_onclick(Element* e);
static void save_source_onclick(Element* e);
static void modify_source_id(Element* e);
static void modify_source_setting(Element* e);

void tracker_default(Tracker* tracker) {
  Tracker_source source = (Tracker_source) {
    .source = audio_source_empty(),
  };
  for (size_t i = 0; i < MAX_AUDIO_SOURCE; ++i) {
    tracker->sources[i] = source;
  }
  tracker_channels_init(tracker);
  tracker->source = tracker->sources[0];
  tracker->source_id = 0;
  tracker->auto_load_source_from_id = true;
  tracker->background_color = lerp_color(UI_BACKGROUND_COLOR, UI_INTERPOLATION_COLOR, 0.2f);
  tracker->highlight_color = brighten_color(saturate_color(UI_BUTTON_COLOR, -0.1f), 0.2f);
  tracker->active_color = lerp_color(UI_BACKGROUND_COLOR, UI_INTERPOLATION_COLOR, 0.2f);
  tracker->inactive_color = lerp_color(UI_BACKGROUND_COLOR, UI_INTERPOLATION_COLOR, 0.05f);
  tracker->tick = 0;
  tracker->active_row = 0;
}

void tracker_channel_init(Tracker_channel* channel) {
  for (i32 i = 0; i < MAX_TRACKER_ROW; ++i) {
    channel->rows[i] = (Tracker_row) {
      .id = i,
      .note = 0,
      .source_id = -1,
    };
  }
  channel->active_row = NULL;
  channel->freq = 1;
  channel->sample_index = 0;
  channel->state = SAMPLE_STATE_END;
}

void tracker_channels_init(Tracker* tracker) {
  for (size_t i = 0; i < MAX_TRACKER_CHANNELS; ++i) {
    tracker_channel_init(&tracker->channels[i]);
  }
}

void tracker_row_update(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  if ((i32)tracker->active_row == e->v.i) {
    e->background_color = tracker->highlight_color;
  }
  else {
    e->background_color = tracker->background_color;
  }
}

void tracker_sample_input_update(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  ASSERT(e->type == ELEMENT_INPUT);
  ASSERT(e->data.input.value != NULL);
  if (e->data.input.value_type == VALUE_TYPE_INTEGER) {
    i32 value = *(i32*)e->data.input.value;
    if (value >= 0 && value < MAX_AUDIO_SOURCE) {
      e->background_color = tracker->active_color;
      e->text_color = UI_TEXT_COLOR;
    }
    else {
      e->background_color = tracker->inactive_color;
      e->text_color = lerp_color(UI_TEXT_COLOR, invert_color(UI_INTERPOLATION_COLOR), 0.4f);
    }
  }
}

void tracker_editor_update(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  if (tracker->prev_tick != tracker->tick) {
    f32 progress = (tracker->tick % MAX_TRACKER_ROW) / (f32)MAX_TRACKER_ROW;
    // ui_scroll_container(e, progress);
  }
}

void decrement_source_id(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  if (tracker->source_id > 0) {
    tracker->source_id -= 1;
  }
  else {
    tracker->source_id = MAX_AUDIO_SOURCE - 1;
  }
  change_source_id(tracker);
}

void modify_source_setting(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  Tracker_source* src = &tracker->source;
  Tracker_source* dest = &tracker->sources[tracker->source_id];
  dest->settings = src->settings;
}

void increment_source_id(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  tracker->source_id = (tracker->source_id + 1) % MAX_AUDIO_SOURCE;
  change_source_id(tracker);
}

void change_source_id(Tracker* tracker) {
  tracker->source_id = CLAMP(tracker->source_id, 0, MAX_AUDIO_SOURCE - 1); // sanity check
  if (tracker->auto_load_source_from_id) {
    load_source(tracker);
  }
}

void load_source(Tracker* tracker) {
  Tracker_source* src = &tracker->sources[tracker->source_id];
  Tracker_source* dest = &tracker->source;
  Tracker_source dest_copy = *dest;
  Tracker_source src_copy = *src;

  audio_unload_audio(&dest->source);
  dest->source = audio_source_copy_into_new(src->source.buffer, src->source.samples, src->source.channel_count);
  dest->source.mutex = dest_copy.source.mutex;
  dest->settings = src_copy.settings;
}

void save_source(Tracker* tracker) {
  Tracker_source* src = &tracker->source;
  Tracker_source* dest = &tracker->sources[tracker->source_id];
  Tracker_source dest_copy = *dest;
  Tracker_source src_copy = *src;

  audio_unload_audio(&dest->source);
  dest->source = audio_source_copy_into_new(src->source.buffer, src->source.samples, src->source.channel_count);
  dest->source.mutex = dest_copy.source.mutex;
  dest->settings = src_copy.settings;
}

void load_source_onclick(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  load_source(tracker);
}

void save_source_onclick(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  save_source(tracker);
}

void modify_source_id(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  change_source_id(tracker);
}

void tracker_init(Instrument* ins) {
  (void)ins;
  Tracker* tracker = memory_alloc(sizeof(Tracker));
  ins->userdata = tracker;
  tracker_default(tracker);
}

void tracker_ui_new(Instrument* ins, Element* container) {
  (void)ins; (void)container;
  Tracker* tracker = (Tracker*)ins->userdata;
  const i32 input_width = FONT_SIZE + UI_X_PADDING / 2;
  const i32 line_height = FONT_SIZE + UI_Y_PADDING / 2;
  const i32 audio_canvas_width = 300;
  const i32 audio_source_height = 180;
  container->background_color = lerp_color(UI_BACKGROUND_COLOR, COLOR_RGB(255, 25, 255), 0.08f);

  Element audio_canvas = ui_audio_canvas("sample editor", audio_source_height, &tracker->source.source, false);
  audio_canvas.border = true;
  audio_canvas.background = true;
  audio_canvas.sizing.x_mode = SIZE_MODE_PIXELS;
  audio_canvas.sizing.x = audio_canvas_width;

  Element* canvas = ui_attach_element(container, &audio_canvas);

  {
    Element e = ui_text("sample id");
    e.sizing = SIZING_PERCENT(35, 0);
    ui_attach_element(canvas, &e);
  }
  {
    Element e = ui_input_int("sample id", &tracker->source_id);
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 25, .y = line_height, };
    e.userdata = tracker;
    e.onmodify = modify_source_id;
    e.tooltip = "audio sample/source id";
    ui_attach_element(canvas, &e);
  }
  {
    Element e = ui_button("<");
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 20, .y = line_height, };
    e.userdata = tracker;
    e.onclick = decrement_source_id;
    ui_attach_element(canvas, &e);
  }
  {
    Element e = ui_button(">");
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 20, .y = line_height, };
    e.userdata = tracker;
    e.onclick = increment_source_id;
    ui_attach_element(canvas, &e);
  }
  {
    Element e = ui_button("save");
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 50, .y = line_height, };
    e.userdata = tracker;
    e.onclick = save_source_onclick;
    ui_attach_element(canvas, &e);
  }
  {
    Element e = ui_button("load");
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 50, .y = line_height, };
    e.userdata = tracker;
    e.onclick = load_source_onclick;
    ui_attach_element(canvas, &e);
  }
  {
    Element e = ui_line_break(2);
    e.render = e.background = true;
    e.background_color = lerp_color(UI_BACKGROUND_COLOR, UI_INTERPOLATION_COLOR, 0.4f);
    ui_attach_element(canvas, &e);
  }
  {
    Element e = ui_text("start");
    e.sizing = SIZING_PERCENT(35, 0);
    ui_attach_element(canvas, &e);
  }
  {
    Element e = ui_input_int("start", &tracker->source.settings.start);
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 65, .y = line_height, };
    e.userdata = tracker;
    e.onmodify = modify_source_setting;
    e.tooltip = "start";
    ui_attach_element(canvas, &e);
  }
  {
    Element e = ui_text("length");
    e.sizing = SIZING_PERCENT(35, 0);
    ui_attach_element(canvas, &e);
  }
  {
    Element e = ui_input_int("length", &tracker->source.settings.length);
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 65, .y = line_height, };
    e.userdata = tracker;
    e.onmodify = modify_source_setting;
    e.tooltip = "length";
    ui_attach_element(canvas, &e);
  }
  {
    Element e = ui_toggle_ex(&tracker->source.settings.pingpong, "pingpong");
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 100, .y = line_height, };
    e.userdata = tracker;
    e.onmodify = modify_source_setting;
    ui_attach_element(canvas, &e);
  }

  ui_attach_element_v2(container, ui_line_break(0));

  const i32 width = 480;
  const i32 height = 400;
  Element* inner = NULL;
  {
    Element e = ui_container(NULL);
    e.background = true;
    e.border = true;
    e.sizing = (Sizing) {
      .x_mode = SIZE_MODE_PERCENT, .x = 100,
      .y_mode = SIZE_MODE_PIXELS, .y = height,
    };
    e.y_padding = 0;
    e.x_padding = 0;
    e.userdata = tracker;
    e.onupdate = tracker_editor_update;
    inner = ui_attach_element(container, &e);
  }
  for (size_t row_index = 0; row_index < MAX_TRACKER_ROW; ++row_index) {
    {
      Element e = ui_input_int(NULL, &tracker->channels[0].rows[row_index].id);
      e.readonly = true;
      e.background = true;
      e.background_color = tracker->background_color;
      e.border = true;
      e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .x = 5, .y_mode = SIZE_MODE_PIXELS, .y = line_height, };
      e.v.i = row_index;
      e.userdata = tracker;
      e.onupdate = tracker_row_update;
      e.tooltip = "position";
      ui_attach_element(inner, &e);
    }
    {
      Element e = ui_none();
      e.box = BOX(0, 0, line_height, line_height);
      ui_attach_element(inner, &e);
    }
    for (size_t channel_index = 0; channel_index < MAX_TRACKER_CHANNELS; ++channel_index) {
      Tracker_channel* channel = &tracker->channels[channel_index];
      Tracker_row* row = &channel->rows[row_index];
      {
        Element e = ui_input_int(NULL, &row->note);
        e.border = true;
        e.box = BOX(0, 0, input_width, line_height);
        e.tooltip = "note";
        e.v.i = row_index;
        e.userdata = tracker;
        e.onupdate = tracker_row_update;
        ui_attach_element(inner, &e);
      }
      {
        Element e = ui_input_int(NULL, &row->source_id);
        e.border = true;
        e.box = BOX(0, 0, input_width, line_height);
        e.tooltip = "sample id";
        e.v.i = row_index;
        e.userdata = tracker;
        e.onupdate = tracker_sample_input_update;
        ui_attach_element(inner, &e);
      }
      {
        Element e = ui_none();
        e.box = BOX(0, 0, line_height * 2, line_height);
        ui_attach_element(inner, &e);
      }
    }
    ui_attach_element_v2(inner, ui_line_break(0));
  }
}

void tracker_update(Instrument* ins, Mix* mix) {
  (void)ins; (void)mix;
  Tracker* tracker = (Tracker*)ins->userdata;
  tracker->prev_tick = tracker->tick;
  tracker->tick = mix->timed_tick;
  tracker->active_row = tracker->tick % MAX_TRACKER_ROW;

  if (tracker->prev_tick != tracker->tick) {
    for (size_t i = 0; i < MAX_TRACKER_CHANNELS; ++i) {
      Tracker_channel* channel = &tracker->channels[i];
      Tracker_row* row = &channel->rows[tracker->active_row];
      if (row->source_id >= 0 && row->source_id < MAX_AUDIO_SOURCE) {
        Tracker_source* src = &tracker->sources[row->source_id];
        f32 base_freq = freq_table[24]; // C2
        f32 freq = freq_table[row->note % LENGTH(freq_table)];
        channel->freq = freq / base_freq;
        channel->sample_index = src->settings.start;
        channel->state = SAMPLE_STATE_PLAY;
        channel->active_row = row;
      }
    }
  }

  for (size_t i = 0; i < MAX_TRACKER_CHANNELS; ++i) {
    Tracker_channel* channel = &tracker->channels[i];
    if (channel->active_row) {
      i32 id = channel->active_row->source_id;
      if (id == tracker->source_id) {
        tracker->source.source.cursor = (size_t)channel->sample_index;
      }
    }
  }

  if (!ui_input_interacting() && !IsKeyDown(KEY_LEFT_CONTROL)) {
    // add keyboard shortcut
  }
}

void tracker_process(Instrument* ins, Mix* mix, Audio_engine* audio, f32 dt) {
  (void)mix; (void)audio; (void)dt;
  Tracker* tracker = (Tracker*)ins->userdata;
  for (size_t i = 0; i < MAX_AUDIO_SOURCE; ++i) {
    Tracker_source* src = &tracker->sources[i];
    ticket_mutex_begin(&src->source.mutex);
  }

  memset(ins->out_buffer, 0, sizeof(f32) * ins->samples);

  for (size_t channel_index = 0; channel_index < MAX_TRACKER_CHANNELS; ++channel_index) {
    Tracker_channel* channel = &tracker->channels[channel_index];
    Tracker_row* row = channel->active_row;
    if (!row) {
      continue;
    }
    if (row->source_id >= 0 && row->source_id < MAX_AUDIO_SOURCE && channel->state != SAMPLE_STATE_END) {
      Tracker_source* src = &tracker->sources[row->source_id];
      if (src->source.buffer == NULL || src->source.samples == 0) {
        continue;
      }
      i32 sample_end = 0;
      if (src->settings.length == 0) {
        sample_end = src->source.samples;
      }
      else {
        sample_end = src->settings.start + src->settings.length;
      }

      for (size_t i = 0; i < ins->samples; ++i) {
        if (channel->sample_index >= sample_end || channel->sample_index >= src->source.samples) {
          if (src->settings.pingpong) {
            channel->sample_index = sample_end - 1;
            channel->freq = -channel->freq;
          }
          else {
            channel->state = SAMPLE_STATE_END;
            channel->active_row = NULL;
            break;
          }
        }
        if (channel->sample_index < src->settings.start || channel->sample_index < 0) {
          if (src->settings.pingpong) {
            channel->sample_index = src->settings.start;
            channel->freq = -channel->freq;
          }
          else {
            break;
          }
        }
        ins->out_buffer[i] += src->source.buffer[(size_t)channel->sample_index];
        channel->sample_index += channel->freq;
      }
    }
  }

  for (size_t i = 0; i < MAX_AUDIO_SOURCE; ++i) {
    Tracker_source* src = &tracker->sources[i];
    ticket_mutex_end(&src->source.mutex);
  }
}

void tracker_noteon(Instrument* ins, u8 note, f32 velocity) {
  (void)ins; (void)note; (void)velocity;
}

void tracker_noteoff(Instrument* ins, u8 note) {
  (void)ins; (void)note;
}

void tracker_destroy(Instrument* ins) {
  (void)ins;
  Tracker* tracker = (Tracker*)ins->userdata;

  Hash source_hash = hash_djb2((u8*)&tracker->source.source, sizeof(Audio_source));
  bool source_is_alias = false;
  for (size_t i = 0; i < MAX_AUDIO_SOURCE; ++i) {
    Tracker_source* src = &tracker->sources[i];
    if (!source_is_alias) {
      Hash hash = hash_djb2((u8*)&src->source, sizeof(Audio_source));
      source_is_alias = hash == source_hash;
    }
    audio_unload_audio(&src->source);
  }
  if (!source_is_alias) {
    audio_unload_audio(&tracker->source.source);
  }
}

#undef MAX_AUDIO_SOURCE
#undef MAX_TRACKER_ROW
#undef MAX_TRACKER_CHANNELS
