// ui.h

#ifndef _UI_H
#define _UI_H

#ifndef UI_FRAME_ARENA_SIZE
  #define UI_FRAME_ARENA_SIZE Kb(8)
#endif

typedef struct Theme {
  Color main_background;
  Color background;
  Color border;
  Color button;
  Color text;
  f32 border_thickness;
  i32 title_bar_padding;
  f32 button_roundness;
} Theme;

typedef enum Theme_id {
  THEME_DEFAULT = 0,
  THEME_NAVY,
  THEME_GRAY,
  THEME_COLORFUL,

  MAX_THEME_ID,
} Theme_id;

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
  } text;
  struct {
    char* title; // unused
    i32 title_padding; // unused
  } container;
  struct {
    i32 mouse_x;
    i32 mouse_y;
  } canvas;
  struct {
    i32* value;
    char* text;
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

  void (*onclick)(struct Element* e);
  void (*onrender)(struct Element* e);
} __attribute__((aligned(CACHELINESIZE))) Element;

typedef struct UI_state {
  Element root;
  u32 id_counter;
  f32 latency;
  u32 element_update_count;
  u32 element_render_count;
  Vector2 mouse;
  Element* hover;
  Element* active;
  Element* select;
  i32 fd;
  u32 active_id;
  Arena frame_arena;
} UI_state;

extern UI_state ui_state;

Result ui_init(void);
void ui_update(void);
void ui_hierarchy_print(void);
void ui_render(void);
void ui_free(void);

Element* ui_attach_element(Element* target, Element* e);
Element ui_container(char* title);
Element ui_grid(u32 cols, bool render);
Element ui_text(char* text);
Element ui_button(char* text);
Element ui_canvas(bool border);
Element ui_toggle(i32* value);
Element ui_toggle_ex(i32* value, char* text);
Element ui_slider(void* value, Slider_type type, Range range);

#endif // _UI_H
