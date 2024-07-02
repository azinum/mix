// ui.h
// macros:
//  UI_FRAME_ARENA_SIZE
//  UI_DRAW_GUIDES
//  UI_LOG_HIERARCHY
//  UI_LOG_PATH
//  UI_TOOLTIP_DELAY
//  UI_SCROLL_SPEED
//  UI_EMULATE_TOUCH_SCREEN

#ifndef _UI_H
#define _UI_H

// #define UI_DRAW_GUIDES
// #define UI_LOG_HIERARCHY

#ifndef UI_FRAME_ARENA_SIZE
  #define UI_FRAME_ARENA_SIZE Kb(8)
#endif

#ifndef UI_LOG_PATH
  #define UI_LOG_PATH "ui.txt"
#endif

// delay, in seconds, until showing tooltip
#ifndef UI_TOOLTIP_DELAY
  #define UI_TOOLTIP_DELAY 0.2f
#endif

#ifndef UI_SCROLL_SPEED
  #define UI_SCROLL_SPEED 40
#endif

// how many seconds to wait until scrollbar hides
#ifndef UI_SCROLLBAR_DECAY
  #define UI_SCROLLBAR_DECAY 2.0f
#endif

#ifndef MAX_ALERT_TEXT_SIZE
  #define MAX_ALERT_TEXT_SIZE 512
#endif

#ifndef UI_ALERT_DECAY
  #define UI_ALERT_DECAY 3.0f
#endif

typedef struct {
  i32 x;
  i32 y;
  i32 w;
  i32 h;
} Box;

typedef enum {
  ELEMENT_NONE = 0,
  ELEMENT_CONTAINER,
  ELEMENT_GRID,
  ELEMENT_TEXT,
  ELEMENT_BUTTON,
  ELEMENT_CANVAS,
  ELEMENT_TOGGLE,
  ELEMENT_SLIDER,
  ELEMENT_INPUT,

  MAX_ELEMENT_TYPE,
} Element_type;

const char* element_type_str[] = {
  "none",
  "container",
  "grid",
  "text",
  "button",
  "canvas",
  "toggle",
  "slider",
  "input",
};

#define BOX(X, Y, W, H) ((Box) { .x = X, .y = Y, .w = W, .h = H })

typedef enum Value_type {
  VALUE_TYPE_NONE = 0,
  VALUE_TYPE_FLOAT,
  VALUE_TYPE_INT32,
  VALUE_TYPE_INT16,
  VALUE_TYPE_INT8,
  VALUE_TYPE_STRING,

  MAX_VALUE_TYPE,
} Value_type;

const size_t value_type_size[MAX_VALUE_TYPE] = {
  [VALUE_TYPE_NONE]   = 0,
  [VALUE_TYPE_FLOAT]  = sizeof(f32),
  [VALUE_TYPE_INT32]  = sizeof(i32),
  [VALUE_TYPE_INT16]  = sizeof(i16),
  [VALUE_TYPE_INT8]   = sizeof(i8),
  [VALUE_TYPE_STRING] = 0,
};

typedef union {
  struct {
    f32 f_min;
    f32 f_max;
  };
  struct {
    i32 i_min;
    i32 i_max;
  };
} Range;

typedef enum Input_type {
  INPUT_TEXT = 0,
  INPUT_NUMBER,

  MAX_INPUT_TYPE,
} Input_type;

#define RANGE_FLOAT(min, max) ((Range) { .f_min = min, .f_max = max, })
#define RANGE(min, max) ((Range) { .i_min = min, .i_max = max, })

typedef struct Input {
  Buffer buffer;
  size_t cursor;
  char* preview;
  Input_type input_type;
  Value_type value_type;
  Hash value_hash; // hash the current value to detect changes, update the buffer if a change is detected
  void* value;
  void (*callback)(struct Input*);
  u32 max_length;
} Input;

typedef enum Slider_type {
  SLIDER_HORIZONTAL,
  SLIDER_VERTICAL,
  SLIDER_ROUNDED,

  MAX_SLIDER_TYPE,
} Slider_type;

typedef union Element_data {
  struct {
    u32 rows;
    u32 cols;
  } grid;
  struct {
    char* string;
    // allow overflow = true:  do not mutate element size, keep the current element size and let text overflow
    // allow overflow = false: mutate element size based on the text content
    bool allow_overflow;
    // if text wrapping is true and allow overflow is false, the text element will be adjusted according to its text content
    // TODO(lucas): these settings can be confusing, make simpler
    bool text_wrapping;
  } text;
  struct {
    i32 scroll_x;
    i32 scroll_y;
    i32 content_height;
    bool scrollable;
    bool auto_adjust_height; // adjust height based on content
  } container;
  struct {
    i32 mouse_x;
    i32 mouse_y;
  } canvas;
  struct {
    i32* value;
    char* text[2];
  } toggle;
  struct {
    union {
      f32* f;
      i32* i;
    } v;
    Value_type type;
    Range range;
    Slider_type slider_type;
    f32 deadzone;
  } slider;
  Input input;
} Element_data;

typedef struct Title_bar {
  char* title;
  i32 padding;
  bool top;
} Title_bar;

typedef enum Placement {
  PLACEMENT_NONE = 0,
  PLACEMENT_FILL,
  PLACEMENT_BLOCK,
  PLACEMENT_ROWS,

  MAX_PLACEMENT_TYPE,
} Placement;

const char* placement_str[] = {
  "none",
  "fill",
  "block",
  "rows",
};

typedef enum Size_mode {
  SIZE_MODE_NONE = 0,
  SIZE_MODE_PIXELS,
  SIZE_MODE_PERCENT, // percent in relation to the parent element

  MAX_SIZE_MODE,
} Size_mode;

const char* size_mode_str[] = {
  "none",
  "pixels",
  "percent",
};

typedef struct Sizing {
  Size_mode x_mode;
  Size_mode y_mode;
  i32 x;
  i32 y;
} Sizing;

#define SIZING_PIXELS(X, Y) ((Sizing) { .x_mode = SIZE_MODE_PIXELS, .y_mode = SIZE_MODE_PIXELS, .x = X, .y = Y, })
#define SIZING_PERCENT(X, Y) ((Sizing) { .x_mode = SIZE_MODE_PERCENT, .y_mode = SIZE_MODE_PERCENT, .x = X, .y = Y, })

typedef struct Element {
  struct Element* items;
  u32 count;
  u32 size;

  const char* name;

  u32 id;
  Box box;
  Element_type type;
  Element_data data;
  void* userdata;

  struct {
    f32 f;
    union {
      i32 i;
      struct {
        i16 i16_a;
        i16 i16_b;
      };
    };
    char* s;
  } v;

  i32 x_padding;
  i32 y_padding;
  Title_bar title_bar;

  Color text_color;
  Color background_color;
  Color secondary_color;
  Color border_color;

  bool render;
  bool background;
  bool border;
  bool scissor;
  bool hidden;
  bool readonly;
  bool zoomable;

  f32 border_thickness;
  f32 roundness;

  Placement placement;
  Sizing sizing;

  char* tooltip;

  void (*onclick)(struct Element* e);
  void (*onupdate)(struct Element* e);
  void (*onrender)(struct Element* e);
  void (*onconnect)(struct Element* e, struct Element* target); // element e connects to target
  void (*oninput)(struct Element* e, char ch);
  void (*onmodify)(struct Element* e);
  void (*onhover)(struct Element* e);
} __attribute__((aligned(sizeof(size_t)))) Element;

typedef enum UI_tag {
  UI_TAG_MAIN = 0,
  UI_TAG_SETTINGS,
  UI_TAG_MIDI_SETTINGS,

  MAX_UI_TAGS,
} UI_tag;

typedef struct UI_state {
  Element* root;
  Element tags[MAX_UI_TAGS];
  i32 tag;
  u32 id_counter;
  u32 element_count;
  f32 latency;
  f32 render_latency;
  u32 element_update_count;
  u32 element_render_count;
  Vector2 mouse;
  Vector2 prev_mouse;
  Element* hover;
  Element* active;
  Element* select;
  Element* marker; // a marker element is used to connect two elements together via a callback function
  Element* container; // last container element that was hovered
  Element* scrollable; // last scrollable container
  Element* input;

  Element* zoom;
  Sizing zoom_sizing;
  Box zoom_box;

  i32 fd;
  u32 active_id;
  Arena frame_arena;
  f32 dt;
  f32 timer;
  f32 tooltip_timer;
  f32 blink_timer;
  f32 alert_timer;
  f32 scrollbar_timer;
  f32 slider_deadzone;
  Vector2 scroll;
  bool (*connection_filter)(struct Element* e, struct Element* target);
  char alert_text[512];
  i32 cursor;
} UI_state;

extern UI_state ui_state;

Result ui_init(void);
void ui_update(f32 dt);
void ui_hierarchy_print(void);
void ui_render(void);
void ui_free(void);

// set default slider deadzone so that when new sliders are created, they use this deadzone value from now on
void ui_set_slider_deadzone(f32 deadzone);
// set a filter for which elements that can be connected together
void ui_set_connection_filter(bool (*filter)(struct Element*, struct Element*));
void ui_set_title(Element* e, char* title);
void ui_reset_connection_filter(void);
// is the user currently interacting with an input element?
bool ui_input_interacting(void);
void ui_alert_simple(const char* message);
void ui_alert(const char* format, ...);
void ui_switch_state(i32 tag);
i32  ui_get_current_tag(void);
void ui_scroll_container(Element* e, f32 ratio); // 0-1, 0 -> top, 1 -> bottom

Element* ui_attach_element(Element* target, Element* e);
Element* ui_attach_element_v2(Element* target, Element e);
void    ui_detach_elements(Element* e); // detach child nodes of this element
void    ui_detach(Element* e, u32 index);
void    ui_detach_last(Element* e);
Element* ui_replace_element(Element* e, Element* new_element);
Element ui_none(void);
Element ui_container(char* title);
Element ui_container_ex(char* title, bool scrollable);
Element ui_grid(u32 cols, bool render);
Element ui_text(char* text);
Element ui_text_ex(char* text, bool text_wrapping);
Element ui_text_line(char* text); // text that takes up the width of the container it belongs to
Element ui_button(char* text);
Element ui_canvas(bool border);
Element ui_toggle(i32* value);
Element ui_toggle_ex(i32* value, char* text);
Element ui_toggle_ex2(i32* value, char* false_text, char* true_text);
Element ui_slider(void* value, Value_type type, Range range);
Element ui_slider_int(i32* value, i32 min_value, i32 max_value);
Element ui_slider_float(f32* value, f32 min_value, f32 max_value);
Element ui_line_break(i32 height);
Element ui_input(char* preview);
Element ui_input_ex(char* preview, Input_type input_type);
Element ui_input_ex2(char* preview, void* value, Input_type input_type, Value_type value_type);
Element ui_input_int(char* preview, i32* value);
Element ui_input_int16(char* preview, i16* value);
Element ui_input_int8(char* preview, i8* value);
Element ui_input_float(char* preview, f32* value);
Element ui_input_text(char* preview, char* str, u32 max_length);

#define UI_EASY_INPUT_F32(container, desc, ref_value, min_value, max_value) { \
  {                                                                           \
    Element e = ui_text(desc);                                                \
    e.box = BOX(0, 0, 0, FONT_SIZE);                                          \
    e.sizing = SIZING_PERCENT(100, 0);                                        \
    ui_attach_element(container, &e);                                         \
  }                                                                           \
  {                                                                           \
    Element e = ui_input_float(desc, ref_value);                              \
    e.box = BOX(0, 0, 0, FONT_SIZE);                                          \
    e.sizing = SIZING_PERCENT(15, 0);                                         \
    ui_attach_element(container, &e);                                         \
  }                                                                           \
  {                                                                           \
    Element e = ui_slider_float(ref_value, min_value, max_value);             \
    e.name = desc;                                                            \
    e.box = BOX(0, 0, 0, FONT_SIZE);                                          \
    e.sizing = SIZING_PERCENT(35, 0);                                         \
    ui_attach_element(container, &e);                                         \
  }                                                                           \
}

#define UI_EASY_INPUT_INT(container, desc, ref_value, min_value, max_value) { \
  {                                                                           \
    Element e = ui_text(desc);                                                \
    e.box = BOX(0, 0, 0, FONT_SIZE);                                          \
    e.sizing = SIZING_PERCENT(100, 0);                                        \
    ui_attach_element(container, &e);                                         \
  }                                                                           \
  {                                                                           \
    Element e = ui_input_int(desc, ref_value);                                \
    e.box = BOX(0, 0, 0, FONT_SIZE);                                          \
    e.sizing = SIZING_PERCENT(15, 0);                                         \
    ui_attach_element(container, &e);                                         \
  }                                                                           \
  {                                                                           \
    Element e = ui_slider_int(ref_value, min_value, max_value);               \
    e.name = desc;                                                            \
    e.box = BOX(0, 0, 0, FONT_SIZE);                                          \
    e.sizing = SIZING_PERCENT(35, 0);                                         \
    ui_attach_element(container, &e);                                         \
  }                                                                           \
}

#endif // _UI_H
