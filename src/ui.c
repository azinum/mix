// ui.c

#define DRAW_GUIDES 0
#define LOG_UI_HIERARCHY 1
#define UI_LOG_PATH "ui.txt"

UI_state ui_state = {0};

Color BACKGROUND_COLOR = COLOR_RGB(35, 35, 42);
Color UI_BACKGROUND_COLOR = COLOR_RGB(85, 85, 105);
Color UI_BORDER_COLOR = COLOR_RGB(0, 0, 0);
Color UI_BUTTON_COLOR = COLOR_RGB(153, 102, 255);
Color UI_TEXT_COLOR = COLOR_RGB(255, 255, 255);
f32 UI_BORDER_THICKNESS = 1.0f;
i32 UI_TITLE_BAR_PADDING = 2;

#define C(R, G, B) COLOR_RGB(R, G, B)
static Theme themes[MAX_THEME_ID] = {
  // main background     background           border      button               text              border thickness  title bar padding
  { C(35, 35, 42),       C(85, 85, 105),      C(0, 0, 0), C(153, 102, 255),    C(255, 255, 255), 1.0f,             2 },
  { C(0x27, 0x2d, 0x3a), C(0x31, 0x3d, 0x5e), C(0, 0, 0), C(0x45, 0x78, 0xa3), C(255, 255, 255), 1.0f,             4 },
};
#undef C

static void ui_state_init(UI_state* ui);
static void ui_theme_init(void);
static void ui_update_elements(UI_state* ui, Element* e);
static void ui_update_container(UI_state* ui, Element* e);
static void ui_update_grid(UI_state* ui, Element* e);
static void ui_render_elements(UI_state* ui, Element* e);
static void ui_free_elements(UI_state* ui, Element* e);
static void ui_element_init(Element* e);
static bool ui_overlap(i32 x, i32 y, Box box);
static void ui_onclick(struct Element* e);
static void ui_onrender(struct Element* e);

static void ui_print_elements(UI_state* ui, i32 fd, Element* e, u32 level);
static void tabs(i32 fd, const u32 count);

void ui_state_init(UI_state* ui) {
  ui_theme_init();
  ui_element_init(&ui->root);
  ui->root.type = ELEMENT_CONTAINER;
  ui->root.padding = 0;
  ui->root.border = false;

  ui->id_counter = 2;
  ui->latency = 0;
  ui->element_update_count = 0;
  ui->element_render_count = 0;
  ui->mouse = (Vector2) {0, 0},
  ui->hover = NULL;
  ui->active = NULL;
  ui->select = NULL;
#if LOG_UI_HIERARCHY
  ui->fd = open(UI_LOG_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0664);
  if (ui->fd < 0) {
    log_print(STDERR_FILENO, LOG_TAG_ERROR, "failed to open file `%s` for writing\n", UI_LOG_PATH);
  }
#else
  ui->fd = -1;
  ui->active_id = 0;
#endif
}

void ui_theme_init(void) {
  if (UI_THEME >= 0 && UI_THEME < MAX_THEME_ID) {
    const Theme* theme = &themes[UI_THEME];
    BACKGROUND_COLOR = theme->main_background;
    UI_BACKGROUND_COLOR = theme->background;
    UI_BORDER_COLOR = theme->border;
    UI_BUTTON_COLOR = theme->button;
    UI_TEXT_COLOR = theme->text;
    UI_BORDER_THICKNESS = theme->border_thickness;
    UI_TITLE_BAR_PADDING = theme->title_bar_padding;
  }
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
    case ELEMENT_CONTAINER: {
      ui_update_container(ui, e);
      break;
    }
    case ELEMENT_GRID: {
      ui_update_grid(ui, e);
      break;
    }
    case ELEMENT_BUTTON: {
      if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && e == ui->hover) {
        ui->active = e;
      }
      if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && e == ui->hover) {
        ui->select = e;
      }
      break;
    }
    case ELEMENT_TEXT: {
      char* text = e->data.text.string;
      if (!text) {
        break;
      }
      const Font font = assets.font;
      i32 font_size = FONT_SIZE;
      i32 spacing = 0;
      Vector2 text_size = MeasureTextEx(font, text, font_size, spacing);
      const Sizing sizing = e->sizing;
      i32 w = text_size.x;
      i32 h = text_size.y;
      if (sizing.mode == SIZE_MODE_PERCENT) {
        if (sizing.x != 0) {
          w = e->box.w;
        }
        if (sizing.y != 0) {
          h = e->box.h;
        }
      }
      e->box.w = w;
      e->box.h = h;
      break;
    }
    case ELEMENT_CANVAS: {
      if (e == ui->hover) {
        e->data.canvas.mouse_x = ui->mouse.x - e->box.x;
        e->data.canvas.mouse_y = ui->mouse.y - e->box.y;
        break;
      }
      e->data.canvas.mouse_x = 0;
      e->data.canvas.mouse_y = 0;
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
  (void)ui;
  char* title = e->data.container.title;
  i32 title_padding = e->data.container.title_padding;

  // placement offsets
  i32 px = 0;
  i32 py = 0;
  // block placement offsets
  i32 py_offset = 0; // element with the greatest height
  u32 num_elements_on_line = 0;

  const i32 font_size = FONT_SIZE;
  const i32 title_height = font_size + title_padding * 2;
  if (title) {
    e->box.y += title_height;
    e->box.h -= title_height;
  }

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
        const Sizing sizing = item->sizing;
        i32 w = item->box.w;
        i32 h = item->box.h;
        if (sizing.mode == SIZE_MODE_PERCENT) {
          if (sizing.x != 0) {
            w = (sizing.x / 100.0f) * e->box.w - 2 * e->padding;
          }
          if (sizing.y != 0) {
            h = (sizing.y / 100.0f) * e->box.h - 2 * e->padding;
          }
        }
        item->box = BOX(
          e->box.x + e->padding + px,
          e->box.y + e->padding + py,
          w,
          h
        );
        py += item->box.h + 2 * e->padding;
        break;
      }
      case PLACEMENT_BLOCK: {
        const Sizing sizing = item->sizing;
        i32 w = item->box.w;
        i32 h = item->box.h;
        if (sizing.mode == SIZE_MODE_PERCENT) {
          if (sizing.x != 0) {
            w = (sizing.x / 100.0f) * e->box.w - 2 * e->padding;
          }
          if (sizing.y != 0) {
            h = (sizing.y / 100.0f) * e->box.h - 2 * e->padding;
          }
        }

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
          w,
          h
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
  (void)ui;
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
    DrawRectangleLinesEx((Rectangle) { e->box.x, e->box.y, e->box.w, e->box.h}, e->border_thickness, e->border_color);
  }

#if DRAW_GUIDES
  Color guide_color = COLOR(255, 50, 255, 255);
  i32 guide_overlap = 0;
  DrawLine(e->box.x - guide_overlap, e->box.y + e->box.h / 2, e->box.x + e->box.w + guide_overlap, e->box.y + e->box.h / 2, guide_color);
  DrawLine(e->box.x + e->box.w / 2, e->box.y - guide_overlap, e->box.x + e->box.w / 2, e->box.y + e->box.h + guide_overlap, guide_color);
#endif

  switch (e->type) {
    case ELEMENT_TEXT: {
      // TODO(lucas): text wrapping
      char* text = e->data.text.string;
      if (!text) {
        break;
      }
      const Font font = assets.font;
      const i32 font_size = FONT_SIZE;
      const i32 spacing = 0;
      Vector2 text_size = MeasureTextEx(font, text, font_size, spacing);
      const i32 x = e->box.x;
      const i32 y = e->box.y;
      const i32 w = (i32)text_size.x;
      const i32 h = (i32)text_size.y;
      DrawTextEx(font, text, (Vector2) { x, y }, font_size, spacing, e->text_color);
      (void)w;
      (void)h;
#if DRAW_GUIDES
      DrawRectangleLines(x, y, w, h, guide_color);
#endif
      break;
    }
    case ELEMENT_BUTTON: {
      char* text = e->data.text.string;
      if (!text) {
        break;
      }
      const Font font = assets.font;
      i32 font_size = FONT_SIZE;
      i32 spacing = 0;
      Vector2 text_size = MeasureTextEx(font, text, font_size, spacing);
      const i32 x = e->box.x + e->box.w / 2 - text_size.x / 2;
      const i32 y = e->box.y + e->box.h / 2 - text_size.y / 2;
      const i32 w = (i32)text_size.x;
      const i32 h = (i32)text_size.y;
      DrawTextEx(font, text, (Vector2) { x, y }, font_size, spacing, e->text_color);
      (void)w; (void)h;
#if DRAW_GUIDES
      DrawRectangleLines(x, y, w, h, guide_color);
#endif
      break;
    }
    case ELEMENT_CANVAS: {
      e->onrender(e);
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
  // draw title bar after ending scissor mode if the element is a container and has a title
  if (e->type == ELEMENT_CONTAINER) {
    char* title = e->data.container.title;
    i32 title_padding = e->data.container.title_padding;
    if (title) {
      const Font font = assets.font;
      const i32 font_size = FONT_SIZE;
      const i32 title_height = font_size + title_padding * 2;
      i32 spacing = 0;
      const i32 w = e->box.w;
      const i32 h = title_height + (i32)e->border_thickness;
      const i32 x = e->box.x;
      const i32 y = e->box.y - title_height;
      DrawRectangle(x, y, w, h, lerpcolor(e->background_color, COLOR_RGB(0, 0, 0), 0.3f));
      DrawTextEx(font, title, (Vector2) { x + title_padding, y + title_padding }, font_size, spacing, e->text_color);
      if (e->border) {
        DrawRectangleLinesEx((Rectangle) { x, y, w, h}, e->border_thickness, e->border_color);
      }
    }
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
  e->id = 1;
  e->box = BOX(0, 0, 0, 0);
  e->type = ELEMENT_NONE;
  memset(&e->data, 0, sizeof(e->data));
  e->userdata = NULL;

  e->padding = UI_PADDING;

  e->text_color = UI_TEXT_COLOR;
  e->background_color = UI_BACKGROUND_COLOR;
  e->border_color = UI_BORDER_COLOR;

  e->render = true;
  e->background = false;
  e->border = false;
  e->scissor = false;
  e->hidden = false;

  e->border_thickness = UI_BORDER_THICKNESS;

  e->placement = PLACEMENT_NONE;
  e->sizing = (Sizing) {
    .mode = SIZE_MODE_PIXELS,
    .x = 0,
    .y = 0,
  };

  e->onclick = ui_onclick;
  e->onrender = ui_onrender;
}

bool ui_overlap(i32 x, i32 y, Box box) {
  return (x >= box.x && x <= box.x + box.w)
    && (y >= box.y && y <= box.y + box.h);
}

void ui_onclick(struct Element* e) {
  (void)e;
}

void ui_onrender(struct Element* e) {
  (void)e;
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
  if (ui->active) {
    if (ui->active_id == 0) {
      ui->active_id = ui->active->id;
    }
  }

  if (ui->hover == ui->select && ui->hover) {
    if (ui->hover->id == ui->active_id) {
      ui->select->onclick(ui->select);
    }
    ui->active_id = 0;
  }
  i32 cursor = MOUSE_CURSOR_DEFAULT;
  if (ui->hover) {
    switch (ui->hover->type) {
      case ELEMENT_BUTTON: {
        cursor = MOUSE_CURSOR_POINTING_HAND;
        break;
      }
      case ELEMENT_CANVAS: {
        cursor = MOUSE_CURSOR_CROSSHAIR;
        break;
      }
      default:
        break;
    }
  }
  SetMouseCursor(cursor);
  ui->latency += TIMER_END();
}

void ui_hierarchy_print(void) {
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
  ui_hierarchy_print();
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

Element ui_container(char* title) {
  Element e;
  ui_element_init(&e);
  e.type = ELEMENT_CONTAINER;
  e.render = true;
  e.scissor = true;
  e.data.container.title = title;
  e.data.container.title_padding = UI_TITLE_BAR_PADDING;
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
  e.background = false;
  e.border = false;
  e.scissor = false;
  return e;
}

Element ui_button(char* text) {
  Element e;
  ui_element_init(&e);
  e.type = ELEMENT_BUTTON;
  e.data.text.string = text;
  e.background = true;
  e.border = true;
  e.scissor = false;

  e.background_color = UI_BUTTON_COLOR;
  return e;
}

Element ui_canvas(bool border) {
  Element e;
  ui_element_init(&e);
  e.type = ELEMENT_CANVAS;
  e.background = true;
  e.border = border;
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
  tabs(fd, level); stb_dprintf(fd, "sizing: { mode: %s, x: %d, y: %d }\n", size_mode_str[e->sizing.mode], e->sizing.x, e->sizing.y);

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
