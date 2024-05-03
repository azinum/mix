// tracker.c
// TODO:
//  - fix memory leak
//  - fix saving/loading of patterns when paused

#define MAX_AUDIO_SOURCE 128
#define MAX_TRACKER_ROW 32
#define MAX_TRACKER_CHANNELS 8
#define MAX_PATTERN 128
#define MAX_SONG_PATTERN_SEQUENCE 180

#define ROOT_NOTE 24

typedef enum {
  SAMPLE_STATE_END,
  SAMPLE_STATE_PLAY,
  MAX_SAMPLE_STATE,
} Sample_state;

#define TRACKER_SOURCE_NAME_LENGTH 24

typedef struct Tracker_source {
  Audio_source source;
  struct {
    i32 start;
    i32 length;
    i32 pingpong;
    char name[TRACKER_SOURCE_NAME_LENGTH];
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

typedef struct Pattern {
  Tracker_channel channels[MAX_TRACKER_CHANNELS];
  bool dirty;
} Pattern;

typedef struct Song {
  i32 pattern_sequence[MAX_SONG_PATTERN_SEQUENCE];
  i32 length;
  i32 index;
} Song;

typedef struct Tracker {
  Tracker_source sources[MAX_AUDIO_SOURCE];
  Tracker_source source; // current editable source
  i32 source_id;
  i32 auto_load_source_from_id;
  size_t tick;
  size_t prev_tick;
  size_t active_row;
  i32 follow_pattern;
  i32 loop_pattern;

  Pattern patterns[MAX_PATTERN];
  Pattern pattern;
  Pattern pattern_copy;
  i32 pattern_id;
  i32 prev_pattern_id;

  Song song;
  Element* song_editor;

  Color background_color;
  Color highlight_color;
  // input colors
  Color active_color;
  Color inactive_color;
} Tracker;

static void tracker_default(Tracker* tracker);
static void tracker_channel_init(Tracker_channel* channel);
static void tracker_pattern_init(Pattern* pattern);
static void tracker_patterns_init(Tracker* tracker);
static void tracker_song_init(Song* song);
static void tracker_row_update(Element* e);
static void tracker_sample_input_update(Element* e);
static void tracker_editor_update(Element* e);
static void tracker_copy_pattern(Tracker* tracker);
static void tracker_paste_pattern(Tracker* tracker);
static void tracker_add_pattern_in_song(Tracker* tracker);
static void add_pattern_in_song(Element* e);
static void modify_song_index(Element* e);
static void song_pattern_row_update(Element* e);
static void decrement_source_id(Element* e);
static void increment_source_id(Element* e);
static void change_source_id(Tracker* tracker);
static void load_source(Tracker* tracker);
static void save_source(Tracker* tracker);
static void save_source_onclick(Element* e);
static void modify_source_id(Element* e);
static void modify_source_setting(Element* e);
static void decrement_pattern_id(Element* e);
static void increment_pattern_id(Element* e);
static void modify_pattern_id(Element* e);
static void change_pattern_id(Tracker* tracker);
static void modify_item_in_pattern_editor(Element* e);
static void load_pattern(Tracker* tracker);
static void save_pattern(Tracker* tracker);
static void copy_pattern(Element* e);
static void paste_pattern(Element* e);

void tracker_default(Tracker* tracker) {
  Tracker_source source = (Tracker_source) {
    .source = audio_source_empty(),
    .settings = {
      .start  = 0,
      .length = 0,
      .pingpong = 0,
      .name = {0},
    },
  };
  for (size_t i = 0; i < MAX_AUDIO_SOURCE; ++i) {
    tracker->sources[i] = source;
  }
  tracker->source = tracker->sources[0];
  tracker->source_id = 0;
  tracker->auto_load_source_from_id = true;
  tracker->tick = 0;
  tracker->prev_tick = 0;
  tracker->active_row = 0;
  tracker->follow_pattern = false;
  tracker->loop_pattern = true;

  tracker_patterns_init(tracker);
  tracker->pattern_id = 0;
  tracker->prev_pattern_id = 0;

  tracker_song_init(&tracker->song);
  tracker->song_editor = NULL;

  tracker->background_color = lerp_color(UI_BACKGROUND_COLOR, UI_INTERPOLATION_COLOR, 0.2f);
  tracker->highlight_color = brighten_color(saturate_color(UI_BUTTON_COLOR, -0.1f), 0.2f);
  tracker->active_color = lerp_color(UI_BACKGROUND_COLOR, UI_INTERPOLATION_COLOR, 0.2f);
  tracker->inactive_color = lerp_color(UI_BACKGROUND_COLOR, UI_INTERPOLATION_COLOR, 0.05f);
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

void tracker_pattern_init(Pattern* pattern) {
  for (size_t i = 0; i < MAX_TRACKER_CHANNELS; ++i) {
    tracker_channel_init(&pattern->channels[i]);
  }
  pattern->dirty = false;
}

void tracker_patterns_init(Tracker* tracker) {
  tracker_pattern_init(&tracker->pattern);
  for (size_t i = 0; i < MAX_PATTERN; ++i) {
    tracker_pattern_init(&tracker->patterns[i]);
  }
}

void tracker_song_init(Song* song) {
  memset(song->pattern_sequence, 0, sizeof(song->pattern_sequence));
  song->length = 0;
  song->index = 0;
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
  if (!tracker->follow_pattern) {
    return;
  }
  if (tracker->prev_tick != tracker->tick) {
    f32 progress = (tracker->tick % MAX_TRACKER_ROW) / (f32)MAX_TRACKER_ROW;
    ui_scroll_container(e, progress);
  }
}

void tracker_copy_pattern(Tracker* tracker) {
  memcpy(&tracker->pattern_copy, &tracker->pattern, sizeof(Pattern));
}

void tracker_paste_pattern(Tracker* tracker) {
  memcpy(&tracker->pattern, &tracker->pattern_copy, sizeof(Pattern));
  tracker->pattern.dirty = true;
}

void tracker_add_pattern_in_song(Tracker* tracker) {
  ASSERT(tracker->song_editor);
  Song* song = &tracker->song;
  song->index = (u32)song->index % MAX_SONG_PATTERN_SEQUENCE;
  song->length = CLAMP(song->length, 0, MAX_SONG_PATTERN_SEQUENCE - 1);

  const i32 line_height = FONT_SIZE + UI_Y_PADDING / 2;
  Element e = ui_input_int("pattern id", &song->pattern_sequence[song->length]);
  e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .x = 100, .y_mode = SIZE_MODE_PIXELS, .y = line_height, };
  e.v.i = song->length;
  e.userdata = tracker;
  e.onupdate = song_pattern_row_update;
  ui_attach_element(tracker->song_editor, &e);
  song->length += 1;
  song->length = CLAMP(song->length, 0, MAX_SONG_PATTERN_SEQUENCE - 1);
}

void add_pattern_in_song(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  tracker_add_pattern_in_song(tracker);
}

void modify_song_index(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  tracker->song.index = (u32)tracker->song.index % MAX_SONG_PATTERN_SEQUENCE;
  tracker->song.length = CLAMP(tracker->song.length, 0, MAX_SONG_PATTERN_SEQUENCE - 1);
}

void song_pattern_row_update(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  Song* song = &tracker->song;
  if ((e->v.i % MAX_SONG_PATTERN_SEQUENCE) == song->index) {
    e->background_color = tracker->highlight_color;
  }
  else {
    e->background_color = tracker->background_color;
  }
}

void decrement_source_id(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  tracker->source_id -= 1;
  tracker->source_id = (u32)tracker->source_id % MAX_AUDIO_SOURCE;
  change_source_id(tracker);
}

void increment_source_id(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  tracker->source_id = (tracker->source_id + 1) % MAX_AUDIO_SOURCE;
  change_source_id(tracker);
}

void change_source_id(Tracker* tracker) {
  tracker->source_id = (u32)tracker->source_id % MAX_AUDIO_SOURCE;
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

void save_source_onclick(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  save_source(tracker);
}

void modify_source_id(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  change_source_id(tracker);
}

void modify_source_setting(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  Tracker_source* src = &tracker->source;
  Tracker_source* dest = &tracker->sources[tracker->source_id];
  dest->settings = src->settings;
}

void decrement_pattern_id(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  tracker->pattern_id -= 1;
  tracker->pattern_id = (u32)tracker->pattern_id % MAX_PATTERN;
  change_pattern_id(tracker);
}

void increment_pattern_id(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  tracker->pattern_id += 1;
  change_pattern_id(tracker);
}

void modify_pattern_id(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  change_pattern_id(tracker);
}

void change_pattern_id(Tracker* tracker) {
  tracker->pattern_id = (u32)tracker->pattern_id % MAX_PATTERN;
}

void modify_item_in_pattern_editor(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  tracker->pattern.dirty = true;
}

void load_pattern(Tracker* tracker) {
  ASSERT(tracker->pattern_id >= 0 && tracker->pattern_id < MAX_PATTERN);
  ASSERT(tracker->patterns[tracker->pattern_id].dirty == false);

  memcpy(&tracker->pattern, &tracker->patterns[tracker->pattern_id], sizeof(Pattern));
}

void save_pattern(Tracker* tracker) {
  tracker->pattern.dirty = false;
  ASSERT(tracker->pattern_id >= 0 && tracker->pattern_id < MAX_PATTERN);
  memcpy(&tracker->patterns[tracker->pattern_id], &tracker->pattern, sizeof(Pattern));
}

void copy_pattern(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  tracker_copy_pattern(tracker);
}

void paste_pattern(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  tracker_paste_pattern(tracker);
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
  const i32 audio_source_height = 180;
  container->background_color = lerp_color(UI_BACKGROUND_COLOR, COLOR_RGB(255, 25, 255), 0.08f);

  // sample editor
  Element audio_canvas = ui_audio_canvas("sample editor", audio_source_height, &tracker->source.source, false);
  audio_canvas.border = true;
  audio_canvas.background = true;
  audio_canvas.sizing.x_mode = SIZE_MODE_PERCENT;
  audio_canvas.sizing.x = 50;

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
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 100, .y = line_height, };
    e.userdata = tracker;
    e.onclick = save_source_onclick;
    ui_attach_element(canvas, &e);
  }

  {
    Element e = ui_line_break(2);
    e.render = e.background = true;
    e.background_color = lerp_color(UI_BACKGROUND_COLOR, UI_INTERPOLATION_COLOR, 0.4f);
    ui_attach_element(canvas, &e);
  }
  {
    Element e = ui_text("name");
    e.sizing = SIZING_PERCENT(35, 0);
    ui_attach_element(canvas, &e);
  }
  {
    Element e = ui_input_text("name", (char*)&tracker->source.settings.name[0], TRACKER_SOURCE_NAME_LENGTH);
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 65, .y = line_height, };
    e.userdata = tracker;
    e.onmodify = modify_source_setting;
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

  // song editor/pattern sequencer

  Element song_container = ui_container("song editor");
  song_container.background = true;
  song_container.border = true;
  song_container.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 50, .y = 2 * audio_source_height, };
  tracker->song_editor = ui_attach_element(container, &song_container);

  {
    Element e = ui_button("+");
    e.sizing = SIZING_PIXELS(input_width, line_height);
    e.userdata = tracker;
    e.onclick = add_pattern_in_song;
    ui_attach_element(tracker->song_editor, &e);
  }
  {
    Element e = ui_input_int("index", &tracker->song.index);
    e.sizing = SIZING_PIXELS(input_width * 3, line_height);
    e.userdata = tracker;
    e.onmodify = modify_song_index;
    ui_attach_element(tracker->song_editor, &e);
  }
  {
    Element e = ui_line_break(2);
    e.render = e.background = true;
    e.background_color = lerp_color(UI_BACKGROUND_COLOR, UI_INTERPOLATION_COLOR, 0.4f);
    ui_attach_element(tracker->song_editor, &e);
  }

  // pattern list here

  tracker_add_pattern_in_song(tracker);

  Element outer = ui_container(NULL);
  outer.background = false;
  outer.border = false;
  outer.sizing = (Sizing) {
    .x_mode = SIZE_MODE_PERCENT, .x = 100,
    .y_mode = SIZE_MODE_PIXELS,  .y = 0,
  };
  outer.data.container.auto_adjust_height = true;

  // control panel/settings
  Element settings = ui_container("control panel");
  settings.background = true;
  settings.border = true;
  settings.sizing = (Sizing) {
    .x_mode = SIZE_MODE_PERCENT, .x = 100,
    .y_mode = SIZE_MODE_PIXELS,  .y = 0,
  };
  settings.data.container.auto_adjust_height = true;

  {
    Element e = ui_text("pattern");
    e.sizing = SIZING_PIXELS(input_width * 4, line_height);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_input_int("pattern id", &tracker->pattern_id);
    e.sizing = SIZING_PIXELS(input_width * 2, line_height);
    e.userdata = tracker;
    e.onmodify = modify_pattern_id;
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_button("<");
    e.sizing = SIZING_PIXELS(input_width, line_height);
    e.userdata = tracker;
    e.onclick = decrement_pattern_id;
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_button(">");
    e.sizing = SIZING_PIXELS(input_width, line_height);
    e.userdata = tracker;
    e.onclick = increment_pattern_id;
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_button("copy");
    e.sizing = SIZING_PIXELS(input_width * 2, line_height);
    e.userdata = tracker;
    e.onclick = copy_pattern;
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_button("paste");
    e.sizing = SIZING_PIXELS(input_width * 2, line_height);
    e.userdata = tracker;
    e.onclick = paste_pattern;
    ui_attach_element(&settings, &e);
  }

  ui_attach_element_v2(&settings, ui_line_break(0));
  {
    Element e = ui_toggle_ex(&tracker->follow_pattern, "follow");
    e.sizing = SIZING_PIXELS(input_width * 4, line_height);
    e.tooltip = "follow pattern editor cursor";
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_toggle_ex(&tracker->loop_pattern, "loop");
    e.sizing = SIZING_PIXELS(input_width * 4, line_height);
    e.tooltip = "loop current pattern";
    ui_attach_element(&settings, &e);
  }

  ui_attach_element(&outer, &settings);

  // pattern editor
  const i32 height = 380;
  Element* inner = NULL;
  {
    Element e = ui_container("pattern editor - 8 channels");
    e.background = true;
    e.border = true;
    e.sizing = (Sizing) {
      .x_mode = SIZE_MODE_PERCENT, .x = 100,
      .y_mode = SIZE_MODE_PIXELS,  .y = height,
    };
    e.x_padding = 0;
    e.y_padding = 0;
    e.userdata = tracker;
    e.onupdate = tracker_editor_update;
    inner = ui_attach_element(&outer, &e);
  }
  const i32 subdivision = 4;
  for (size_t row_index = 0; row_index < MAX_TRACKER_ROW; ++row_index) {
    {
      Element e = ui_input_int(NULL, &tracker->pattern.channels[0].rows[row_index].id);
      e.readonly = true;
      e.background = true;
      e.background_color = tracker->background_color;
      e.border = true;
      e.box = BOX(0, 0, input_width, line_height);
      e.v.i = row_index;
      e.userdata = tracker;
      e.onupdate = tracker_row_update;
      e.tooltip = "position";
      ui_attach_element(inner, &e);
    }
    {
      Element e = ui_none();
      e.box = BOX(0, 0, line_height, line_height);
      if (!(row_index % subdivision)) {
        e.render = true;
        e.background = true;
        e.background_color = lerp_color(UI_BACKGROUND_COLOR, invert_color(UI_INTERPOLATION_COLOR), 0.35f);
      }
      ui_attach_element(inner, &e);
    }
    for (size_t channel_index = 0; channel_index < MAX_TRACKER_CHANNELS; ++channel_index) {
      Tracker_channel* channel = &tracker->pattern.channels[channel_index];
      Tracker_row* row = &channel->rows[row_index];
      {
        Element e = ui_input_int(NULL, &row->note);
        e.border = true;
        e.box = BOX(0, 0, 1.5 * input_width, line_height);
        e.tooltip = "note";
        e.v.i = row_index;
        e.userdata = tracker;
        e.onupdate = tracker_row_update;
        e.onmodify = modify_item_in_pattern_editor;
        ui_attach_element(inner, &e);
      }
      {
        Element e = ui_input_int(NULL, &row->source_id);
        e.border = true;
        e.box = BOX(0, 0, 1.5 * input_width, line_height);
        e.tooltip = "sample id";
        e.v.i = row_index;
        e.userdata = tracker;
        e.onupdate = tracker_sample_input_update;
        e.onmodify = modify_item_in_pattern_editor;
        ui_attach_element(inner, &e);
      }
      {
        Element e = ui_none();
        e.box = BOX(0, 0, line_height * 2, line_height);
        if (!(row_index % subdivision)) {
          e.render = true;
          e.background = true;
          e.background_color = lerp_color(UI_BACKGROUND_COLOR, invert_color(UI_INTERPOLATION_COLOR), 0.35f);
        }
        ui_attach_element(inner, &e);
      }
    }
    ui_attach_element_v2(inner, ui_line_break(0));
  }

  ui_attach_element(container, &outer);
}

void tracker_update(Instrument* ins, Mix* mix) {
  (void)ins; (void)mix;
  Tracker* tracker = (Tracker*)ins->userdata;
  tracker->prev_tick = tracker->tick;
  tracker->tick = mix->timed_tick;
  tracker->active_row = tracker->tick % MAX_TRACKER_ROW;

  Song* song = &tracker->song;

  if (tracker->prev_tick != tracker->tick) {
    if ((tracker->tick % MAX_TRACKER_ROW) == 0 && !tracker->loop_pattern) {
      song->index += 1;
      song->index = ((u32)song->index % MAX_SONG_PATTERN_SEQUENCE) % song->length;
      tracker->pattern_id = song->pattern_sequence[song->index];
      change_pattern_id(tracker);
    }

    if (tracker->prev_pattern_id != tracker->pattern_id) {
      i32 pattern_id = tracker->pattern_id;
      tracker->pattern_id = tracker->prev_pattern_id;
      save_pattern(tracker);
      tracker->pattern_id = pattern_id;
      load_pattern(tracker);
    }

    tracker->prev_pattern_id = tracker->pattern_id;
    for (size_t i = 0; i < MAX_TRACKER_CHANNELS; ++i) {
      Tracker_channel* channel = &tracker->pattern.channels[i];
      Tracker_row* row = &channel->rows[tracker->active_row];
      if (row->source_id >= 0 && row->source_id < MAX_AUDIO_SOURCE) {
        Tracker_source* src = &tracker->sources[row->source_id];
        f32 base_freq = freq_table[ROOT_NOTE]; // C2
        f32 freq = freq_table[row->note % LENGTH(freq_table)];
        if (row->note < 0) {
          freq = -row->note / 100.0f;
        }
        channel->freq = freq / base_freq;
        if (src->source.samples > 0) {
          channel->sample_index = src->settings.start % src->source.samples;
        }
        else {
          channel->sample_index = src->settings.start;
        }
        channel->state = SAMPLE_STATE_PLAY;
        channel->active_row = row;
      }
    }
  }

  for (size_t i = 0; i < MAX_TRACKER_CHANNELS; ++i) {
    Tracker_channel* channel = &tracker->pattern.channels[i];
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
    Tracker_channel* channel = &tracker->pattern.channels[channel_index];
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
#undef MAX_PATTERN
#undef MAX_SONG_PATTERN_SEQUENCE
#undef ROOT_NOTE
