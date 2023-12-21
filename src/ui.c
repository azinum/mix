// ui.c
// TODO:
//  - overflow scroll in containers
//  - string formatting of text elements
//  - text wrapping
//  - text input field
//  - icon/image
//  - container tabs
//  - drag and drop (external file browser)

#define DRAW_SIMPLE_TEXT_EX(X, Y, SIZE, COLOR, FORMAT_STR, ...) do { \
  static char _text##__LINE__[SIZE] = {0}; \
  stb_snprintf(_text##__LINE__, sizeof(_text##__LINE__), FORMAT_STR, ##__VA_ARGS__); \
  DrawText(_text##__LINE__, X, Y, FONT_SIZE_SMALLEST, COLOR); \
} while (0)

#define DRAW_SIMPLE_TEXT(X, Y, FORMAT_STR, ...) DRAW_SIMPLE_TEXT_EX(X, Y, 64, COLOR_RGB(255, 255, 255), FORMAT_STR, ##__VA_ARGS__)
#define DRAW_SIMPLE_TEXT2(X, Y, COLOR, FORMAT_STR, ...) DRAW_SIMPLE_TEXT_EX(X, Y, 64, COLOR, FORMAT_STR, ##__VA_ARGS__)

#ifdef UI_DRAW_GUIDES
static Color GUIDE_COLOR             = COLOR_RGB(255, 0, 255);
static Color GUIDE_COLOR2            = COLOR_RGB(255, 100, 255);
static Color GUIDE_TEXT_COLOR        = COLOR_RGB(255, 180, 255);
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
static void ui_element_init(Element* e, Element_type type);
static bool ui_overlap(i32 x, i32 y, Box box);
static Box  ui_pad_box(Box box, i32 padding);
static Box  ui_pad_box_ex(Box box, i32 x_padding, i32 y_padding);
static Box  ui_expand_box(Box box, i32 padding);
static void ui_center_of(const Box* box, i32* x, i32* y);
static void ui_render_tooltip(UI_state* ui, char* tooltip);
static void ui_render_tooltip_of_element(UI_state* ui, Element* e);
static void ui_onclick(struct Element* e);
static void ui_toggle_onclick(struct Element* e);
static void ui_slider_onclick(UI_state* ui, struct Element* e);
static void ui_onrender(struct Element* e);
static void ui_onconnect(struct Element* e, struct Element* target);
static bool ui_connection_filter(struct Element* e, struct Element* target);
static void ui_print_elements(UI_state* ui, i32 fd, Element* e, u32 level);
static void tabs(i32 fd, const u32 count);

void ui_state_init(UI_state* ui) {
  ui_theme_init();
  ui_element_init(&ui->root, ELEMENT_CONTAINER);
  ui->root.placement = PLACEMENT_FILL;
  ui->root.padding = 0;
  ui->root.border = false;
  ui->root.background = false;

  ui->id_counter = 2;
  ui->latency = 0;
  ui->render_latency = 0;
  ui->element_update_count = 0;
  ui->element_render_count = 0;
  ui->mouse = (Vector2) {0, 0},
  ui->prev_mouse = ui->mouse;
  ui->hover = NULL;
  ui->active = NULL;
  ui->select = NULL;
  ui->marker = NULL;
#ifdef UI_LOG_HIERARCHY
  ui->fd = open(UI_LOG_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0664);
  if (ui->fd < 0) {
    log_print(STDERR_FILENO, LOG_TAG_ERROR, "failed to open file `%s` for writing\n", UI_LOG_PATH);
  }
#else
  ui->fd = -1;
#endif
  ui->active_id = 0;
  ui->frame_arena = arena_new(UI_FRAME_ARENA_SIZE);
  ui->dt = 0.0f;
  ui->timer = 0.0f;
  ui->slider_deadzone = 0.0f;
  ui->connection_filter = ui_connection_filter;
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

  const Title_bar title_bar = e->title_bar;
  // NOTE(lucas): title bars will only work proper when applied to elements that are scaled by percent or scaled by container and grid elements, with their placement and scaling methods
  if (title_bar.title) {
    const i32 font_size = FONT_SIZE;
    const i32 title_height = font_size + title_bar.padding * 2;
    if (title_bar.top) {
      e->box.y += title_height;
      e->box.h -= title_height;
    }
    else {
      e->box.h -= title_height;
    }
  }

  if (ui_overlap((i32)ui->mouse.x, (i32)ui->mouse.y, e->box)) {
    ui->hover = e;
  }
  else {
    e->tooltip_timer = 0.0f;
  }
  if (((i32)ui->mouse.x != (i32)ui->prev_mouse.x) || ((i32)ui->mouse.y != (i32)ui->prev_mouse.y)) {
    e->tooltip_timer = 0.0f;
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
  (void)ui; // unused

  // placement offsets
  i32 px = 0;
  i32 py = 0;
  // block placement offsets
  i32 py_offset = 0; // element with the greatest height

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
    factor += 0.4f * (*e->data.toggle.value == true);
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

#ifdef UI_DRAW_GUIDES
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
#ifdef UI_DRAW_GUIDES
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
#ifdef UI_DRAW_GUIDES
      if (e == ui->hover || !ONLY_DRAW_GUIDE_ON_HOVER) {
        DrawRectangleLines(x, y, w, h, GUIDE_COLOR);
      }
#endif
      break;
    }
    case ELEMENT_TOGGLE: {
      char* text = e->data.toggle.text[*e->data.toggle.value == true];
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
#ifdef UI_DRAW_GUIDES
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

  // draw title bar after ending scissor mode
  const Title_bar title_bar = e->title_bar;
  if (title_bar.title) {
    const Font font = assets.font;
    const i32 font_size = FONT_SIZE;
    const i32 title_height = font_size + title_bar.padding * 2;
    i32 spacing = 0;
    const i32 w = e->box.w;
    const i32 h = title_height + (i32)e->border_thickness;
    const i32 x = e->box.x;
    i32 y = e->box.y - title_height;
    if (!title_bar.top) {
      y = e->box.y + e->box.h;
    }
    DrawRectangle(x, y, w, h, lerp_color(e->background_color, COLOR_RGB(0, 0, 0), 0.2f));
    DrawTextEx(font, title_bar.title, (Vector2) { x + title_bar.padding, y + title_bar.padding }, font_size, spacing, e->text_color);
    if (e->border) {
      DrawRectangleLinesEx((Rectangle) { x, y, w, h}, e->border_thickness, e->border_color);
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

void ui_element_init(Element* e, Element_type type) {
  ASSERT(type >= ELEMENT_NONE && type < MAX_ELEMENT_TYPE);

  e->items = NULL;
  e->count = 0;
  e->size = 0;

  e->name = element_type_str[type];

  e->id = 1;
  e->box = BOX(0, 0, 0, 0);
  e->type = type;
  memset(&e->data, 0, sizeof(e->data));
  e->userdata = NULL;
  e->title_bar = (Title_bar) {
    .title = NULL,
    .padding = UI_TITLE_BAR_PADDING,
    .top = true,
  };
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
  e->roundness = UI_ROUNDNESS;

  e->placement = PLACEMENT_NONE;
  e->sizing = (Sizing) {
    .mode = SIZE_MODE_PIXELS,
    .x = 0,
    .y = 0,
  };
  e->tooltip_timer = 0.0f;
  e->tooltip = NULL;

  e->onclick = ui_onclick;
  e->onrender = ui_onrender;
  e->onconnect = ui_onconnect;
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

Box ui_expand_box(Box box, i32 padding) {
  return ui_pad_box(box, -padding);
}

void ui_center_of(const Box* box, i32* x, i32* y) {
  *x = box->x + box->w / 2;
  *y = box->y + box->h / 2;
}

void ui_render_tooltip(UI_state* ui, char* tooltip) {
  if (!tooltip) {
    return;
  }
  const Box box = ui->root.box;
  i32 x_center = 0, y_center = 0;
  ui_center_of(&box, &x_center, &y_center);
  if (x_center == 0 || y_center == 0) {
    return;
  }

  const Font font = assets.font;
  const i32 font_size = FONT_SIZE;
  const i32 spacing = 0;
  Vector2 text_size = MeasureTextEx(font, tooltip, font_size, spacing);

  i32 x = ui->mouse.x - text_size.x / 2;
  i32 y = ui->mouse.y - text_size.y / 2;
  i32 x_offset = 0;
  i32 y_offset = 0;

  Vector2 offset_from_center_factor = (Vector2) {
    .x = -((x / (f32)x_center) - 1.0f),
    .y = -((y / (f32)y_center) - 1.0f),
  };

  offset_from_center_factor.x = offset_from_center_factor.x / fabs(offset_from_center_factor.x);
  offset_from_center_factor.y = offset_from_center_factor.y / fabs(offset_from_center_factor.y);

  const i32 base_x_offset = 10 + UI_PADDING;
  const i32 base_y_offset = 10 + UI_PADDING;

  x_offset += offset_from_center_factor.x * text_size.x * 0.5f + (offset_from_center_factor.x * base_x_offset);
  y_offset += offset_from_center_factor.y * text_size.y * 0.5f + (offset_from_center_factor.y * base_y_offset);

  x += x_offset;
  y += y_offset;

  const Color background_color = UI_BACKGROUND_COLOR;
  const i32 segments = 8;
  const f32 roundness = UI_BUTTON_ROUNDNESS;
  const f32 border_thickness = UI_BORDER_THICKNESS;
  const Color border_color = UI_BORDER_COLOR;
  {
    Box box = ui_expand_box(BOX(x, y, text_size.x, text_size.y), UI_PADDING);
    if (roundness > 0) {
      DrawRectangleRounded((Rectangle) { box.x, box.y, box.w, box.h }, roundness, segments, background_color);
      DrawRectangleRoundedLines((Rectangle) { box.x, box.y, box.w, box.h }, roundness, segments, border_thickness, border_color);
    }
    else {
      DrawRectangle(box.x, box.y, box.w, box.h, background_color);
      DrawRectangleLinesEx((Rectangle) {box.x, box.y, box.w, box.h }, border_thickness, border_color);
    }
    DrawTextEx(font, tooltip, (Vector2) { x, y }, font_size, spacing, COLOR_RGB(255, 255, 255));
  }
}

void ui_render_tooltip_of_element(UI_state* ui, Element* e) {
  static char tooltip[256] = {0};
  switch (e->type) {
    case ELEMENT_SLIDER: {
      if (e->data.slider.type == SLIDER_FLOAT) {
        stb_snprintf(tooltip, sizeof(tooltip), "range: %g, %g\nvalue: %g", e->data.slider.range.f_min, e->data.slider.range.f_max, *e->data.slider.v.f);
        break;
      }
      stb_snprintf(tooltip, sizeof(tooltip), "range: %d, %d\nvalue: %d", e->data.slider.range.i_min, e->data.slider.range.i_max, *e->data.slider.v.i);
      break;
    }
    default:
      break;
  }
  if (*tooltip != 0) {
    ui_render_tooltip(ui, tooltip);
  }
  memset(tooltip, 0, sizeof(tooltip));
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

void ui_onconnect(struct Element* e, struct Element* target) {
  (void)e;
  (void)target;
#if 0
  // hehehe liitl swapparoo
  Element copy = *e;
  *e = *target;
  *target = copy;
#endif
}

bool ui_connection_filter(struct Element* e, struct Element* target) {
  return (e != NULL) && (target != NULL);
}

Result ui_init(void) {
  ui_state_init(&ui_state);
  return Ok;
}

void ui_update(f32 dt) {
  TIMER_START();
  UI_state* ui = &ui_state;
  ui->latency = 0;
  ui->dt = dt;
  ui->timer += dt;

  Element* root = &ui->root;
  root->box = BOX(0, 0, GetScreenWidth(), GetScreenHeight());
  arena_reset(&ui->frame_arena);

#ifdef UI_DRAW_GUIDES
  ONLY_DRAW_GUIDE_ON_HOVER = true;
  if (IsKeyDown(KEY_LEFT_CONTROL)) {
    ONLY_DRAW_GUIDE_ON_HOVER = false;
  }
#endif

  ui->element_update_count = 0;
  ui->element_render_count = 0;
  ui->prev_mouse = ui->mouse;
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

  bool mod_key = IsKeyDown(KEY_LEFT_CONTROL);

  if (ui->marker && !mod_key) {
    ui->marker = NULL;
  }

  if (ui->hover == ui->select && ui->hover) {
    if (ui->select->id == ui->active_id) {
      if (mod_key) {
        if (ui->marker && ui->marker != ui->select) {
          ui->marker->onconnect(ui->marker, ui->select);
          ui->marker = NULL;
        }
        else {
          ui->marker = ui->select;
        }
      }
      else {
        if (ui->select->type == ELEMENT_TOGGLE) {
          ui_toggle_onclick(ui->select);
        }
        ui->select->onclick(ui->select);
      }
    }
    ui->active_id = 0;
  }
  if (ui->hover == ui->active && ui->hover) {
    if (ui->active->id == ui->active_id && ui->active->type == ELEMENT_SLIDER && !mod_key) {
      ui_slider_onclick(ui, ui->active);
    }
  }
  if (ui->hover != NULL) {
    ui->hover->tooltip_timer += ui->dt;
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
#ifdef UI_DRAW_GUIDES
  if (ui->hover != NULL) {
    Element* e = ui->hover;
    DrawRectangleLinesEx((Rectangle) { e->box.x, e->box.y, e->box.w, e->box.h}, 1.0f, GUIDE_COLOR2);
    DRAW_SIMPLE_TEXT2(e->box.x + 4, e->box.y + 4, GUIDE_TEXT_COLOR, "timer: %g", e->tooltip_timer);
  }
#endif
  static const Color marker_color_bright = COLOR_RGB(80, 200, 80);
  static const Color marker_color_dim    = COLOR_RGB(60, 140, 60);

  if (ui->marker != NULL) {
    const Color color = lerp_color(marker_color_dim, marker_color_bright, (1 + sinf(12*ui->timer)) * 0.5f);
    Element* e = ui->marker;
    if (e->roundness > 0) {
      const i32 segments = 8;
      DrawRectangleRoundedLines((Rectangle) { e->box.x, e->box.y, e->box.w, e->box.h}, e->roundness, segments, 1.1f, color);
    }
    else {
      DrawRectangleLinesEx((Rectangle) { e->box.x, e->box.y, e->box.w, e->box.h}, 1.1f, color);
    }
  }
  if (ui->hover != NULL) {
    Element* e = ui->hover;
    if (ui->marker) {

      // TODO(lucas): improve filtering
      if (ui->connection_filter(ui->marker, ui->hover)) {
        const Color color = lerp_color(marker_color_dim, marker_color_bright, (1 + sinf(12*ui->timer)) * 0.5f);
        Element* e = ui->hover;
        if (e->roundness > 0) {
          const i32 segments = 8;
          DrawRectangleRoundedLines((Rectangle) { e->box.x, e->box.y, e->box.w, e->box.h}, e->roundness, segments, 1.1f, color);
        }
        else {
          DrawRectangleLinesEx((Rectangle) { e->box.x, e->box.y, e->box.w, e->box.h}, 1.1f, color);
        }
        i32 x1, y1, x2, y2;
        ui_center_of(&ui->marker->box, &x1, &y1);
        ui_center_of(&e->box, &x2, &y2);
        DrawLine(x1, y1, x2, y2, color);
      }
    }
    if (e->tooltip_timer >= UI_TOOLTIP_DELAY) {
      if (e->tooltip) {
        ui_render_tooltip(ui, e->tooltip);
      }
      else {
        ui_render_tooltip_of_element(ui, e);
      }
    }
  }
  ui->latency += TIMER_END();
  ui->render_latency = TIMER_END();
}

void ui_free(void) {
  UI_state* ui = &ui_state;
  ui_hierarchy_print();
  ui_free_elements(ui, &ui->root);
  close(ui->fd);
  ui->fd = -1;
  arena_free(&ui->frame_arena);
}

void ui_set_slider_deadzone(f32 deadzone) {
  UI_state* ui = &ui_state;
  ui->slider_deadzone = CLAMP(deadzone, 0.0f, 1.0f);
}

void ui_set_connection_filter(bool (*filter)(struct Element*, struct Element*)) {
  UI_state* ui = &ui_state;
  ui->connection_filter = filter;
}

void ui_reset_connection_filter(void) {
  UI_state* ui = &ui_state;
  ui->connection_filter = ui_connection_filter;
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

Element ui_none(void) {
  Element e;
  ui_element_init(&e, ELEMENT_NONE);
  e.render = false;
  return e;
}

Element ui_container(char* title) {
  Element e;
  ui_element_init(&e, ELEMENT_CONTAINER);
  e.render = true;
  e.scissor = true;
  e.title_bar = (Title_bar) {
    .title = title,
    .padding = UI_TITLE_BAR_PADDING,
    .top = true,
  };
  e.data.container.title = title;
  e.data.container.title_padding = UI_TITLE_BAR_PADDING;
  if (title) {
    e.roundness = 0;
  }
  return e;
}

Element ui_grid(u32 cols, bool render) {
  Element e;
  ui_element_init(&e, ELEMENT_GRID);
  e.data.grid.cols = cols;
  e.render = render;
  e.roundness = 0;
  return e;
}

Element ui_text(char* text) {
  Element e;
  ui_element_init(&e, ELEMENT_TEXT);
  e.data.text.string = text;
  e.background = false;
  e.border = false;
  e.scissor = false;
  return e;
}

Element ui_button(char* text) {
  Element e;
  ui_element_init(&e, ELEMENT_BUTTON);
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
  ui_element_init(&e, ELEMENT_CANVAS);
  e.type = ELEMENT_CANVAS;
  e.background = true;
  e.border = border;
  e.scissor = false;
  return e;
}

Element ui_toggle(i32* value) {
  ASSERT(value != NULL);
  Element e;
  ui_element_init(&e, ELEMENT_TOGGLE);
  e.data.toggle.value = value;
  e.data.toggle.text[0] = NULL;
  e.data.toggle.text[1] = NULL;
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
  e.data.toggle.text[0] = text;
  e.data.toggle.text[1] = text;
  return e;
}

Element ui_toggle_ex2(i32* value, char* false_text, char* true_text) {
  Element e = ui_toggle(value);
  e.data.toggle.text[0] = false_text;
  e.data.toggle.text[1] = true_text;
  return e;
}

Element ui_slider(void* value, Slider_type type, Range range) {
  ASSERT(value != NULL);
  UI_state* ui = &ui_state;

  Element e;
  ui_element_init(&e, ELEMENT_SLIDER);
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
  e.data.slider.deadzone = ui->slider_deadzone;
  e.type = ELEMENT_SLIDER;
  e.scissor = false;
  e.background = true;
  e.border = true;
  e.roundness = UI_BUTTON_ROUNDNESS;
  e.background_color = lerp_color(UI_BACKGROUND_COLOR, COLOR_RGB(255, 255, 255), 0.2f);
  return e;
}

Element ui_line_break(i32 height) {
  Element e = ui_none();
  e.box.h = height;
  e.sizing = SIZING_PERCENT(100, 0);
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
