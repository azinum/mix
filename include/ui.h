// ui.h
// macros:
//  UI_FRAME_ARENA_SIZE
//  UI_DRAW_GUIDES
//  UI_LOG_HIERARCHY
//  UI_LOG_PATH
//  UI_TOOLTIP_DELAY
//  UI_SCROLL_SPEED

#ifndef _UI_H
#define _UI_H

// #define UI_DRAW_GUIDES

#ifndef UI_FRAME_ARENA_SIZE
  #define UI_FRAME_ARENA_SIZE Kb(8)
#endif

#ifndef UI_LOG_PATH
  #define UI_LOG_PATH "ui.txt"
#endif

// delay, in seconds, until showing tooltip
#ifndef UI_TOOLTIP_DELAY
  #define UI_TOOLTIP_DELAY 0.7f
#endif

#ifndef UI_SCROLL_SPEED
  #define UI_SCROLL_SPEED 20
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
};

#define BOX(X, Y, W, H) ((Box) { .x = X, .y = Y, .w = W, .h = H })

typedef enum {
  SLIDER_FLOAT,
  SLIDER_INTEGER,
} Slider_type;

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

#define RANGE_FLOAT(min, max) ((Range) { .f_min = min, .f_max = max, })
#define RANGE(min, max) ((Range) { .i_min = min, .i_max = max, })

typedef union Element_data {
  struct {
    u32 rows;
    u32 cols;
  } grid;
  struct {
    char* string;
    bool allow_overflow;
    bool text_wrapping;
  } text;
  struct {
    i32 scroll_x;
    i32 scroll_y;
    i32 content_height;
    bool scrollable;
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
    Slider_type type;
    Range range;
    bool vertical;
    f32 deadzone;
  } slider;
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
  SIZE_MODE_PIXELS = 0,
  SIZE_MODE_PERCENT,

  MAX_SIZE_MODE,
} Size_mode;

const char* size_mode_str[] = {
  "pixels",
  "percent",
};

typedef struct Sizing {
  Size_mode mode;
  i32 x;
  i32 y;
} Sizing;

#define SIZING_PERCENT(X, Y) ((Sizing) { .mode = SIZE_MODE_PERCENT, .x = X, .y = Y, })

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

  i32 padding;
  Title_bar title_bar;

  Color text_color;
  Color background_color;
  Color border_color;

  bool render;
  bool background;
  bool border;
  bool scissor;
  bool hidden;

  f32 border_thickness;
  f32 roundness;

  Placement placement;
  Sizing sizing;

  char* tooltip;

  void (*onclick)(struct Element* e);
  void (*onupdate)(struct Element* e);
  void (*onrender)(struct Element* e);
  void (*onconnect)(struct Element* e, struct Element* target); // element e connects to target
} __attribute__((aligned(CACHELINESIZE))) Element;

typedef struct UI_state {
  Element root;
  u32 id_counter;
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
  i32 fd;
  u32 active_id;
  Arena frame_arena;
  f32 dt;
  f32 timer;
  f32 tooltip_timer;
  f32 slider_deadzone;
  bool (*connection_filter)(struct Element* e, struct Element* target);
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
void ui_reset_connection_filter(void);

Element* ui_attach_element(Element* target, Element* e);
Element ui_none(void);
Element ui_container(char* title);
Element ui_container_ex(char* title, bool scrollable);
Element ui_grid(u32 cols, bool render);
Element ui_text(char* text);
Element ui_button(char* text);
Element ui_canvas(bool border);
Element ui_toggle(i32* value);
Element ui_toggle_ex(i32* value, char* text);
Element ui_toggle_ex2(i32* value, char* false_text, char* true_text);
Element ui_slider(void* value, Slider_type type, Range range);
Element ui_line_break(i32 height);

#endif // _UI_H
