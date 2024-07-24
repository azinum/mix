// tracker.c
// TODO:
//  - fix saving/loading of patterns when paused
//  - drag and drop to load songs
//  - fix random crash when loading audio files

#define MAX_AUDIO_SOURCE 128
#define MAX_TRACKER_ROW 64
#define MAX_TRACKER_CHANNELS 8
#define MAX_PATTERN 128
#define MAX_SONG_PATTERN_SEQUENCE 160

#define ROOT_NOTE 24

#define TRACKER_VERSION 0
#define TRACKER_MAGIC   0xbeef

#define ANIMATION_SPEED 15.0f
#define FREQ_INTERP_SPEED 200.0f

#define TRACKER_FILE_EXT "tp" // i.e. tracker project

static const char* channel_str[MAX_TRACKER_CHANNELS] = {
  "ch #0",
  "ch #1",
  "ch #2",
  "ch #3",
  "ch #4",
  "ch #5",
  "ch #6",
  "ch #7",
};

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
    f32 gain;
    i32 pingpong;
    char name[TRACKER_SOURCE_NAME_LENGTH];
  } settings;
} Tracker_source;

typedef struct Tracker_row {
  i16 id;
  i16 note;
  i16 source_id;
} Tracker_row;

typedef struct Tracker_channel {
  Tracker_row rows[MAX_TRACKER_ROW];
  i32 active_row;
  f32 freq;
  f32 sample_index;
  f32 freq_target;
  Sample_state state;
} Tracker_channel;

typedef struct Pattern {
  Tracker_channel channels[MAX_TRACKER_CHANNELS];
  bool dirty;
} Pattern;

typedef struct Song_step {
  i16 pattern_id;
  i16 loop; // number of times to loop the pattern
  i16 animation;
} Song_step;

typedef struct Song {
  Song_step pattern_sequence[MAX_SONG_PATTERN_SEQUENCE];
  i32 length;
  i32 index;
  i16 loop_counter;
  char name[24];
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
  i32 muted_channels[MAX_TRACKER_CHANNELS];
  f32 channel_panning[MAX_TRACKER_CHANNELS];

  Pattern patterns[MAX_PATTERN];
  Pattern pattern;
  Pattern pattern_copy;
  i16 pattern_id;
  i16 prev_pattern_id;

  Song song;
  Element* song_editor;

  Instrument* self;

  f32 dt;
  i32 samples_per_subtick;

  i32 bpm;

  Color background_color;
  Color highlight_color;
  // input colors
  Color active_color;
  Color inactive_color;

  Color loop_highlight_color;
} Tracker;

// header
// tracker state
// sample data locations
// sample data
typedef struct Tracker_header {
  u16 version;
  u16 magic;
  size_t sample_data_start;
  size_t sample_data_size;
  // Hash checksum;
} Tracker_header;

// HACK: to not contaminate Tracker struct with additional fields
static size_t noteon_tick = 0;
static u8 noteon_note = 0;

static void tracker_default(Tracker* tracker, Mix* mix);
static void tracker_channel_init(Tracker_channel* channel);
static void tracker_pattern_init(Pattern* pattern);
static void tracker_patterns_init(Tracker* tracker);
static void tracker_song_init(Song* song);
static void tracker_row_update(Element* e);
static void tracker_hover_note(Element* e);
static void tracker_hover_sample_id(Element* e);
static void tracker_sample_input_update(Element* e);
static void tracker_editor_update(Element* e);
static void tracker_copy_pattern(Tracker* tracker);
static void tracker_paste_pattern(Tracker* tracker);
static void tracker_modify_song_index(Tracker* tracker);
static void tracker_change_pattern(Tracker* tracker);
static Result tracker_save_song(Tracker* tracker);
static Result tracker_load_song(Tracker* tracker);
static Result tracker_insert_pattern_in_song(Tracker* tracker, size_t index);
static void add_pattern_in_song(Element* e);
static void modify_song_index(Element* e);
static void decrement_song_index(Element* e);
static void increment_song_index(Element* e);
static void song_pattern_row_update(Element* e);
static void decrement_source_id(Element* e);
static void increment_source_id(Element* e);
static void change_source_id(Tracker* tracker);
static void load_source(Tracker* tracker);
static void save_source(Tracker* tracker);
static void kill_source(Tracker* tracker);
static void save_source_onclick(Element* e);
static void kill_source_onclick(Element* e);
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
static void save_song(Element* e);
static void load_song(Element* e);
static void restart_song(Element* e);
static void modify_bpm(Element* e);
static void decrement_bpm(Element* e);
static void increment_bpm(Element* e);

void tracker_default(Tracker* tracker, Mix* mix) {
  Tracker_source source = (Tracker_source) {
    .source = audio_source_empty(),
    .settings = {
      .start  = 0,
      .length = 0,
      .gain = 1,
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
  for (size_t i = 0; i < MAX_TRACKER_CHANNELS; ++i) {
    tracker->muted_channels[i] = false;
    tracker->channel_panning[i] = 0.5f;
  }

  tracker_patterns_init(tracker);
  tracker->pattern_id = 0;
  tracker->prev_pattern_id = 0;

  tracker_song_init(&tracker->song);
  tracker->song_editor = NULL;
  tracker->self = NULL;

  tracker->dt = 0;
  tracker->samples_per_subtick = 0;

  tracker->bpm = mix->bpm;

  tracker->background_color = lerp_color(UI_BACKGROUND_COLOR, UI_INTERPOLATION_COLOR, 0.2f);
  tracker->highlight_color  = brighten_color(saturate_color(UI_BUTTON_COLOR, -0.1f), 0.2f);
  tracker->active_color     = lerp_color(UI_BACKGROUND_COLOR, UI_INTERPOLATION_COLOR, 0.2f);
  tracker->inactive_color   = lerp_color(UI_BACKGROUND_COLOR, UI_INTERPOLATION_COLOR, 0.05f);

  tracker->loop_highlight_color = brighten_color(saturate_color(UI_BUTTON_COLOR, 0.1f), 0.6f);
}

void tracker_channel_init(Tracker_channel* channel) {
  for (i32 i = 0; i < MAX_TRACKER_ROW; ++i) {
    channel->rows[i] = (Tracker_row) {
      .id = i,
      .note = 0,
      .source_id = -1,
    };
  }
  channel->active_row = -1;
  channel->freq = 1;
  channel->freq_target = 1;
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
  song->length = 1;
  song->index = 0;
  snprintf(song->name, sizeof(song->name), "untitled");
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

void tracker_hover_note(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  i16* note_value = (i16*)e->data.input.value;
  ASSERT(value != NULL);
  if (noteon_tick == mix_get_tick()) {
    *note_value = noteon_note;
    noteon_tick = (size_t)-1;
  }
}

void tracker_hover_sample_id(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  (void)tracker;
}

void tracker_sample_input_update(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  ASSERT(e->type == ELEMENT_INPUT);
  ASSERT(e->data.input.value != NULL);
  if (e->data.input.value_type == VALUE_TYPE_INT32) {
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

void tracker_modify_song_index(Tracker* tracker) {
  if (tracker->song.index < 0) {
    tracker->song.index = MAX_SONG_PATTERN_SEQUENCE - 1;
  }
  if (tracker->song.index >= MAX_SONG_PATTERN_SEQUENCE || tracker->song.index >= tracker->song.length) {
    tracker->song.index = 0;
  }

  tracker->song.length = CLAMP(tracker->song.length, 0, MAX_SONG_PATTERN_SEQUENCE - 1);
  tracker->song.loop_counter = 0;
  Song_step* step = &tracker->song.pattern_sequence[tracker->song.index];
  tracker->pattern_id = step->pattern_id;
  mix_reset_tick();
  tracker->active_row = 0;
}

void tracker_change_pattern(Tracker* tracker) {
  i16 pattern_id = tracker->pattern_id;
  tracker->pattern_id = tracker->prev_pattern_id;
  save_pattern(tracker);
  tracker->pattern_id = pattern_id;
  load_pattern(tracker);
  tracker->active_row = 0;
  mix_reset_tick();
}

Result tracker_save_song(Tracker* tracker) {
  char path[MAX_PATH_LENGTH] = {0};
  snprintf(path, MAX_PATH_LENGTH, "%s.%s", tracker->song.name, TRACKER_FILE_EXT);

  i32 fd = open(path, O_WRONLY | O_TRUNC | O_CREAT, 0664);
  if (fd < 0) {
    return Error;
  }

  // 1) write header
  size_t sample_data_size = 0;
  for (size_t i = 0; i < MAX_AUDIO_SOURCE; ++i) {
    Tracker_source* src = &tracker->sources[i];
    sample_data_size += src->source.samples * sizeof(i16);
  }
  Tracker_header header = (Tracker_header) {
    .version = TRACKER_VERSION,
    .magic = TRACKER_MAGIC,
    .sample_data_start = sizeof(Tracker_header) + sizeof(Tracker) + sizeof(size_t) * MAX_AUDIO_SOURCE,
    .sample_data_size = sample_data_size,
  };

  write(fd, &header, sizeof(header));

  // 2) write tracker state
  write(fd, tracker, sizeof(Tracker));

  // 3) write sample data/audio sources data locations
  size_t sample_data_index = header.sample_data_start;
  for (size_t i = 0; i < MAX_AUDIO_SOURCE; ++i) {
    Tracker_source* src = &tracker->sources[i];
    write(fd, &sample_data_index, sizeof(sample_data_index));
    sample_data_index += src->source.samples * sizeof(i16);
  }

  // 4) write sample data
  for (size_t i = 0; i < MAX_AUDIO_SOURCE; ++i) {
    Tracker_source* src = &tracker->sources[i];
    for (size_t sample_index = 0; sample_index < src->source.samples; ++sample_index) {
      i16 sample = src->source.buffer[sample_index] * INT16_MAX;
      write(fd, &sample, sizeof(sample));
    }
    sample_data_index += src->source.samples * sizeof(i16);
  }

  // 5) profit?
  close(fd);
  return Ok;
}

Result tracker_load_song(Tracker* tracker) {
  Result result = Ok;

  char path[MAX_PATH_LENGTH] = {0};
  snprintf(path, MAX_PATH_LENGTH, "%s.%s", tracker->song.name, TRACKER_FILE_EXT);

  Buffer save = buffer_new_from_file(path);
  if (!save.data) {
    return Error;
  }
#define READ(p, size) \
  if (read_index + size > save.count) { \
    log_print(STDERR_FILENO, LOG_TAG_ERROR, "tracker_load_song: tried to read data outside the save file `%s` from %zu to %zu\n", path, read_index, read_index + size); \
    return_defer(Error); \
  } \
  memcpy((p), &save.data[read_index], size); read_index += size

  size_t read_index = 0;
  size_t total_size = 0;
  Tracker_header header;
  Tracker tracker_state;

  READ(&header, sizeof(Tracker_header));

  total_size = sizeof(Tracker_header) + header.sample_data_size + (sizeof(size_t) * MAX_AUDIO_SOURCE) + sizeof(Tracker);

  if (total_size != save.count) {
    log_print(STDERR_FILENO, LOG_TAG_ERROR, "tracker_load_song: invalid file size in `%s`\n", path);
    return_defer(Error);
  }
  if (header.version != TRACKER_VERSION) {
    log_print(STDERR_FILENO, LOG_TAG_ERROR, "tracker_load_song: invalid version (got %u, but expected %u)\n", path, header.version, TRACKER_VERSION);
    return_defer(Error);
  }
  if (header.magic != TRACKER_MAGIC) {
    log_print(STDERR_FILENO, LOG_TAG_ERROR, "tracker_load_song: invalid magic (got %u, but expected %u)\n", path, header.magic, TRACKER_MAGIC);
    return_defer(Error);
  }

  READ(&tracker_state, sizeof(Tracker));
  tracker_state.song_editor = NULL;
  tracker_state.self = tracker->self;
  tracker_state.source = (Tracker_source) {
    .source = audio_source_empty(),
    .settings = {
      .start  = 0,
      .length = 0,
      .gain = 1,
      .pingpong = 0,
      .name = {0},
    },
  };
  tracker_state.source.source.mutex = tracker->source.source.mutex;

  for (size_t i = 0; i < MAX_AUDIO_SOURCE; ++i) {
    Tracker_source* src = &tracker_state.sources[i];
    src->source.buffer = NULL;
    src->source.mutex = tracker->sources[i].source.mutex;
    size_t index = 0;
    READ(&index, sizeof(index));
    if (src->source.samples == 0) {
      continue;
    }
    if (index >= save.count) {
      return_defer(Error);
    }
    src->source = audio_source_new_from_i16_buffer((i16*)&save.data[index], src->source.samples, src->source.channel_count);
  }

  printf(
    "version           = %u\n"
    "magic             = %x\n"
    "sample_data_start = %zu\n"
    "sample_data_size  = %zu\n"
    "total             = %zu\n"
    ,
    header.version,
    header.magic,
    header.sample_data_start,
    header.sample_data_size,
    total_size
  );

  for (size_t i = 0; i < MAX_AUDIO_SOURCE; ++i) {
    Tracker_source* src = &tracker->sources[i];
    audio_unload_audio(&src->source);
  }
  audio_unload_audio(&tracker->source.source);

  *tracker = tracker_state;
  mix_reload_ui();
  mix_set_bpm(tracker->bpm);
defer:
  buffer_free(&save);
#undef READ
  return result;
}

Result tracker_insert_pattern_in_song(Tracker* tracker, size_t index) {
  ASSERT(tracker->song_editor);
  Song* song = &tracker->song;
  song->index = (u32)song->index % MAX_SONG_PATTERN_SEQUENCE;
  if (index < MAX_SONG_PATTERN_SEQUENCE) {
    const i32 line_height = FONT_SIZE + UI_Y_PADDING / 2;
    song->pattern_sequence[song->length].pattern_id = tracker->pattern_id;
    song->pattern_sequence[song->length].loop = 0;
    {
      Element e = ui_input_int16("pattern id", &song->pattern_sequence[index].pattern_id);
      e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .x = 75, .y_mode = SIZE_MODE_PIXELS, .y = line_height, };
      e.v.i = index;
      e.userdata = tracker;
      e.onupdate = song_pattern_row_update;
      e.tooltip = "pattern id";
      ui_attach_element(tracker->song_editor, &e);
    }
    {
      Element e = ui_input_int16("loop", &song->pattern_sequence[index].loop);
      e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .x = 25, .y_mode = SIZE_MODE_PIXELS, .y = line_height, };
      e.v.i = index;
      e.userdata = tracker;
      e.onupdate = song_pattern_row_update;
      e.tooltip = "number of times to loop this pattern";
      ui_attach_element(tracker->song_editor, &e);
    }
    return Ok;
  }
  return Error;
}

void add_pattern_in_song(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  if (tracker_insert_pattern_in_song(tracker, tracker->song.length) == Ok) {
    tracker->song.length += 1;
  }
  tracker->song.length = CLAMP(tracker->song.length, 0, MAX_SONG_PATTERN_SEQUENCE - 1);
}

void modify_song_index(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  tracker_modify_song_index(tracker);
}

void decrement_song_index(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  tracker->song.index -= 1;
  tracker_modify_song_index(tracker);
}

void increment_song_index(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  tracker->song.index += 1;
  tracker_modify_song_index(tracker);
}

void song_pattern_row_update(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  Song* song = &tracker->song;

  i32 index = ((u32)e->v.i % MAX_SONG_PATTERN_SEQUENCE);
  Song_step* step = &song->pattern_sequence[index];
  if (step->animation > 0) {
    e->background_color = tracker->loop_highlight_color;
    step->animation -= 1;
  }

  if (index == song->index) {
    e->background_color = lerp_color(e->background_color, tracker->highlight_color, tracker->dt * ANIMATION_SPEED);
  }
  else {
    e->background_color = lerp_color(e->background_color, tracker->background_color, tracker->dt * ANIMATION_SPEED);
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

void kill_source(Tracker* tracker) {
  Tracker_source* src = &tracker->sources[tracker->source_id];
  audio_unload_audio(&src->source);
  load_source(tracker);
}

void save_source_onclick(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  save_source(tracker);
}

void kill_source_onclick(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  kill_source(tracker);
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
  tracker->pattern_id = (u16)tracker->pattern_id % MAX_PATTERN;
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
  tracker->pattern_id = (u16)tracker->pattern_id % MAX_PATTERN;
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

void save_song(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  if (tracker_save_song(tracker) != Ok) {
    ui_alert("failed to save song '%s.tp'", tracker->song.name);
    return;
  }
  ui_alert("song '%s.tp' saved", tracker->song.name);
}

void load_song(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  if (tracker_load_song(tracker) != Ok) {
    ui_alert("failed to load song '%s.tp'", tracker->song.name);
    return;
  }
  ui_alert("song '%s.tp' loaded", tracker->song.name);
}

void restart_song(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  tracker->song.index = 0;
  tracker->song.loop_counter = 0;
  tracker_modify_song_index(tracker);
}

void modify_bpm(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  tracker->bpm = CLAMP(tracker->bpm, BPM_MIN, BPM_MAX);
  mix_set_bpm(tracker->bpm);
}

void decrement_bpm(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  tracker->bpm = CLAMP(tracker->bpm - 1, BPM_MIN, BPM_MAX);
  mix_set_bpm(tracker->bpm);
}

void increment_bpm(Element* e) {
  Tracker* tracker = (Tracker*)e->userdata;
  tracker->bpm = CLAMP(tracker->bpm + 1, BPM_MIN, BPM_MAX);
  mix_set_bpm(tracker->bpm);
}

void tracker_init(Instrument* ins, Mix* mix) {
  (void)ins;
  Tracker* tracker = memory_alloc(sizeof(Tracker));
  ins->userdata = tracker;
  tracker_default(tracker, mix);
  tracker->self = ins;
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
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 50, .y = line_height, };
    e.userdata = tracker;
    e.onclick = save_source_onclick;
    ui_attach_element(canvas, &e);
  }
  {
    Element e = ui_button("kill");
    e.background_color = warmer_color(e.background_color, 80);
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 50, .y = line_height, };
    e.userdata = tracker;
    e.onclick = kill_source_onclick;
    e.tooltip = "kill sample";
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
    Element e = ui_text("gain");
    e.sizing = SIZING_PERCENT(35, 0);
    ui_attach_element(canvas, &e);
  }
  {
    Element e = ui_input_float("gain", &tracker->source.settings.gain);
    e.sizing = (Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PIXELS, .x = 65, .y = line_height, };
    e.userdata = tracker;
    e.onmodify = modify_source_setting;
    e.tooltip = "gain";
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
    e.tooltip = "add current pattern to song sequence";
    ui_attach_element(tracker->song_editor, &e);
  }
  {
    Element e = ui_input_int("position", &tracker->song.index);
    e.sizing = SIZING_PIXELS(input_width * 2, line_height);
    e.userdata = tracker;
    e.onmodify = modify_song_index;
    e.tooltip = "song position/bar";
    ui_attach_element(tracker->song_editor, &e);
  }
  {
    Element e = ui_button("<");
    e.sizing = SIZING_PIXELS(input_width, line_height);
    e.userdata = tracker;
    e.onclick = decrement_song_index;
    ui_attach_element(tracker->song_editor, &e);
  }
  {
    Element e = ui_button(">");
    e.sizing = SIZING_PIXELS(input_width, line_height);
    e.userdata = tracker;
    e.onclick = increment_song_index;
    ui_attach_element(tracker->song_editor, &e);
  }

  {
    Element e = ui_input_text("song name", tracker->song.name, sizeof(tracker->song.name));
    e.sizing = SIZING_PIXELS(input_width * 7, line_height);
    e.tooltip = "song name";
    ui_attach_element(tracker->song_editor, &e);
  }

  {
    Element e = ui_button("save");
    e.sizing = SIZING_PIXELS(2 * input_width, line_height);
    e.userdata = tracker;
    e.onclick = save_song;
    e.tooltip = "save the song to disk";
    ui_attach_element(tracker->song_editor, &e);
  }
  {
    Element e = ui_button("load");
    e.sizing = SIZING_PIXELS(2 * input_width, line_height);
    e.userdata = tracker;
    e.onclick = load_song;
    e.tooltip = "load song from disk";
    ui_attach_element(tracker->song_editor, &e);
  }
  {
    Element e = ui_button("restart");
    e.sizing = SIZING_PIXELS(3 * input_width, line_height);
    e.userdata = tracker;
    e.onclick = restart_song;
    e.tooltip = "restart song from the top";
    ui_attach_element(tracker->song_editor, &e);
  }

  {
    Element e = ui_line_break(2);
    e.render = e.background = true;
    e.background_color = lerp_color(UI_BACKGROUND_COLOR, UI_INTERPOLATION_COLOR, 0.4f);
    ui_attach_element(tracker->song_editor, &e);
  }

  // pattern list here
  for (i32 i = 0; i < tracker->song.length; ++i) {
    tracker_insert_pattern_in_song(tracker, i);
  }

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
    e.sizing = SIZING_PIXELS(input_width * 3, line_height);
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_input_int16("pattern id", &tracker->pattern_id);
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
    Element e = ui_text("bpm");
    e.sizing = SIZING_PIXELS(input_width * 3, line_height);
    ui_attach_element(&settings, &e);
  }

  {
    Element e = ui_input_int("bpm", &tracker->bpm);
    e.sizing = SIZING_PIXELS(input_width * 2, line_height);
    e.userdata = tracker;
    e.onmodify = modify_bpm;
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_button("<");
    e.sizing = SIZING_PIXELS(input_width, line_height);
    e.userdata = tracker;
    e.onclick = decrement_bpm;
    ui_attach_element(&settings, &e);
  }
  {
    Element e = ui_button(">");
    e.sizing = SIZING_PIXELS(input_width, line_height);
    e.userdata = tracker;
    e.onclick = increment_bpm;
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
    Element e = ui_container("pattern editor");
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

  // row 0
  {
    Element e = ui_none();
    e.box = BOX(0, 0, input_width + line_height, line_height);
    ui_attach_element(inner, &e);
  }
  for (size_t i = 0; i < MAX_TRACKER_CHANNELS; ++i) {
    {
      Element e = ui_toggle_ex(&tracker->muted_channels[i], (char*)channel_str[i]);
      e.box = BOX(0, 0, 3 * input_width, line_height);
      ui_attach_element(inner, &e);
    }
    {
      Element e = ui_none();
      e.box = BOX(0, 0, line_height, line_height);
      ui_attach_element(inner, &e);
    }
  }
  // row 1 - panning
  ui_attach_element_v2(inner, ui_line_break(0));
  {
    Element e = ui_none();
    e.box = BOX(0, 0, input_width + line_height, line_height);
    ui_attach_element(inner, &e);
  }
  for (size_t i = 0; i < MAX_TRACKER_CHANNELS; ++i) {
    {
      Element e = ui_slider_float(&tracker->channel_panning[i], 0, 1);
      e.box = BOX(0, 0, 3 * input_width, line_height);
      e.tooltip = "channel panning";
      ui_attach_element(inner, &e);
    }
    {
      Element e = ui_none();
      e.box = BOX(0, 0, line_height, line_height);
      ui_attach_element(inner, &e);
    }
  }
  ui_attach_element_v2(inner, ui_line_break(0));

  const i32 subdivision = 4;
  for (size_t row_index = 0; row_index < MAX_TRACKER_ROW; ++row_index) {
    {
      Element e = ui_input_int16(NULL, &tracker->pattern.channels[0].rows[row_index].id);
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
        Element e = ui_input_int16(NULL, &row->note);
        e.border = true;
        e.box = BOX(0, 0, 1.5 * input_width, line_height);
        e.tooltip = "note";
        e.v.i = row_index;
        e.userdata = tracker;
        e.onupdate = tracker_row_update;
        e.onmodify = modify_item_in_pattern_editor;
        e.onhover = tracker_hover_note;
        ui_attach_element(inner, &e);
      }
      {
        Element e = ui_input_int16(NULL, &row->source_id);
        e.border = true;
        e.box = BOX(0, 0, 1.5 * input_width, line_height);
        e.tooltip = "sample id";
        e.v.i = row_index;
        e.userdata = tracker;
        e.onupdate = tracker_sample_input_update;
        e.onmodify = modify_item_in_pattern_editor;
        e.onhover = tracker_hover_sample_id;
        ui_attach_element(inner, &e);
      }
      {
        Element e = ui_none();
        e.box = BOX(0, 0, line_height, line_height);
        if (!(row_index % subdivision)) {
          e.render = true;
          e.background = true;
          e.background_color = lerp_color(UI_BACKGROUND_COLOR, invert_color(UI_INTERPOLATION_COLOR), 0.4f);
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
  tracker->dt = mix->dt;
  tracker->samples_per_subtick = (SAMPLE_RATE / (f32)mix->bpm);

  tracker->prev_tick = tracker->tick;
  tracker->tick = mix->timed_tick;
  tracker->active_row = tracker->tick % MAX_TRACKER_ROW;

  Song* song = &tracker->song;

  if (tracker->prev_tick != tracker->tick) {
    if ((tracker->tick % MAX_TRACKER_ROW) == 0 && !tracker->loop_pattern) {
      Song_step* step = &song->pattern_sequence[song->index];
      tracker->song.loop_counter += 1;
      if (tracker->song.loop_counter >= step->loop + 1) {
        song->index += 1;
        song->index = ((u32)song->index % MAX_SONG_PATTERN_SEQUENCE) % song->length;
        tracker->pattern_id = song->pattern_sequence[song->index].pattern_id;
        change_pattern_id(tracker);
        tracker->song.loop_counter = 0;
      }
      else {
        step->animation = 2;
      }
    }

    if (tracker->prev_pattern_id != tracker->pattern_id) {
      tracker_change_pattern(tracker);
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
        channel->freq_target = freq / base_freq;
        if (src->source.samples > 0) {
          channel->sample_index = (src->settings.start * tracker->samples_per_subtick) % src->source.samples;
        }
        else {
          channel->sample_index = (src->settings.start * tracker->samples_per_subtick);
        }
        channel->state = SAMPLE_STATE_PLAY;
        channel->active_row = tracker->active_row;
      }
    }
  }

  for (size_t i = 0; i < MAX_TRACKER_CHANNELS; ++i) {
    Tracker_channel* channel = &tracker->pattern.channels[i];
    if (channel->active_row >= 0 && channel->active_row < MAX_TRACKER_ROW) {
      Tracker_row* row = &channel->rows[channel->active_row];
      i32 id = row->source_id;
      if (id == tracker->source_id) {
        tracker->source.source.cursor = (size_t)channel->sample_index;
      }
    }
  }
  if (!ui_input_interacting()) {
    if (IsKeyDown(KEY_LEFT_CONTROL)) {
      if (IsKeyDown(KEY_S)) {
        if (tracker_save_song(tracker) == Ok) {
          ui_alert("song '%s.%s' saved", tracker->song.name, TRACKER_FILE_EXT);
        }
      }
    }
    else {
      // add keyboard shortcut(s)
    }
  }
}

void tracker_process(Instrument* ins, Mix* mix, Audio_engine* audio, f32 dt) {
  (void)mix; (void)audio; (void)dt;
  Tracker* tracker = (Tracker*)ins->userdata;
  // ticket_mutex_begin(&tracker->source.source.mutex);
  // for (size_t i = 0; i < MAX_AUDIO_SOURCE; ++i) {
  //   Tracker_source* src = &tracker->sources[i];
  //   ticket_mutex_begin(&src->source.mutex);
  // }
  const size_t channel_count = audio->channel_count;
  const f32 sample_dt = dt / ins->samples;
  memset(ins->out_buffer, 0, sizeof(f32) * ins->samples);

  for (size_t channel_index = 0; channel_index < MAX_TRACKER_CHANNELS; ++channel_index) {
    if (tracker->muted_channels[channel_index]) {
      continue;
    }
    Tracker_channel* channel = &tracker->pattern.channels[channel_index];
    Tracker_row* row = NULL;
    if (channel->active_row >= 0 && channel->active_row < MAX_TRACKER_ROW) {
      row = &channel->rows[channel->active_row];
    }
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
        sample_end = (src->settings.start * tracker->samples_per_subtick) + (src->settings.length * tracker->samples_per_subtick);
      }
      f32 pan_map[2] = {
        1 - tracker->channel_panning[channel_index], // left channel
        tracker->channel_panning[channel_index], // right channel
      };

      for (size_t i = 0; i < ins->samples; i += channel_count) {
        for (size_t channel_index_out = 0; channel_index_out < channel_count; ++channel_index_out) {
          f32 pan = pan_map[channel_index_out % channel_count];
          if (channel->sample_index >= sample_end || channel->sample_index >= src->source.samples) {
            if (src->settings.pingpong) {
              channel->sample_index = sample_end - 1;
              channel->freq = -channel->freq;
              channel->freq_target = -channel->freq_target;
            }
            else {
              channel->state = SAMPLE_STATE_END;
              channel->active_row = -1;
              break;
            }
          }
          if (channel->sample_index < (src->settings.start * tracker->samples_per_subtick) || channel->sample_index < 0) {
            if (src->settings.pingpong) {
              channel->sample_index = (src->settings.start * tracker->samples_per_subtick);
              channel->freq = -channel->freq;
              channel->freq_target = -channel->freq_target;
            }
            else {
              break;
            }
          }
          ins->out_buffer[i + channel_index_out] += pan * src->settings.gain * src->source.buffer[(size_t)channel->sample_index];
          channel->sample_index += channel->freq;
          channel->freq = lerp_f32(channel->freq, channel->freq_target, sample_dt * FREQ_INTERP_SPEED);
        }
      }
    }
  }
  // ticket_mutex_end(&tracker->source.source.mutex);
  // for (size_t i = 0; i < MAX_AUDIO_SOURCE; ++i) {
  //   Tracker_source* src = &tracker->sources[i];
  //   ticket_mutex_end(&src->source.mutex);
  // }
}

void tracker_noteon(Instrument* ins, u8 note, f32 velocity) {
  (void)ins; (void)note; (void)velocity;
  noteon_tick = mix_get_tick();
  noteon_note = note;
}

void tracker_noteoff(Instrument* ins, u8 note) {
  (void)ins; (void)note;
}

void tracker_destroy(Instrument* ins) {
  (void)ins;
  Tracker* tracker = (Tracker*)ins->userdata;

  for (size_t i = 0; i < MAX_AUDIO_SOURCE; ++i) {
    Tracker_source* src = &tracker->sources[i];
    audio_unload_audio(&src->source);
  }
  audio_unload_audio(&tracker->source.source);
}

#undef MAX_AUDIO_SOURCE
#undef MAX_TRACKER_ROW
#undef MAX_TRACKER_CHANNELS
#undef MAX_PATTERN
#undef MAX_SONG_PATTERN_SEQUENCE
#undef ROOT_NOTE
#undef TRACKER_VERSION
#undef TRACKER_MAGIC
#undef ANIMATION_SPEED
#undef FREQ_INTERP_SPEED
#undef TRACKER_FILE_EXT
