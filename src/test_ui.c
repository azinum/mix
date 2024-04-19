// test_ui.c

#define NUM_ROWS 32

typedef struct Tracker_row {
  i32 id;
  i32 velocity;
  i32 note;
  i32 command;
} Tracker_row;

struct {
  size_t cursor;
  i32 follow_cursor;
  Tracker_row rows[NUM_ROWS];
  Color highlight_color;
  Color background_color;
} state = {
  .cursor = 0,
  .follow_cursor = true,
};

static void tracker_onupdate(Element* e);
static void row_onupdate(Element* e);

void tracker_onupdate(Element* e) {
  Mix* mix = (Mix*)e->userdata;

  size_t prev_cursor = state.cursor;
  state.cursor = mix->timed_tick;
  state.cursor %= NUM_ROWS;
  if (state.cursor != prev_cursor) {
    // stepped...
    if (state.follow_cursor) {
      f32 cursor_progress = (f32)state.cursor / NUM_ROWS;
      ui_scroll_container(e, cursor_progress);
    }
  }
}

void row_onupdate(Element* e) {
  if (e->v.i == (i32)state.cursor) {
    e->background_color = state.highlight_color;
  }
  else {
    e->background_color = state.background_color;
  }
}

Element test_ui_new(Mix* mix) {
  f32 seed = get_time();
  random_init((Random)seed);
  Element test_ui = ui_container(NULL);
  test_ui.sizing = SIZING_PERCENT(100, 100);
  test_ui.scissor = false;
  test_ui.border = false;
  test_ui.background = false;
  test_ui.placement = PLACEMENT_BLOCK;
  test_ui.x_padding = 16;
  test_ui.y_padding = 16;

  state.background_color = UI_BACKGROUND_COLOR;
  state.highlight_color = brighten_color(saturate_color(UI_BUTTON_COLOR, -0.1f), 0.2f);
  const i32 width = 600;
  const i32 height = 400;

  Element* inner = NULL;
  {
    Element e = ui_container("tracker");
    e.border = true;
    e.background = true;
    e.background_color = lerp_color(UI_BACKGROUND_COLOR, COLOR_RGB(255, 50, 255), 0.1f);
    e.sizing = (Sizing) {
      .x_mode = SIZE_MODE_PIXELS,
      .y_mode = SIZE_MODE_PIXELS,
      .x = width, .y = height,
    };
    e.y_padding = 0;
    e.x_padding = 0;
    e.onupdate = tracker_onupdate;
    e.userdata = mix;
    inner = ui_attach_element(&test_ui, &e);
  }
  i32 line_height = FONT_SIZE;
  i32 input_width = FONT_SIZE * 1.5f;
  for (i32 i = 0; i < NUM_ROWS; ++i) {
    state.rows[i] = (Tracker_row) {
      .id = i,
      .velocity = 0,
      .note = 0,
    };
    {
      Element e = ui_input_int(NULL, &state.rows[i].id);
      e.readonly = true;
      e.background = true;
      e.background_color = state.background_color;
      e.border = true;
      e.box = BOX(0, 0, line_height, line_height);
      e.v.i = i;
      e.onupdate = row_onupdate;
      e.tooltip = "position";
      ui_attach_element(inner, &e);
    }
    {
      Element e = ui_none();
      e.box = BOX(0, 0, line_height, line_height);
      ui_attach_element(inner, &e);
    }
    {
      Element e = ui_input_int(NULL, &state.rows[i].note);
      e.border = true;
      e.box = BOX(0, 0, input_width, line_height);
      e.tooltip = "note";
      ui_attach_element(inner, &e);
    }
    {
      Element e = ui_input_int(NULL, &state.rows[i].velocity);
      e.border = true;
      e.box = BOX(0, 0, input_width, line_height);
      e.tooltip = "velocity";
      ui_attach_element(inner, &e);
    }
    {
      Element e = ui_input_int(NULL, &state.rows[i].command);
      e.border = true;
      e.box = BOX(0, 0, input_width, line_height);
      e.tooltip = "command";
      ui_attach_element(inner, &e);
    }
    ui_attach_element_v2(inner, ui_line_break(0));
  }
  return test_ui;
}
