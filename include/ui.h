// ui.h

#ifndef _UI_H
#define _UI_H

extern Color BACKGROUND_COLOR;
extern Color UI_BACKGROUND_COLOR;
extern Color UI_BORDER_COLOR;
extern Color UI_BUTTON_COLOR;
extern Color UI_TEXT_COLOR;
extern f32 UI_BORDER_THICKNESS;
extern i32 UI_TITLE_BAR_PADDING;

typedef struct Theme {
  Color main_background;
  Color background;
  Color border;
  Color button;
  Color text;
  f32 border_thickness;
  i32 title_bar_padding;
} Theme;

typedef enum Theme_id {
  THEME_DEFAULT = 0,
  THEME_NAVY,

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

  MAX_ELEMENT_TYPE,
} Element_type;

const char* element_type_str[] = {
  "none",
  "container",
  "grid",
  "text",
  "button",
  "canvas",
};

#define BOX(X, Y, W, H) ((Box) { .x = X, .y = Y, .w = W, .h = H })

typedef union Element_data {
  struct {
    u32 rows;
    u32 cols;
  } grid;
  struct {
    char* string;
  } text;
  struct {
    char* title;
    i32 title_padding;
  } container;
  struct {
    i32 mouse_x;
    i32 mouse_y;
  } canvas;
} Element_data;

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

  Color text_color;
  Color background_color;
  Color border_color;

  bool render;
  bool background;
  bool border;
  bool scissor;
  bool hidden;

  f32 border_thickness;

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

#endif // _UI_H
