// ui.c
// TODO:
//  - string formatting of text elements
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

#define KEY_PRESSED(KEY) (IsKeyPressed(KEY) || IsKeyPressedRepeat(KEY))

#ifdef UI_DRAW_GUIDES
static Color GUIDE_COLOR             = COLOR_RGB(255, 0, 255);
static Color GUIDE_COLOR2            = COLOR_RGB(255, 100, 255);
static Color GUIDE_TEXT_COLOR        = COLOR_RGB(255, 180, 255);
static bool ONLY_DRAW_GUIDE_ON_HOVER = false;
#endif

UI_state ui_state = {0};

static void ui_state_init(UI_state* ui);
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
static void ui_onupdate(struct Element* e);
static void ui_onrender(struct Element* e);
static void ui_onconnect(struct Element* e, struct Element* target);
static void ui_oninput(struct Element* e, char ch);
static void ui_onenter(struct Element* e);
static bool ui_connection_filter(struct Element* e, struct Element* target);
static void ui_print_elements(UI_state* ui, i32 fd, Element* e, u32 level);
static void tabs(i32 fd, const u32 count);
static void ui_render_rectangle(Box box, f32 roundness, Color color);
static void ui_render_rectangle_lines(Box box, f32 thickness, f32 roundness, Color color);
// returns true if box was mutated
static bool ui_measure_text(Font, char* text, Box* box, bool allow_overflow, bool text_wrapping, i32 font_size, i32 spacing, i32 line_spacing);
static void ui_render_text(Font font, char* text, const Box* box, bool text_wrapping, i32 font_size, i32 spacing, i32 line_spacing, Color tint);
static void ui_update_input(UI_state* ui, Element* e);
static void ui_render_input(Element* e);

void ui_state_init(UI_state* ui) {
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
  ui->container = NULL;
  ui->input = NULL;
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
  ui->tooltip_timer = 0.0f;
  ui->slider_deadzone = 0.0f;
  ui->connection_filter = ui_connection_filter;
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
    if (e->type == ELEMENT_CONTAINER) {
      if (e->data.container.scrollable) {
        ui->container = e;
      }
    }
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
      const i32 font_size = FONT_SIZE;
      const i32 spacing = 0;
      const Sizing sizing = e->sizing;
      i32 w = e->box.w;
      i32 h = e->box.h;
      const bool allow_overflow = e->data.text.allow_overflow;
      const bool text_wrapping = e->data.text.text_wrapping;
      if (sizing.mode == SIZE_MODE_PERCENT) {
        if (sizing.x != 0) {
          w = e->box.w;
        }
        if (sizing.y != 0) {
          h = e->box.h;
        }
      }
      if (!ui_measure_text(font, text, &e->box, allow_overflow, text_wrapping, font_size, spacing, UI_LINE_SPACING)) {
        e->box.w = w;
        e->box.h = h;
      }
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
  e->onupdate(e);
  for (size_t i = 0; i < e->count; ++i) {
    Element* item = &e->items[i];
    ui_update_elements(ui, item);
  }
}

void ui_update_container(UI_state* ui, Element* e) {
  (void)ui; // unused

  i32 scroll_y = e->data.container.scroll_y;

  // placement offsets
  i32 px = 0;
  i32 py = scroll_y;
  // block placement offsets
  i32 py_offset = 0; // element with the greatest height

  bool hide = false;
  bool done = false;
  for (size_t i = 0; i < e->count && !done; ++i) {
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
        // this fitment only allows one item
        done = true;
        break;
      }
      case PLACEMENT_ROWS: {
        if (py >= e->box.h) {
          item->hidden = hide = true;
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
  e->data.container.content_height = py + py_offset - scroll_y;
}

void ui_update_grid(UI_state* ui, Element* e) {
  (void)ui;
  const u32 cols = e->data.grid.cols;
  const u32 rows = (u32)ceilf((f32)e->count / cols);
  for (size_t i = 0; i < e->count; ++i) {
    Element* item = &e->items[i];
    const i32 w = (i32)ceilf((f32)(e->box.w - 2 * e->padding) / cols);
    const i32 h = (i32)ceilf((f32)e->box.h / rows);
    const i32 x = i % cols;
    const i32 y = (i32)floorf((f32)i / cols);
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

  if (e->background) {
    ui_render_rectangle(e->box, e->roundness, background_color);
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
      char* text = e->data.text.string;
      if (!text) {
        break;
      }
      const Font font = assets.font;
      const i32 font_size = FONT_SIZE;
      const i32 spacing = 0;
      const bool text_wrapping = e->data.text.text_wrapping;
      ui_render_text(font, text, &e->box, text_wrapping, font_size, spacing, UI_LINE_SPACING, e->text_color);
#ifdef UI_DRAW_GUIDES
      ui_render_rectangle_lines(e->box, 1, 0, GUIDE_COLOR2);
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
      ui_render_rectangle(box, e->roundness, line_color);
      Range range = e->data.slider.range;
      i32 x = box.x;
      i32 y = box.y;
      i32 radius = UI_SLIDER_KNOB_SIZE;
      i32 h = box.h;
      f32 factor = 0.0f;
      switch (e->data.slider.type) {
        case VALUE_TYPE_FLOAT: {
          f32 range_length = -(range.f_min - range.f_max);
          factor = (*e->data.slider.v.f - range.f_min) / range_length;
          break;
        }
        case VALUE_TYPE_INTEGER: {
          i32 range_length = -(range.i_min - range.i_max);
          factor = (*e->data.slider.v.i - range.i_min) / (f32)range_length;
          break;
        }
        default:
            break;
      }
      factor = CLAMP(factor, 0.0f, 1.0f);
      ui_render_rectangle(BOX(box.x, box.y, box.w * factor, box.h), e->roundness, lerp_color(UI_BUTTON_COLOR, COLOR_RGB(255, 255, 255), 0.2f));
      DrawCircle(x + box.w*factor, y + h/2, radius, UI_BUTTON_COLOR);
      if (e->border_thickness > 0.0f) {
        DrawCircleLines(x + box.w*factor, y + h/2, radius, e->border_color);
      }
      break;
    }
    case ELEMENT_INPUT: {
      const Font font = assets.font;
      const i32 font_size = FONT_SIZE;
      const i32 spacing = 0;
      if (e->data.input.buffer.count > 0) {
        ui_render_text(font, (char*)e->data.input.buffer.data, &e->box, false, font_size, spacing, UI_LINE_SPACING, e->text_color);
        break;
      }
      char* preview = e->data.input.preview;
      if (preview) {
        ui_render_text(font, preview, &e->box, false, font_size, spacing, UI_LINE_SPACING, lerp_color(e->text_color, COLOR_RGB(0, 0, 0), 0.2f));
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

  if (e->border) {
    ui_render_rectangle_lines(e->box, e->border_thickness, e->roundness, e->border_color);
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
  if (e->type == ELEMENT_INPUT) {
    buffer_free(&e->data.input.buffer);
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
  e->v.i = 0;
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
  e->tooltip = NULL;

  e->onclick = ui_onclick;
  e->onupdate = ui_onupdate;
  e->onrender = ui_onrender;
  e->onconnect = ui_onconnect;
  e->oninput = ui_oninput;
  e->onenter = ui_onenter;
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
      if (e->data.slider.type == VALUE_TYPE_FLOAT) {
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

void ui_onupdate(struct Element* e) {
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
    case VALUE_TYPE_FLOAT: {
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
    case VALUE_TYPE_INTEGER: {
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

void ui_oninput(struct Element* e, char ch) {
  (void)e;
  (void)ch;
}

void ui_onenter(struct Element* e) {
  (void)e;
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
  ui->container = NULL;

  ui_update_elements(ui, root);
  if (ui->active) {
    if (ui->active_id == 0) {
      ui->active_id = ui->active->id;
    }
  }

  bool mod_key = IsKeyDown(KEY_LEFT_CONTROL);

  if (ui->select != NULL) {
    if (ui->select->type == ELEMENT_INPUT && ui->select->id == ui->active_id) {
      ui->input = ui->select;
    }
    else {
      ui->input = NULL;
    }
  }
  if (ui->input) {
    ui_update_input(ui, ui->input);
  }

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
  if (((i32)ui->mouse.x != (i32)ui->prev_mouse.x) || ((i32)ui->mouse.y != (i32)ui->prev_mouse.y)) {
    ui->tooltip_timer = 0.0f;
  }
  if (ui->hover != NULL) {
    ui->tooltip_timer += ui->dt;
  }
  if (ui->container != NULL) {
    ASSERT(ui->container->type == ELEMENT_CONTAINER);
    Element* e = ui->container;
    Vector2 wheel = GetMouseWheelMoveV();
    i32 scroll_y = e->data.container.scroll_y;
    i32 content_height = e->data.container.content_height;
    if (content_height > e->box.h || scroll_y < 0) {
      scroll_y += wheel.y * UI_SCROLL_SPEED;
      if (scroll_y > 0) {
        scroll_y = 0;
      }
      i32 content_height_delta = content_height - e->box.h;
      if (-scroll_y > content_height_delta) {
        scroll_y = -content_height_delta;
      }
      e->data.container.scroll_y = scroll_y;
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
      case ELEMENT_INPUT: {
        cursor = MOUSE_CURSOR_IBEAM;
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
  if (ui->fd >= 0) {
    ui_print_elements(ui, ui->fd, &ui->root, 0);
  }
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
    if (e->type == ELEMENT_CONTAINER) {
      DRAW_SIMPLE_TEXT2(e->box.x + 4, e->box.y + 4, GUIDE_TEXT_COLOR, "content_height: %d, box.h: %d", e->data.container.content_height, e->box.h);
    }
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
  if (ui->input != NULL) {
    ui_render_input(ui->input);
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
    if (ui->tooltip_timer >= UI_TOOLTIP_DELAY) {
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

bool ui_no_input(void) {
  UI_state* ui = &ui_state;
  return ui->input == NULL;
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
  e.data.container.scroll_x = 0;
  e.data.container.scroll_y = 0;
  e.data.container.content_height = 0;
  e.data.container.scrollable = true;

  e.render = true;
  e.scissor = true;
  e.title_bar = (Title_bar) {
    .title = title,
    .padding = UI_TITLE_BAR_PADDING,
    .top = true,
  };
  if (title) {
    e.roundness = 0;
  }
  return e;
}

Element ui_container_ex(char* title, bool scrollable) {
  Element e = ui_container(title);
  e.data.container.scrollable = scrollable;
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
  e.data.text.allow_overflow = false;
  e.data.text.text_wrapping = true;
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

Element ui_slider(void* value, Value_type type, Range range) {
  ASSERT(value != NULL);
  UI_state* ui = &ui_state;

  Element e;
  ui_element_init(&e, ELEMENT_SLIDER);
  switch (type) {
    case VALUE_TYPE_FLOAT:
      e.data.slider.v.f = (f32*)value;
      break;
    case VALUE_TYPE_INTEGER:
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

Element ui_input(char* preview) {
  Element e;
  ui_element_init(&e, ELEMENT_INPUT);
  e.box.h = FONT_SIZE;
  e.data.input.buffer = buffer_new(0);
  e.data.input.cursor = 0;
  e.data.input.preview = preview;
  e.data.input.input_type = INPUT_TEXT;
  e.data.input.value_type = VALUE_TYPE_NONE;
  e.data.input.value = NULL;
  e.background = true;
  e.border = true;
  e.scissor = true;
  e.roundness = UI_BUTTON_ROUNDNESS;
  e.background_color = lerp_color(UI_BACKGROUND_COLOR, COLOR_RGB(255, 255, 255), 0.2f);
  return e;
}

Element ui_input_ex(char* preview, Input_type input_type) {
  Element e = ui_input(preview);
  e.data.input.input_type = input_type;
  return e;
}

Element ui_input_ex2(char* preview, void* value, Input_type input_type, Value_type value_type) {
  ASSERT(value != NULL);

  Element e = ui_input_ex(preview, input_type);
  e.data.input.value_type = value_type;
  e.data.input.value = value;
  switch (value_type) {
    case VALUE_TYPE_FLOAT: {
      e.data.input.buffer = buffer_new_from_fmt(32, "%g", *(f32*)value);
      break;
    }
    case VALUE_TYPE_INTEGER: {
      e.data.input.buffer = buffer_new_from_fmt(32, "%d", *(i32*)value);
      break;
    }
    default:
      break;
  }
  e.data.input.cursor = e.data.input.buffer.count;
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

void ui_render_rectangle(Box box, f32 roundness, Color color) {
  const i32 segments = 8;
  if (roundness > 0) {
    DrawRectangleRounded((Rectangle) { box.x, box.y, box.w, box.h }, roundness, segments, color);
    return;
  }
  DrawRectangle(box.x, box.y, box.w, box.h, color);
}

void ui_render_rectangle_lines(Box box, f32 thickness, f32 roundness, Color color) {
  const i32 segments = 8;
  if (roundness > 0) {
    DrawRectangleRoundedLines((Rectangle) { box.x, box.y, box.w, box.h }, roundness, segments, thickness, color);
    return;
  }
  DrawRectangleLinesEx((Rectangle) { box.x, box.y, box.w, box.h }, thickness, color);
}

bool ui_measure_text(
    Font font,
    char* text,
    Box* box,
    bool allow_overflow,
    bool text_wrapping,
    i32 font_size,
    i32 spacing,
    i32 line_spacing) {
  bool mutated = false;
  if (font.texture.id == 0) {
    font = GetFontDefault();
  }
  size_t length = strlen(text);
  f32 x_offset = 0.0f;
  f32 x_offset_largest = 0.0f;
  i32 y_offset = 0;

  f32 scale_factor = font_size / (f32)font.baseSize;
  i32 max_line_width = 0;
  if (text_wrapping) {
    max_line_width = box->w;
  }

  for (size_t i = 0; i < length;) {
    i32 codepoint_size = 0;
    i32 codepoint = GetCodepointNext(&text[i], &codepoint_size);
    i32 glyph_index = GetGlyphIndex(font, codepoint);
    if (x_offset >= max_line_width && max_line_width > 0) {
      x_offset = 0.0f;
      y_offset += line_spacing;
    }
    if (codepoint == '\n') {
      x_offset = 0.0f;
      y_offset += line_spacing;
    }
    else {
      if (font.glyphs[glyph_index].advanceX == 0) {
        x_offset += ((f32)font.recs[glyph_index].width * scale_factor + spacing);
      }
      else {
        x_offset += ((f32)font.glyphs[glyph_index].advanceX * scale_factor + spacing);
      }
      if (x_offset > x_offset_largest) {
        x_offset_largest = x_offset;
      }
    }
    i += codepoint_size;
  }

  if (!allow_overflow) {
    box->h = y_offset + line_spacing;
    if (!text_wrapping) {
      box->w = x_offset_largest;
    }
    mutated = true;
  }

  return mutated;
}

void ui_render_text(
    Font font,
    char* text,
    const Box* box,
    bool text_wrapping,
    i32 font_size,
    i32 spacing,
    i32 line_spacing,
    Color tint) {
  if (font.texture.id == 0) {
    font = GetFontDefault();
  }
  size_t length = strlen(text);
  if (length == 0) {
    return;
  }

  f32 x_offset = 0;
  i32 y_offset = 0;

  f32 scale_factor = font_size / (f32)font.baseSize;

  i32 max_line_width = 0;
  if (text_wrapping) {
    max_line_width = box->w;
  }

  for (size_t i = 0; i < length;) {
    i32 codepoint_size = 0;
    i32 codepoint = GetCodepointNext(&text[i], &codepoint_size);
    i32 glyph_index = GetGlyphIndex(font, codepoint);

    if (x_offset >= max_line_width && max_line_width > 0) {
      x_offset = 0.0f;
      y_offset += line_spacing;
    }

    if (codepoint == '\n') {
      y_offset += line_spacing;
      x_offset = 0.0f;
    }
    else {
      if ((codepoint != ' ') && (codepoint != '\t')) {
        DrawTextCodepoint(font, codepoint, (Vector2) { box->x + x_offset, box->y + y_offset }, font_size, tint);
      }

      if (font.glyphs[glyph_index].advanceX == 0) {
        x_offset += ((f32)font.recs[glyph_index].width*scale_factor + spacing);
      }
      else {
        x_offset += ((f32)font.glyphs[glyph_index].advanceX*scale_factor + spacing);
      }
    }
    i += codepoint_size;
  }
}

void ui_update_input(UI_state* ui, Element* e) {
  Buffer* buffer = &e->data.input.buffer;
  char ch = 0;
  while ((ch = GetCharPressed()) != 0) {
    switch (e->data.input.input_type) {
      case INPUT_TEXT: {
        buffer_insert(buffer, ch, e->data.input.cursor);
        e->data.input.cursor += 1;
        break;
      }
      case INPUT_NUMBER: {
        if ((ch >= '0' && ch <= '9') || ch == '.') {
          buffer_insert(buffer, ch, e->data.input.cursor);
          e->data.input.cursor += 1;
        }
        break;
      }
      default:
        break;
    }
    e->oninput(e, ch);
  }

  if (KEY_PRESSED(KEY_BACKSPACE)) {
    if (e->data.input.cursor > 0) {
      buffer_erase(buffer, e->data.input.cursor - 1);
      e->data.input.cursor -= 1;
    }
  }
  if (KEY_PRESSED(KEY_LEFT)) {
    if (e->data.input.cursor > 0) {
      e->data.input.cursor -= 1;
    }
  }
  if (KEY_PRESSED(KEY_RIGHT)) {
    e->data.input.cursor += 1;
    if (e->data.input.cursor > buffer->count) {
      e->data.input.cursor = buffer->count;
    }
  }
  if (IsKeyPressed(KEY_ENTER)) {
    if (e->data.input.value != NULL) {
      Buffer* buffer = &e->data.input.buffer;
      switch (e->data.input.value_type) {
        case VALUE_TYPE_FLOAT: {
          f32* value = (f32*)e->data.input.value;
          *value = buffer_to_float(buffer);
          break;
        }
        case VALUE_TYPE_INTEGER: {
          i32* value = (i32*)e->data.input.value;
          *value = buffer_to_int(buffer);
          break;
        }
        default:
          break;
      }
    }
    ui->input = NULL;
    e->onenter(e);
    if (e->data.input.callback) {
      e->data.input.callback(&e->data.input);
    }
  }
}

void ui_render_input(Element* e) {
  i32 cursor = e->data.input.cursor;
  ui_render_rectangle_lines(e->box, e->border_thickness, e->roundness, UI_FOCUS_COLOR);
  Box cursor_box = BOX(
    e->box.x + cursor * FONT_SIZE/2,
    e->box.y,
    1,
    e->box.h
  );
  cursor_box = ui_pad_box(cursor_box, 2);
  cursor_box.w = 1;
  ui_render_rectangle(cursor_box, 0, UI_TEXT_COLOR);
}
