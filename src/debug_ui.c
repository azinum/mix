// debug_ui.c

#define INIT_ITEMS_SIZE 16
#define list_init(list, desired_size) \
  if ((list)->size < desired_size) { \
    (list)->size = desired_size; \
    (list)->items = memory_realloc((list)->items, (list)->size * sizeof(*(list)->items)); \
    ASSERT((list)->items != NULL && "out of memory"); \
  }

#define list_push(list, item) \
  if ((list)->count >= (list)->size) { \
    if ((list)->size == 0) { \
      (list)->size = INIT_ITEMS_SIZE; \
    } \
    else { \
      (list)->size *= 2; \
    } \
    (list)->items = memory_realloc((list)->items, (list)->size * sizeof(*(list)->items)); \
    ASSERT((list)->items != NULL && "out of memory"); \
  } \
  (list)->items[(list)->count++] = (item)

#define list_free(list) memory_free((list)->items)

typedef struct UI_state {
  Element root;
  u32 id_counter;
  f32 latency;
} UI_state;

static UI_state ui_state = {0};

static void ui_state_init(UI_state* ui);
static void ui_update_elements(UI_state* ui, Element* e);
static void ui_render_elements(UI_state* ui, Element* e);
static void ui_free_elements(UI_state* ui, Element* e);
static void ui_element_init(Element* e);

void ui_state_init(UI_state* ui) {
  ui_element_init(&ui->root);
  ui->root.type = ELEMENT_CONTAINER;
  ui->id_counter = 1;
  ui->latency = 0;
}

void ui_update_elements(UI_state* ui, Element* e) {
  if (!e) {
    return;
  }
  for (size_t i = 0; i < e->count; ++i) {
    Element* item = &e->items[i];
    switch (e->type) {
      case ELEMENT_CONTAINER: {
        item->box = BOX(
          e->box.x + e->padding,
          e->box.y + e->padding,
          e->box.w - 2 * e->padding,
          e->box.h - 2 * e->padding
        );
        break;
      }
      case ELEMENT_GRID: {
        break;
      }
      case ELEMENT_TEXT: {
        break;
      }
      case ELEMENT_BUTTON: {
        break;
      }
      default:
        break;
    }
  }
  for (size_t i = 0; i < e->count; ++i) {
    Element* item = &e->items[i];
    ui_update_elements(ui, item);
  }
}

void ui_render_elements(UI_state* ui, Element* e) {
  if (!e) {
    return;
  }
  if (!e->render) {
    return;
  }
  Color border_color = COLOR_RGB(255, 255, 255);

  DrawRectangleLines(e->box.x, e->box.y, e->box.w, e->box.h, border_color);

  for (size_t i = 0; i < e->count; ++i) {
    Element* item = &e->items[i];
    ui_render_elements(ui, item);
  }
}

void ui_free_elements(UI_state* ui, Element* e) {
  if (!e) {
    return;
  }
  for (size_t i = 0; i < e->count; ++i) {
    ui_free_elements(ui, &e->items[i]);
  }
  list_free(e);
}

void ui_element_init(Element* e) {
  e->items = NULL;
  e->count = 0;
  e->size = 0;
  e->id = 0;
  e->box = BOX(0, 0, 0, 0);
  e->type = ELEMENT_NONE;
  e->padding = 8;
  e->render = true;
  e->border = false;
}

Result ui_init(void) {
  ui_state_init(&ui_state);
  return Ok;
}

void ui_update(void) {
  TIMER_START();
  UI_state* ui = &ui_state;
  ui->latency = 0;
  Element* root = &ui->root;
  root->box = BOX(0, 0, GetScreenWidth(), GetScreenHeight());
  ui_update_elements(ui, root);
  ui->latency += TIMER_END();
}

void ui_render(void) {
  TIMER_START();
  UI_state* ui = &ui_state;
  Element* root = &ui->root;
  ui_render_elements(ui, root);
  ui->latency += TIMER_END();
}

f32 ui_get_latency(void) {
  return ui_state.latency;
}

void ui_free(void) {
  UI_state* ui = &ui_state;
  ui_free_elements(ui, &ui->root);
}

Element* ui_attach_element(Element* target, Element* e) {
  UI_state* ui = &ui_state;
  if (!target) {
    target = &ui->root;
  }
  size_t index = target->count;
  e->id = ui->id_counter++;
  list_push(target, *e);
  return &target->items[index];
}

Element ui_container(bool render) {
  Element e;
  ui_element_init(&e);
  e.type = ELEMENT_CONTAINER;
  e.render = render;
  return e;
}

Element ui_grid(u32 cols, bool render) {
  Element e;
  ui_element_init(&e);
  e.type = ELEMENT_GRID;
  e.data.grid.cols = cols;
  e.render = render;
  return e;
}

Element ui_text(char* text) {
  Element e;
  ui_element_init(&e);
  e.type = ELEMENT_TEXT;
  e.data.text.string = text;
  e.render = true;
  return e;
}

Element ui_button(char* text) {
  Element e;
  ui_element_init(&e);
  e.type = ELEMENT_BUTTON;
  e.data.text.string = text;
  e.render = true;
  return e;
}
