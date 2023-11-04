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

UI_state ui_state = {0};

static void ui_state_init(UI_state* ui);
static void ui_update_elements(UI_state* ui, Element* e);
static void ui_render_elements(UI_state* ui, Element* e);
static void ui_free_elements(UI_state* ui, Element* e);
static void ui_element_init(Element* e);
static bool ui_overlap(i32 x, i32 y, Box box);
static void ui_onclick(struct Element* e, void* userdata);

void ui_state_init(UI_state* ui) {
  ui_element_init(&ui->root);
  ui->root.type = ELEMENT_CONTAINER;
  ui->root.padding = 0;
  ui->root.border = false;

  ui->id_counter = 1;
  ui->latency = 0;
  ui->hover = NULL;
  ui->active = NULL;
  ui->select = NULL;
}

void ui_update_elements(UI_state* ui, Element* e) {
  if (!e) {
    return;
  }
  Vector2 mouse = GetMousePosition();

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
        const u32 cols = e->data.grid.cols;
        const u32 rows = (u32)ceilf((f32)e->count / cols);
        const u32 w = ceilf((f32)e->box.w / cols);
        const u32 h = ceilf((f32)e->box.h / rows);
        const u32 x = i % cols;
        const u32 y = (u32)floorf((f32)i / cols);
        item->box = BOX(
          e->box.x + x * w + e->padding,
          e->box.y + y * h + e->padding,
          w - 2 * e->padding,
          h - 2 * e->padding
        );
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

  if (ui_overlap((i32)mouse.x, (i32)mouse.y, e->box)) {
    ui->hover = e;
  }

  switch (e->type) {
    case ELEMENT_CONTAINER: {
      break;
    }
    case ELEMENT_GRID: {
      break;
    }
    case ELEMENT_TEXT: {
      break;
    }
    case ELEMENT_BUTTON: {
      if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && e == ui->hover) {
        ui->active = e;
      }
      if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && e == ui->hover) {
        ui->select = e;
        e->onclick(e, e->userdata);
      }
      break;
    }
    default:
      break;
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
  if (e->scissor) {
    BeginScissorMode(e->box.x, e->box.y, e->box.w, e->box.h);
  }

  Color background_color = e->background_color;

  if (e->type == ELEMENT_BUTTON && e == ui->hover) {
    f32 factor = 0.1f;
    if (e == ui->active) {
      factor = 0.25f;
    }
    background_color = lerpcolor(background_color, COLOR_RGB(0, 0, 0), factor);
  }

  if (e->background) {
    DrawRectangle(e->box.x, e->box.y, e->box.w, e->box.h, background_color);
  }

  if (e->border) {
    DrawRectangleLines(e->box.x, e->box.y, e->box.w, e->box.h, e->border_color);
  }

  switch (e->type) {
    case ELEMENT_TEXT: {
      if (e->data.text.string) {
        DrawText(e->data.text.string, e->box.x, e->box.y, FONT_SIZE_SMALL, e->text_color);
      }
      break;
    }
    case ELEMENT_BUTTON: {
      if (e->data.text.string) {
        i32 lines = 1;
        i32 max_width = 0;
        i32 font_size = FONT_SIZE_SMALL;
        char* s = e->data.text.string;
        i32 i = 0;
        while (*s++ != 0) {
          i += 1;
          if (*s == '\n') {
            lines += 1;
            i = 0;
          }
          if (i >= max_width) {
            max_width = i;
          }
        }
        i32 x = e->box.x + e->box.w / 2;
        i32 y = e->box.y + e->box.h / 2;
        i32 w = font_size * max_width;
        i32 h = font_size * lines;
        x -= (font_size * max_width) / 2;
        y -= (font_size * lines) / 2;
        DrawText(e->data.text.string, x, y, font_size, e->text_color);
        // DrawRectangleLines(x, y, w, h, COLOR_RGB(255, 50, 50));
      }
      break;
    }
    default:
      break;
  }

  for (size_t i = 0; i < e->count; ++i) {
    Element* item = &e->items[i];
    ui_render_elements(ui, item);
  }
  if (e->scissor) {
    EndScissorMode();
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
  memset(&e->data, 0, sizeof(e->data));
  e->userdata = NULL;

  e->padding = UI_PADDING;

  e->text_color = COLOR_RGB(255, 255, 255);
  e->background_color = COLOR_RGB(255, 255, 255);
  e->border_color = COLOR_RGB(0, 0, 0);

  e->render = true;
  e->background = false;
  e->border = true;
  e->scissor = false;

  e->onclick = ui_onclick;
}

bool ui_overlap(i32 x, i32 y, Box box) {
  return (x >= box.x && x <= box.x + box.w)
    && (y >= box.y && y <= box.y + box.h);
}

void ui_onclick(struct Element* e, void* userdata) {
  (void)e; (void)userdata;
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

  ui->hover = NULL;
  ui->active = NULL;
  ui->select = NULL;

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
  e.scissor = true;
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
