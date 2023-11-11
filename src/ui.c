// ui.c

#define DRAW_GUIDES 0

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
  ui->element_update_count = 0;
  ui->element_render_count = 0;
  ui->mouse = (Vector2) {0, 0},
  ui->hover = NULL;
  ui->active = NULL;
  ui->select = NULL;
}

void ui_update_elements(UI_state* ui, Element* e) {
  if (!e) {
    return;
  }
  if (e->hidden) {
    return;
  }
  ui->element_update_count += 1;

  // placement offsets
  i32 px = 0;
  i32 py = 0;

  bool hide = false;
  for (size_t i = 0; i < e->count; ++i) {
    Element* item = &e->items[i];
    item->hidden = hide;
    switch (e->type) {
      case ELEMENT_CONTAINER: {
        if (e->placement == PLACEMENT_FILL) {
          item->box = BOX(
            e->box.x + e->padding,
            e->box.y + e->padding,
            e->box.w - 2 * e->padding,
            e->box.h - 2 * e->padding
          );
        }
        else if (e->placement == PLACEMENT_ROWS) {
          if (py >= e->box.h) {
            item->hidden = hide = true;
            break;
          }
          item->box = BOX(
            e->box.x + e->padding + px,
            e->box.y + e->padding + py,
            item->box.w,
            item->box.h
          );
          py += item->box.h + 2 * e->padding;
        }
        else if (e->placement == PLACEMENT_BLOCK) {

        }
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

  if (ui_overlap((i32)ui->mouse.x, (i32)ui->mouse.y, e->box)) {
    ui->hover = e;
  }

  switch (e->type) {
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
  if (!e->render || e->hidden) {
    return;
  }
  if (e->scissor) {
    BeginScissorMode(e->box.x, e->box.y, e->box.w, e->box.h);
  }

  ui->element_render_count += 1;
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

#if DRAW_GUIDES
  Color guide_color = COLOR(255, 50, 255, 255);
  i32 guide_overlap = 0;
  DrawLine(e->box.x - guide_overlap, e->box.y + e->box.h / 2, e->box.x + e->box.w + guide_overlap, e->box.y + e->box.h / 2, guide_color);
  DrawLine(e->box.x + e->box.w / 2, e->box.y - guide_overlap, e->box.x + e->box.w / 2, e->box.y + e->box.h + guide_overlap, guide_color);
#endif

  switch (e->type) {
    case ELEMENT_TEXT: {
      if (!e->data.text.string) {
        break;
      }
      const Font font = assets.font;
      const i32 font_size = FONT_SIZE_SMALL;
      const i32 spacing = 2;
      const i32 x = e->box.x;
      const i32 y = e->box.y;
      DrawTextEx(font, e->data.text.string, (Vector2) { x, y }, font_size, spacing, e->text_color);
      break;
    }
    case ELEMENT_BUTTON: {
      if (!e->data.text.string) {
        break;
      }
      char* text = e->data.text.string;
      const Font font = assets.font;
      i32 font_size = FONT_SIZE_SMALL;
      i32 spacing = 2;
      Vector2 text_size = MeasureTextEx(font, text, font_size, spacing);
      i32 x = e->box.x + e->box.w / 2 - text_size.x / 2;
      i32 y = e->box.y + e->box.h / 2 - text_size.y / 2;
      i32 w = (i32)text_size.x;
      i32 h = (i32)text_size.y;
      DrawTextEx(font, text, (Vector2) { x, y }, font_size, spacing, e->text_color);
      (void)w; (void)h;
#if DRAW_GUIDES
      DrawRectangleLines(x, y, w, h, guide_color);
#endif
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
  e->hidden = false;

  e->placement = PLACEMENT_FILL;

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

  ui->element_update_count = 0;
  ui->element_render_count = 0;
  ui->mouse = GetMousePosition();
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
