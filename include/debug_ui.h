// debug_ui.h

#ifndef _DEBUG_UI_H
#define _DEBUG_UI_H

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

typedef struct Element {
  struct Element* items;
  u32 count;
  u32 size;

  u32 id;
  Box box;
  Element_type type;
  Element_data data;

  u32 padding;

  bool render;
  bool border;
} Element;

Result ui_init(void);
void ui_update(void);
void ui_render(void);
f32 ui_get_latency(void);
void ui_free(void);

Element* ui_attach_element(Element* target, Element* e);
Element ui_container(bool render);
Element ui_grid(u32 cols, bool render);
Element ui_text(char* text);
Element ui_button(char* text);

#endif // _DEBUG_UI_H
