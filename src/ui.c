// ui.c
// TODO:
//  - text input field
//  - icon/image

#ifndef DRAW_GUIDES
  #define DRAW_GUIDES 1
#endif

#define LOG_UI_HIERARCHY 0
#define UI_LOG_PATH "ui.txt"

#if DRAW_GUIDES
static Color GUIDE_COLOR = COLOR_RGB(255, 0, 255);
static Color GUIDE_COLOR2 = COLOR_RGB(255, 100, 255);
static bool ONLY_DRAW_GUIDE_ON_HOVER = false;
#endif

UI_state ui_state = {0};

#if 0
#define C(R, G, B) COLOR_RGB(R, G, B)
static Theme themes[MAX_THEME_ID] = {
  // main background     background           border               button               text                 border thickness  title bar padding   button roundness
  { C(35, 35, 42),       C(85, 90, 115),      C(0, 0, 0),          C(153, 102, 255),    C(255, 255, 255),    0.0f,             8,                  0.1f },
  { C(0x27, 0x2d, 0x3a), C(0x31, 0x3d, 0x5e), C(0, 0, 0),          C(0x45, 0x78, 0xa3), C(255, 255, 255),    1.0f,             4,                  0.0f },
  { C(55, 55, 55),       C(75, 75, 75),       C(35, 35, 35),       C(0x55, 0x68, 0xa0), C(230, 230, 230),    2.0f,             8,                  0.3f },
  { C(35, 65, 100),      C(65, 95, 145),      C(240, 100, 240),    C(0x84, 0x34, 0xbf), C(230, 230, 230),    1.0f,             12,                 0.1f },
};
#undef C
#endif

static void ui_state_init(UI_state* ui);
static void ui_theme_init(void);
static void ui_update_elements(UI_state* ui, Element* e);
static void ui_update_container(UI_state* ui, Element* e);
static void ui_update_grid(UI_state* ui, Element* e);
static void ui_render_elements(UI_state* ui, Element* e);
static void ui_free_elements(UI_state* ui, Element* e);
static void ui_element_init(Element* e);
static bool ui_overlap(i32 x, i32 y, Box box);
static Box  ui_pad_box(Box box, i32 padding);
static Box  ui_pad_box_ex(Box box, i32 x_padding, i32 y_padding);
static void ui_onclick(struct Element* e);
static void ui_toggle_onclick(struct Element* e);
static void ui_slider_onclick(UI_state* ui, struct Element* e);
static void ui_onrender(struct Element* e);

static void ui_print_elements(UI_state* ui, i32 fd, Element* e, u32 level);
static void tabs(i32 fd, const u32 count);

void ui_state_init(UI_state* ui) {
  ui_theme_init();
  ui_element_init(&ui->root);
  ui->root.type = ELEMENT_CONTAINER;
  ui->root.placement = PLACEMENT_FILL;
  ui->root.padding = 0;
  ui->root.border = false;
  ui->root.background = false;

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
#endif
  ui->active_id = 0;
  ui->frame_arena = arena_new(UI_FRAME_ARENA_SIZE);

}

void ui_theme_init(void) {
#if 0
  if (UI_THEME >= 0 && UI_THEME < MAX_THEME_ID) {
    const Theme* theme = &themes[UI_THEME];
    BACKGROUND_COLOR = theme->main_background;
    UI_BACKGROUND_COLOR = theme->background;
    UI_BORDER_COLOR = theme->border;
    UI_BUTTON_COLOR = theme->button;
    UI_TEXT_COLOR = theme->text;
    UI_BORDER_THICKNESS = theme->border_thickness;
    UI_TITLE_BAR_PADDING = theme->title_bar_padding;
    UI_BUTTON_ROUNDNESS = theme->button_roundness;
  }
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

  if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && e == ui->hover) {
    ui->active = e;
  }
  if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && e == ui->hover) {
    ui->select = e;
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
        if (px + w + e->padding >= e->box.w) {
          px = 0;
          py += py_offset + 2 * e->padding;
          py_offset = 0;
        }
        if (h > py_offset) {
          py_offset = h;
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
  f32 factor = 0.0f;

  if (e->type == ELEMENT_TOGGLE) {
    factor += 0.4f * (*e->data.toggle.value == false);
  }

  if ((e->type == ELEMENT_BUTTON || e->type == ELEMENT_TOGGLE) && e == ui->hover) {
    factor += 0.1f;
    if (e == ui->active) {
      factor += 0.15f;
    }
  }
  background_color = lerp_color(background_color, COLOR_RGB(0, 0, 0), factor);

  const i32 segments = 8;
  if (e->background) {
    if (e->roundness > 0) {
      DrawRectangleRounded((Rectangle) { e->box.x, e->box.y, e->box.w, e->box.h }, e->roundness, segments, background_color);
    }
    else {
      DrawRectangle(e->box.x, e->box.y, e->box.w, e->box.h, background_color);
    }
  }

#if DRAW_GUIDES
  i32 guide_overlap = 0;
  if (e == ui->hover || !ONLY_DRAW_GUIDE_ON_HOVER) {
    DrawLine(e->box.x - guide_overlap, e->box.y + e->box.h / 2, e->box.x + e->box.w + guide_overlap, e->box.y + e->box.h / 2, GUIDE_COLOR);
    DrawLine(e->box.x + e->box.w / 2, e->box.y - guide_overlap, e->box.x + e->box.w / 2, e->box.y + e->box.h + guide_overlap, GUIDE_COLOR);
  }
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
      if (e == ui->hover || !ONLY_DRAW_GUIDE_ON_HOVER) {
        DrawRectangleLines(x, y, w, h, GUIDE_COLOR);
      }
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
      if (e == ui->hover || !ONLY_DRAW_GUIDE_ON_HOVER) {
        DrawRectangleLines(x, y, w, h, GUIDE_COLOR);
      }
#endif
      break;
    }
    case ELEMENT_TOGGLE: {
      char* toggle_text = e->data.toggle.text;
      if (!toggle_text) {
        break;
      }
      // NOTE: strlen call might be expensive here, this might be a point to come back to if latency becomes an issue
      char* text = arena_alloc(&ui->frame_arena, strlen(toggle_text));
      stb_snprintf(text, ui->frame_arena.size, "%s: %s", toggle_text, bool_str[*e->data.toggle.value == true]);
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
      if (e == ui->hover || !ONLY_DRAW_GUIDE_ON_HOVER) {
        DrawRectangleLines(x, y, w, h, GUIDE_COLOR);
      }
#endif
      break;
    }
    case ELEMENT_CANVAS: {
      e->onrender(e);
      break;
    }
    case ELEMENT_SLIDER: {
      // TODO(lucas): vertical slider
      Box box = ui_pad_box_ex(e->box, UI_SLIDER_INNER_PADDING, 2 * UI_SLIDER_INNER_PADDING);
      Color line_color = lerp_color(e->background_color, COLOR_RGB(0, 0, 0), 0.2f);
      if (e->roundness > 0) {
        DrawRectangleRounded((Rectangle) { box.x, box.y, box.w, box.h }, e->roundness, segments, line_color);
      }
      else {
        DrawRectangle(box.x, box.y, box.w, box.h, line_color);
      }
      Range range = e->data.slider.range;
      i32 x = box.x;
      i32 y = box.y;
      i32 radius = UI_SLIDER_KNOB_SIZE;
      i32 h = box.h;
      f32 factor = 0.0f;
      switch (e->data.slider.type) {
        case SLIDER_FLOAT: {
          f32 range_length = -(range.f_min - range.f_max);
          factor = (*e->data.slider.v.f - range.f_min) / range_length;
          break;
        }
        case SLIDER_INTEGER: {
          i32 range_length = -(range.i_min - range.i_max);
          factor = (*e->data.slider.v.i - range.i_min) / (f32)range_length;
          break;
        }
        default:
            break;
      }
      factor = CLAMP(factor, 0.0f, 1.0f);
      DrawCircle(x + box.w*factor, y + h/2, radius, UI_BUTTON_COLOR);
      if (e->border_thickness > 0.0f) {
        DrawCircleLines(x + box.w*factor, y + h/2, radius, e->border_color);
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

  if (e->border && e->border_thickness > 0) {
    if (e->roundness > 0) {
      DrawRectangleRoundedLines((Rectangle) { e->box.x, e->box.y, e->box.w, e->box.h}, e->roundness, segments, e->border_thickness, e->border_color);
    }
    else {
      DrawRectangleLinesEx((Rectangle) { e->box.x, e->box.y, e->box.w, e->box.h}, e->border_thickness, e->border_color);
    }
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
      DrawRectangle(x, y, w, h, lerp_color(e->background_color, COLOR_RGB(0, 0, 0), 0.2f));
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
  e->roundness = 0.0f;

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

Box ui_pad_box(Box box, i32 padding) {
  return BOX(
    box.x + padding,
    box.y + padding,
    box.w - 2 * padding,
    box.h - 2 * padding
  );
}

Box ui_pad_box_ex(Box box, i32 x_padding, i32 y_padding) {
  return BOX(
    box.x + x_padding,
    box.y + y_padding,
    box.w - 2 * x_padding,
    box.h - 2 * y_padding
  );
}

void ui_onclick(struct Element* e) {
  (void)e;
}

void ui_toggle_onclick(struct Element* e) {
  *e->data.toggle.value = !*e->data.toggle.value;
}

void ui_slider_onclick(UI_state* ui, struct Element* e) {
  Box box = ui_pad_box_ex(e->box, UI_SLIDER_INNER_PADDING, 2 * UI_SLIDER_INNER_PADDING);
  i32 x_delta = ui->mouse.x - box.x;
  f32 factor = x_delta / (f32)box.w;
  f32 deadzone = e->data.slider.deadzone;
  Range range = e->data.slider.range;
  switch (e->data.slider.type) {
    case SLIDER_FLOAT: {
      f32 value = lerp_f32(range.f_min, range.f_max, factor);
      if (factor - deadzone <= 0.0f) {
        value = range.f_min;
      }
      if (factor + deadzone >= 1.0f) {
        value = range.f_max;
      }
      *e->data.slider.v.f = value;
      break;
    }
    case SLIDER_INTEGER: {
      i32 value = (i32)lerp_f32(range.i_min, range.i_max, factor);
      if (factor - deadzone <= 0.0f) {
        value = range.i_min;
      }
      if (factor + deadzone >= 1.0f) {
        value = range.i_max;
      }
      *e->data.slider.v.i = value;
      break;
    }
    default:
      break;
  }
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
  arena_reset(&ui->frame_arena);

#if DRAW_GUIDES
  ONLY_DRAW_GUIDE_ON_HOVER = true;
  if (IsKeyDown(KEY_LEFT_CONTROL)) {
    ONLY_DRAW_GUIDE_ON_HOVER = false;
  }
#endif

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
    if (ui->select->id == ui->active_id) {
      if (ui->select->type == ELEMENT_TOGGLE) {
        ui_toggle_onclick(ui->select);
      }
      ui->select->onclick(ui->select);
    }
    ui->active_id = 0;
  }
  if (ui->hover == ui->active && ui->hover) {
    if (ui->active->id == ui->active_id && ui->active->type == ELEMENT_SLIDER) {
      ui_slider_onclick(ui, ui->active);
    }
  }
  i32 cursor = MOUSE_CURSOR_DEFAULT;
  if (ui->hover) {
    switch (ui->hover->type) {
      case ELEMENT_BUTTON:
      case ELEMENT_TOGGLE:
      case ELEMENT_SLIDER: {
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
#if DRAW_GUIDES
  if (ui->hover != NULL) {
    Element* e = ui->hover;
    DrawRectangleLinesEx((Rectangle) { e->box.x, e->box.y, e->box.w, e->box.h}, 1.0f, GUIDE_COLOR2);
  }
#endif
  ui->latency += TIMER_END();
}

void ui_free(void) {
  UI_state* ui = &ui_state;
  ui_hierarchy_print();
  ui_free_elements(ui, &ui->root);
  close(ui->fd);
  ui->fd = -1;
  arena_free(&ui->frame_arena);
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
  e.roundness = UI_BUTTON_ROUNDNESS;
  e.background_color = UI_BUTTON_COLOR;
  return e;
}

Element ui_canvas(bool border) {
  Element e;
  ui_element_init(&e);
  e.type = ELEMENT_CANVAS;
  e.background = true;
  e.border = border;
  e.scissor = false;
  return e;
}

Element ui_toggle(i32* value) {
  ASSERT(value != NULL);
  Element e;
  ui_element_init(&e);
  e.data.toggle.value = value;
  e.data.toggle.text = NULL;
  e.type = ELEMENT_TOGGLE;
  e.background = true;
  e.border = true;
  e.scissor = false;
  e.roundness = UI_BUTTON_ROUNDNESS;
  e.background_color = UI_BUTTON_COLOR;
  return e;
}

Element ui_toggle_ex(i32* value, char* text) {
  Element e = ui_toggle(value);
  e.data.toggle.text = text;
  return e;
}

Element ui_slider(void* value, Slider_type type, Range range) {
  ASSERT(value != NULL);

  Element e;
  ui_element_init(&e);
  switch (type) {
    case SLIDER_FLOAT:
      e.data.slider.v.f = (f32*)value;
      break;
    case SLIDER_INTEGER:
      e.data.slider.v.i = (i32*)value;
      break;
    default:
      ASSERT(!"invalid slider type");
      break;
  }
  e.data.slider.type = type;
  e.data.slider.range = range;
  e.data.slider.vertical = false;
  e.data.slider.deadzone = 0.0f;
  e.type = ELEMENT_SLIDER;
  e.scissor = false;
  e.background = true;
  e.border = true;
  e.roundness = UI_BUTTON_ROUNDNESS;
  e.background_color = lerp_color(UI_BACKGROUND_COLOR, COLOR_RGB(255, 255, 255), 0.2f);
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
    case ELEMENT_TOGGLE: {
      stb_dprintf(fd, "%s\n", bool_str[*e->data.toggle.value == true]);
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
