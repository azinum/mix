// ui.h

#ifndef _UI_H
#define _UI_H

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

  MAX_ELEMENT_TYPE,
} Element_type;

#define BOX(X, Y, W, H) ((Box) { .x = X, .y = Y, .w = W, .h = H })

typedef union Element_data {
  struct {
    u32 rows;
    u32 cols;
  } grid;
  struct {
    char* string;
  } text;
} Element_data;

typedef enum Placement {
  PLACEMENT_FILL = 0,
  PLACEMENT_BLOCK,
  PLACEMENT_ROWS,

  MAX_PLACEMENT_TYPE,
} Placement;

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

  Placement placement;

  void (*onclick)(struct Element* e, void* userdata);
} Element;

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
} UI_state;

extern UI_state ui_state;

Result ui_init(void);
void ui_update(void);
void ui_render(void);
void ui_free(void);

Element* ui_attach_element(Element* target, Element* e);
Element ui_container(bool render);
Element ui_grid(u32 cols, bool render);
Element ui_text(char* text);
Element ui_button(char* text);

#endif // _UI_H
