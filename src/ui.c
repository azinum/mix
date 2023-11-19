// ui.c

#define DRAW_GUIDES 0
#define LOG_UI_HIERARCHY 1
#define UI_PATH "ui.txt"

UI_state ui_state = {0};

static void ui_state_init(UI_state* ui);
static void ui_update_elements(UI_state* ui, Element* e);
static void ui_update_container(UI_state* ui, Element* e);
static void ui_update_grid(UI_state* ui, Element* e);
static void ui_render_elements(UI_state* ui, Element* e);
static void ui_free_elements(UI_state* ui, Element* e);
static void ui_element_init(Element* e);
static bool ui_overlap(i32 x, i32 y, Box box);
static void ui_onclick(struct Element* e, void* userdata);

static void ui_print_elements(UI_state* ui, i32 fd, Element* e, u32 level);
static void tabs(i32 fd, const u32 count);

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
#if LOG_UI_HIERARCHY
  ui->fd = open(UI_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0664);
  if (ui->fd < 0) {
    log_print(STDERR_FILENO, LOG_TAG_ERROR, "failed to open file `%s` for writing\n", UI_PATH);
  }
#else
  ui->fd = -1;
#endif
}

void ui_update_elements(UI_state* ui, Element* e) {
  if (!e) {
    return;
  }
  if (e->hidden) {
    return;
  }
  ui->element_update_count += 1;

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
      }
      break;
    }
    case ELEMENT_CONTAINER: {
      ui_update_container(ui, e);
      break;
    }
    case ELEMENT_GRID: {
      ui_update_grid(ui, e);
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

void ui_update_container(UI_state* ui, Element* e) {
  // placement offsets
  i32 px = 0;
  i32 py = 0;
  // block placement offsets
  i32 py_offset = 0; // element with the greatest height
  u32 num_elements_on_line = 0;
  bool hide = false;
  for (size_t i = 0; i < e->count; ++i) {
    Element* item = &e->items[i];
    item->hidden = hide;
    switch (e->placement) {
      case PLACEMENT_FILL: {
        item->box = BOX(
          e->box.x + e->padding,
          e->box.y + e->padding,
          e->box.w - 2 * e->padding,
          e->box.h - 2 * e->padding
        );
        return; // this fitment only allows one item
      }
      case PLACEMENT_ROWS: {
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
        break;
      }
      case PLACEMENT_BLOCK: {
        if (py >= e->box.h) {
          item->hidden = hide = true;
          break;
        }
        num_elements_on_line += 1;
        if (px + item->box.w + 2 * e->padding >= e->box.w && num_elements_on_line > 1) {
          px = 0;
          py += py_offset + 2 * e->padding;
          py_offset = 0;
          num_elements_on_line = 0;
        }
        if (item->box.h > py_offset) {
          py_offset = item->box.h;
        }
        item->box = BOX(
          e->box.x + e->padding + px,
          e->box.y + e->padding + py,
          item->box.w,
          item->box.h
        );
        px += item->box.w + 2 * e->padding;
        break;
      }
      default:
        break;
    }
  }
}

void ui_update_grid(UI_state* ui, Element* e) {
  const u32 cols = e->data.grid.cols;
  const u32 rows = (u32)ceilf((f32)e->count / cols);
  for (size_t i = 0; i < e->count; ++i) {
    Element* item = &e->items[i];
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
      const i32 font_size = FONT_SIZE;
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
      i32 font_size = FONT_SIZE;
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

  e->placement = PLACEMENT_NONE;

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
  root->placement = PLACEMENT_FILL;

  ui->element_update_count = 0;
  ui->element_render_count = 0;
  ui->mouse = GetMousePosition();
  ui->hover = NULL;
  ui->active = NULL;
  ui->select = NULL;

  ui_update_elements(ui, root);
  if (ui->hover == ui->select && ui->hover != NULL) {
    ui->select->onclick(ui->select, ui->select->userdata);
  }
  ui->latency += TIMER_END();
}

void ui_hierarchy_print(i32 fd) {
  UI_state* ui = &ui_state;
  ui_print_elements(ui, ui->fd, &ui->root, 0);
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
  ui_hierarchy_print(STDOUT_FILENO);
  ui_free_elements(ui, &ui->root);
  close(ui->fd);
  ui->fd = -1;
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

void ui_print_elements(UI_state* ui, i32 fd, Element* e, u32 level) {
  stb_dprintf(fd, "{\n");
  level += 1;
  tabs(fd, level); stb_dprintf(fd, "type: %s\n", element_type_str[e->type]);
  tabs(fd, level); stb_dprintf(fd, "id: %u\n", e->id);
  tabs(fd, level); stb_dprintf(fd, "box: { %d, %d, %d, %d }\n", e->box.x, e->box.y, e->box.w, e->box.h);
  tabs(fd, level); stb_dprintf(fd, "data: ");
  switch (e->type) {
    case ELEMENT_GRID: {
      stb_dprintf(fd, "{ cols: %u, rows: %u }\n", e->data.grid.cols, e->data.grid.cols);
      break;
    }
    case ELEMENT_BUTTON:
    case ELEMENT_TEXT: {
      char* string = e->data.text.string;
      if (string) {
        stb_dprintf(fd, "\"%s\"\n", string);
        break;
      }
      stb_dprintf(fd, "\"\"\n", string);
      break;
    }
    default:
      stb_dprintf(fd, "-\n");
      break;
  }
  tabs(fd, level); stb_dprintf(fd, "padding: %d\n", e->padding);
  tabs(fd, level); stb_dprintf(fd, "render: %s\n", bool_str[e->render == true]);
  tabs(fd, level); stb_dprintf(fd, "background: %s\n", bool_str[e->background == true]);
  tabs(fd, level); stb_dprintf(fd, "border: %s\n", bool_str[e->border == true]);
  tabs(fd, level); stb_dprintf(fd, "scissor: %s\n", bool_str[e->scissor == true]);
  tabs(fd, level); stb_dprintf(fd, "hidden: %s\n", bool_str[e->hidden == true]);
  tabs(fd, level); stb_dprintf(fd, "placement: %s\n", placement_str[e->placement]);

  for (size_t i = 0; i < e->count; ++i) {
    tabs(fd, level);
    ui_print_elements(ui, fd, &e->items[i], level);
  }

  tabs(fd, level - 1);
  stb_dprintf(fd, "}\n");
}

void tabs(i32 fd, const u32 count) {
  const char* tab = "   ";
  for (u32 i = 0; i < count; ++i) {
    stb_dprintf(fd, "%s", tab);
  }
}
