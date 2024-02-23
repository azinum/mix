// ui.c
// TODO:
//  - use bounding box when rendering text to not draw text that is invisible
//  - string formatting of text elements
//  - icon/image
//  - container tabs
//  - drag and drop (from external file browser)
//  - multiple ui states to switch between
//  - implement an element `watcher` mechanism that notifies external references
//      to elements that changes, for instance when reallocating or detaching ui nodes. external
//      element references must not be invalidated.
//  - improve scissor
//  - make layout design more versatile (constraints, vertical+horizontal padding, min/max sizes, e.t.c)
//  - floating containers
//  - drop-down menu

#define DRAW_SIMPLE_TEXT_EX(X, Y, SIZE, COLOR, FORMAT_STR, ...) do { \
  static char _text##__LINE__[SIZE] = {0}; \
  stb_snprintf(_text##__LINE__, sizeof(_text##__LINE__), FORMAT_STR, ##__VA_ARGS__); \
  DrawText(_text##__LINE__, X, Y, FONT_SIZE_SMALLEST, COLOR); \
} while (0)

#define DRAW_SIMPLE_TEXT(X, Y, FORMAT_STR, ...) DRAW_SIMPLE_TEXT_EX(X, Y, 64, COLOR_RGB(255, 255, 255), FORMAT_STR, ##__VA_ARGS__)
#define DRAW_SIMPLE_TEXT2(X, Y, COLOR, FORMAT_STR, ...) DRAW_SIMPLE_TEXT_EX(X, Y, 64, COLOR, FORMAT_STR, ##__VA_ARGS__)

#define KEY_PRESSED(KEY) (IsKeyPressed(KEY) || IsKeyPressedRepeat(KEY))

#ifdef UI_DRAW_GUIDES
#define GUIDE_COLOR_ALPHA 100
static Color GUIDE_COLOR             = COLOR(255, 0, 255, GUIDE_COLOR_ALPHA);
static Color GUIDE_COLOR2            = COLOR(255, 100, 255, GUIDE_COLOR_ALPHA);
static Color GUIDE_COLOR3            = COLOR(100, 100, 255, GUIDE_COLOR_ALPHA);
static Color GUIDE_TEXT_COLOR        = COLOR_RGB(100, 255, 100);
static bool ONLY_DRAW_GUIDE_ON_HOVER = false;
#undef GUIDE_COLOR_ALPHA
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
static bool ui_container_is_scrollable(Element* e);
static bool ui_container_is_scroll_at_top(Element* e);
static bool ui_container_is_scroll_at_bottom(Element* e);
static Box  ui_pad_box(Box box, i32 padding);
static Box  ui_pad_box_ex(Box box, i32 x_padding, i32 y_padding);
static Box  ui_expand_box(Box box, i32 padding);
static Box  ui_expand_box_ex(Box box, i32 x_padding, i32 y_padding);
static void ui_center_of(const Box* box, i32* x, i32* y);
static void ui_render_tooltip(UI_state* ui, char* tooltip);
static void ui_render_tooltip_of_element(UI_state* ui, Element* e);
static void ui_render_alert(UI_state* ui);
static Hash ui_hash(const u8* data, const size_t size);
static void ui_onclick(struct Element* e);
static void ui_toggle_onclick(struct Element* e);
static void ui_slider_onclick(UI_state* ui, struct Element* e);
static void ui_input_onclick(UI_state* ui, struct Element* e);
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
static void ui_render_text(Font font, char* text, const Box* box, bool allow_overflow, bool text_wrapping, i32 font_size, i32 spacing, i32 line_spacing, Color tint);
static void ui_update_input(UI_state* ui, Element* e);
static void ui_render_input(UI_state* ui, Element* e);

void ui_state_init(UI_state* ui) {
  ui_element_init(&ui->root, ELEMENT_CONTAINER);
  ui->root.placement = PLACEMENT_FILL;
  ui->root.padding = 0;
  ui->root.border = false;
  ui->root.background = false;

  ui->element_count = 1;
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
  ui->scrollable = NULL;
  ui->input = NULL;
  ui->zoom = NULL;
  ui->zoom_box = BOX(0, 0, 0, 0);
  ui->zoom_sizing = SIZING_PERCENT(0, 0);

#ifdef UI_LOG_HIERARCHY
  ui->fd = open(UI_LOG_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0664);
  if (ui->fd < 0) {
    log_print(STDERR_FILENO, LOG_TAG_ERROR, "failed to open file `%s` for writing\n", UI_LOG_PATH);
  }
#else
  ui->fd = -1;
#endif
  ui->active_id = 0;
  MEMORY_TAG("ui.ui_state_init: frame arena");
  ui->frame_arena = arena_new(UI_FRAME_ARENA_SIZE);
  ui->dt = 0.0f;
  ui->timer = 0.0f;
  ui->tooltip_timer = 0.0f;
  ui->blink_timer = 0.0f;
  ui->scrollbar_timer = 0.0f;
  ui->alert_timer = 0.0f;
  ui->slider_deadzone = 0.0f;
  ui->connection_filter = ui_connection_filter;
  memset(&ui->alert_text, 0, sizeof(ui->alert_text));
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
      ui->container = e;
      if (ui_container_is_scrollable(e)) {
        if (
            (!ui_container_is_scroll_at_top(e) && ui->scroll.y > 0)
            ||
            (!ui_container_is_scroll_at_bottom(e) && ui->scroll.y < 0)
        ) {
          ui->scrollable = e;
        }
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
      const bool allow_overflow = e->data.text.allow_overflow;
      const bool text_wrapping = e->data.text.text_wrapping;
      ui_measure_text(font, text, &e->box, allow_overflow, text_wrapping, font_size, spacing, UI_LINE_SPACING);
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
    case ELEMENT_INPUT: {
      if (e->data.input.input_type == INPUT_NUMBER && e->data.input.value) {
        size_t size = 0;
        switch (e->data.input.value_type) {
          case VALUE_TYPE_FLOAT: {
            size = sizeof(f32);
            break;
          }
          case VALUE_TYPE_INTEGER: {
            size = sizeof(i32);
            break;
          }
          default: {
            ASSERT(!"invalid numerical value type of input element");
            break;
          }
        }
        Hash hash = ui_hash(e->data.input.value, size);
        if (hash != e->data.input.value_hash && ui->input != e) {
          e->data.input.value_hash = hash;
          switch (e->data.input.value_type) {
            case VALUE_TYPE_FLOAT: {
              f32 value = *(f32*)e->data.input.value;
              buffer_from_fmt(&e->data.input.buffer, 24, "%g", value);
              break;
            }
            case VALUE_TYPE_INTEGER: {
              buffer_from_fmt(&e->data.input.buffer, 24, "%d", *(i32*)e->data.input.value);
              break;
            }
            default:
              break;
          }
          e->data.input.cursor = e->data.input.buffer.count;
        }
      }
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

  Element* root = &ui->root;

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
    item->hidden = (item->box.y + item->box.h < e->box.y) || (item->box.y > e->box.y + e->box.h) ||
      ((item->box.y + item->box.h < root->box.y) || (item->box.y > root->box.y + root->box.h));
  }

  py_offset += 2 * e->padding;
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

  if ((e->type == ELEMENT_BUTTON || e->type == ELEMENT_TOGGLE || e->type == ELEMENT_SLIDER) && e == ui->hover && !e->readonly) {
    factor += 0.10f;
    if (e == ui->active) {
      factor += 0.20f;
    }
  }
  if (e->readonly) {
    factor = 0.0f;
  }
  background_color = lerp_color(background_color, invert_color(UI_INTERPOLATION_COLOR), factor);

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
      ui_render_text(font, text, &e->box, false, text_wrapping, font_size, spacing, UI_LINE_SPACING, e->text_color);
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
      Box box = ui_expand_box(e->box, -UI_SLIDER_INNER_PADDING);
      Color line_color = lerp_color(e->background_color, invert_color(UI_INTERPOLATION_COLOR), 0.2f);
      ui_render_rectangle(box, e->roundness, line_color);
      Range range = e->data.slider.range;
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
        default: {
          break;
        }
      }
      factor = CLAMP(factor, 0.0f, 1.0f);
      if (e->data.slider.vertical) {
        ui_render_rectangle(BOX(box.x, box.y + box.h - box.h * factor, box.w, box.h * factor), e->roundness, lerp_color(e->secondary_color, warmer_color(UI_INTERPOLATION_COLOR, 22), factor * 0.3f + 0.2f * (ui->active == e && !e->readonly)));
      }
      else {
        ui_render_rectangle(BOX(box.x, box.y, box.w * factor, box.h), e->roundness, lerp_color(e->secondary_color, warmer_color(UI_INTERPOLATION_COLOR, 22), factor * 0.3f + 0.2f * (ui->active == e && !e->readonly)));
      }
      break;
    }
    case ELEMENT_INPUT: {
      const Font font = assets.font;
      const i32 font_size = FONT_SIZE;
      const i32 spacing = 0;
      char* text = NULL;
      Color color = e->text_color;
      if (e->data.input.buffer.count > 0) {
        text = (char*)e->data.input.buffer.data;
      }
      else {
        color = lerp_color(e->text_color, invert_color(UI_INTERPOLATION_COLOR), 0.3f);
        text = e->data.input.preview;
      }
      if (text) {
        Box box = ui_expand_box_ex(e->box, 0, -(e->box.h*0.5f - FONT_SIZE*0.5f));
        ui_render_text(font, text, &box, true, false, font_size, spacing, UI_LINE_SPACING, color);
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
    DrawRectangle(x, y, w, h, UI_TITLE_BAR_COLOR);
    DrawTextEx(font, title_bar.title, (Vector2) { x + title_bar.padding, y + title_bar.padding }, font_size, spacing, e->text_color);
    if (e->border) {
      DrawRectangleLinesEx((Rectangle) { x, y, w, h}, e->border_thickness, e->border_color);
    }
  }

  if (ui->scrollable == e) {
    if (ui_container_is_scrollable(e) && ui->scrollbar_timer < UI_SCROLLBAR_DECAY) {
      const i32 content_height = e->data.container.content_height;
      const i32 height = e->box.h;
      const i32 scroll_y = e->data.container.scroll_y;
      const i32 height_delta = content_height - height;
      const f32 scroll_factor = -scroll_y / (f32)height_delta;
      const i32 scrollbar_width = 4;
      const i32 scrollbar_height = height * (height/(f32)content_height);
      Box box = BOX(
        e->box.x + e->box.w - scrollbar_width - e->border_thickness,
        e->box.y + scroll_factor * (e->box.h - scrollbar_height - 2 * e->border_thickness) + e->border_thickness,
        scrollbar_width,
        scrollbar_height);
      ui_render_rectangle(
        box,
        UI_ROUNDNESS,
        COLOR_RGB(127, 127, 127)
      );
    }
  }
#ifdef UI_DRAW_GUIDES
  if (e->type == ELEMENT_CONTAINER && e->padding > 0) {
    ui_render_rectangle_lines(ui_pad_box(e->box, e->padding), 1, 0, GUIDE_COLOR3);
  }
#endif
}

void ui_free_elements(UI_state* ui, Element* e) {
  if (!e) {
    return;
  }
  if (e->type == ELEMENT_INPUT) {
    buffer_free(&e->data.input.buffer);
  }
  if (ui->hover == e) {
    ui->hover = NULL;
  }
  if (ui->select == e) {
    ui->select = NULL;
  }
  if (ui->marker == e) {
    ui->marker = NULL;
  }
  if (ui->container == e) {
    ui->container = NULL;
  }
  if (ui->scrollable == e) {
    ui->scrollable = NULL;
  }
  if (ui->input == e) {
    ui->input = NULL;
  }
  if (ui->zoom == e) {
    ui->zoom = NULL;
  }
  for (size_t i = 0; i < e->count; ++i) {
    ui_free_elements(ui, &e->items[i]);
  }
  ui->element_count -= e->count;
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
  e->secondary_color = UI_BACKGROUND_COLOR;
  e->border_color = UI_BORDER_COLOR;

  e->render = true;
  e->background = false;
  e->border = false;
  e->scissor = false;
  e->hidden = false;
  e->readonly = false;

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

inline bool ui_overlap(i32 x, i32 y, Box box) {
  return (x >= box.x && x <= box.x + box.w)
    && (y >= box.y && y <= box.y + box.h);
}

bool ui_container_is_scrollable(Element* e) {
  const i32 content_height = e->data.container.content_height;
  const i32 scroll_y = e->data.container.scroll_y;
  return (content_height > e->box.h) || (scroll_y < 0);
}

bool ui_container_is_scroll_at_top(Element* e) {
  const i32 scroll_y = e->data.container.scroll_y;
  return scroll_y == 0;
}

bool ui_container_is_scroll_at_bottom(Element* e) {
  const i32 content_height = e->data.container.content_height;
  const i32 scroll_y = e->data.container.scroll_y;
  return scroll_y == -(content_height - e->box.h);
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

Box ui_expand_box_ex(Box box, i32 x_padding, i32 y_padding) {
  return ui_pad_box_ex(box, -x_padding, -y_padding);
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
        stb_snprintf(tooltip, sizeof(tooltip), "value: %g\nrange: %g, %g", *e->data.slider.v.f, e->data.slider.range.f_min, e->data.slider.range.f_max);
        break;
      }
      stb_snprintf(tooltip, sizeof(tooltip), "value: %d\nrange: %d, %d", *e->data.slider.v.i, e->data.slider.range.i_min, e->data.slider.range.i_max);
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

void ui_render_alert(UI_state* ui) {
  if (ui->alert_text[0] == 0) {
    return;
  }

  char* text = ui->alert_text;
  Element* root = &ui->root;
  const Font font = assets.font;
  const i32 font_size = FONT_SIZE;
  const i32 spacing = 0;
  const i32 line_spacing = UI_LINE_SPACING;
  Box box = BOX(0, 0, root->box.w * 0.3f, 0);
  ui_measure_text(
    font,
    text,
    &box,
    false, // allow overflow
    true,  // text wrapping
    font_size,
    spacing,
    line_spacing
  );
  box.x += (root->box.x + root->box.w - box.w - UI_PADDING) - 2 * UI_PADDING;
  box.y += (root->box.y) + 2 * UI_PADDING;

  Box padded_box = ui_expand_box_ex(box, 2 * UI_PADDING, UI_PADDING);
  ui_render_rectangle(padded_box, UI_ROUNDNESS, UI_BACKGROUND_COLOR);
  ui_render_rectangle_lines(padded_box, UI_BORDER_THICKNESS, UI_ROUNDNESS, UI_BORDER_COLOR);
  ui_render_text(font, ui->alert_text, &box, false, true, font_size, spacing, line_spacing, UI_TEXT_COLOR);
}

Hash ui_hash(const u8* data, const size_t size) {
  if (!data) {
    return 0;
  }
  return hash_djb2(data, size);
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
  if (e->readonly) {
    return;
  }
  Box box = ui_pad_box_ex(e->box, UI_SLIDER_INNER_PADDING, 2 * UI_SLIDER_INNER_PADDING);

  i32 delta = ui->mouse.x - box.x;
  f32 factor = delta / (f32)box.w;
  if (e->data.slider.vertical) {
    delta = ui->mouse.y - box.y;
    factor = 1.0f - (delta / (f32)box.h);
  }

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

void ui_input_onclick(UI_state* ui, struct Element* e) {
  (void)e;
  ui->blink_timer = 0;
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
  ui->blink_timer += dt;
  ui->scrollbar_timer += dt;
  ui->alert_timer -= dt;
  if (ui->alert_timer <= 0.0f) {
    ui->alert_timer = 0;
  }

  Element* root = &ui->root;
  if (ui->zoom) {
    root = ui->zoom;
  }

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
  ui->scroll = GetMouseWheelMoveV();
  if (KEY_PRESSED(KEY_DOWN)) {
    ui->scroll.y -= 1;
  }
  if (KEY_PRESSED(KEY_UP)) {
    ui->scroll.y += 1;
  }

#if defined(TARGET_ANDROID) || defined(UI_EMULATE_TOUCH_SCREEN)
  static Vector2 drag = {0, 0};
  static Vector2 prev_drag = {0, 0};
  i32 gesture = GetGestureDetected();
  if ((gesture & GESTURE_HOLD) || (gesture & GESTURE_DRAG)) {
    prev_drag = drag;
    drag = GetGestureDragVector();
    Vector2 delta = {
      prev_drag.x - drag.x,
      prev_drag.y - drag.y,
    };
    ui->scroll.x = -(delta.x * (root->box.w) / (f32)UI_SCROLL_SPEED);
    ui->scroll.y = -(delta.y * (root->box.h) / (f32)UI_SCROLL_SPEED);
  }
  else {
    prev_drag = drag = (Vector2) {0, 0};
  }
#endif

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
      HideSoftKeyboard();
    }
  }
  if (ui->input) {
    ui_update_input(ui, ui->input);
  }

  if (ui->marker && !mod_key) {
    ui->marker = NULL;
  }

  if (ui->hover == ui->select && ui->hover) {
    if (ui->select->id == ui->active_id && !ui->select->readonly) {
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
        else if (ui->select->type == ELEMENT_INPUT) {
          ui_input_onclick(ui, ui->select);
          ShowSoftKeyboard();
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
  if (ui->container != NULL) {
    bool do_zoom = mod_key && IsKeyPressed(KEY_F);
#if defined(TARGET_ANDROID) || defined(UI_EMULATE_TOUCH_SCREEN)
    i32 gesture = GetGestureDetected();
    do_zoom = do_zoom || (gesture & GESTURE_DOUBLETAP);
#endif
    if (do_zoom && !ui_input_interacting()) {
      if (ui->zoom) {
        ui->zoom->box = ui->zoom_box;
        ui->zoom->sizing = ui->zoom_sizing;
        ui->zoom = NULL;
      }
      else {
        ui->zoom = ui->container;
        ui->zoom_box = ui->zoom->box;
        ui->zoom_sizing = ui->zoom->sizing;
        ui->zoom->sizing = SIZING_PERCENT(100, 100);
      }
    }
  }
  if (((i32)ui->mouse.x != (i32)ui->prev_mouse.x) || ((i32)ui->mouse.y != (i32)ui->prev_mouse.y)) {
    ui->tooltip_timer = 0.0f;
    ui->scrollbar_timer = 0.0f;
  }
  if (ui->hover != NULL) {
    ui->tooltip_timer += ui->dt;
    ui->scrollbar_timer += ui->dt;
  }
  if (ui->scrollable != NULL) {
    ASSERT(ui->scrollable->type == ELEMENT_CONTAINER);
    if (ui->scrollable->data.container.scrollable) {
      Element* e = ui->scrollable;
      Vector2 wheel = ui->scroll;
      i32 scroll_y = e->data.container.scroll_y;
      i32 content_height = e->data.container.content_height;
      i32 content_height_delta = content_height - e->box.h;

      if (ui_container_is_scrollable(e)) {
        scroll_y += wheel.y * UI_SCROLL_SPEED;
        if (-scroll_y > content_height_delta) {
          scroll_y = -content_height_delta;
        }
        if (scroll_y > 0) {
          scroll_y = 0;
        }
        e->data.container.scroll_y = scroll_y;
        if (wheel.y != 0) {
          ui->scrollbar_timer = 0.0f;
        }
      }
    }
  }
  i32 cursor = MOUSE_CURSOR_DEFAULT;
  if (ui->hover) {
    if (!ui->hover->readonly) {
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
  if (ui->zoom) {
    root = ui->zoom;
  }
  ui_render_elements(ui, root);
#ifdef UI_DRAW_GUIDES
  if (ui->hover != NULL) {
    Element* e = ui->hover;
    DrawRectangleLinesEx((Rectangle) { e->box.x, e->box.y, e->box.w, e->box.h}, 1.0f, GUIDE_COLOR2);
    if (e->type == ELEMENT_CONTAINER) {
      DRAW_SIMPLE_TEXT2(e->box.x + 4, e->box.y + 4, GUIDE_TEXT_COLOR, "content_height: %d\nbox.h: %d\nscroll_y: %d", e->data.container.content_height, e->box.h, e->data.container.scroll_y);
    }
  }
#endif
  static const Color marker_color_bright = COLOR_RGB(80, 200, 80);
  static const Color marker_color_dim    = COLOR_RGB(60, 140, 60);

  if (ui->marker != NULL) {
    const Color color = lerp_color(marker_color_dim, marker_color_bright, (1 + sinf(12*ui->timer)) * 0.5f);
    Element* e = ui->marker;
    ui_render_rectangle_lines(e->box, 1.1f, e->roundness, color);
  }
  if (ui->input != NULL) {
    ui_render_input(ui, ui->input);
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
  if (ui->alert_timer > 0.0f) {
    ui_render_alert(ui);
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

void ui_set_title(Element* e, char* title) {
  if (!e) {
    return;
  }
  if (title) {
    e->title_bar.title = title;
    // NOTE(lucas): set roundness to 0 when using title bars because rounded titlebars are not supported yet
    e->roundness = 0;
  }
}

void ui_reset_connection_filter(void) {
  UI_state* ui = &ui_state;
  ui->connection_filter = ui_connection_filter;
}

bool ui_input_interacting(void) {
  UI_state* ui = &ui_state;
  return ui->input != NULL;
}

void ui_alert_simple(const char* message) {
  ui_alert("%s", message);
}

void ui_alert(const char* format, ...) {
  UI_state* ui = &ui_state;
  va_list argp;
  va_start(argp, format);
  vsnprintf(ui->alert_text, sizeof(ui->alert_text), format, argp);
  va_end(argp);
  ui->alert_timer = UI_ALERT_DECAY;
}

Element* ui_attach_element(Element* target, Element* e) {
  UI_state* ui = &ui_state;
  if (!target) {
    target = &ui->root;
  }
  size_t index = target->count;
  e->id = ui->id_counter++;
  MEMORY_TAG("ui.ui_attach_element");
  list_push(target, *e);
  ui->element_count += 1;
  return &target->items[index];
}

Element* ui_attach_element_v2(Element* target, Element e) {
  return ui_attach_element(target, &e);
}

void ui_detach_elements(Element* e) {
  if (!e) {
    return;
  }
  if (e->type == ELEMENT_CONTAINER) {
    e->data.container.scroll_y = 0;
  }
  UI_state* ui = &ui_state;
  ui_free_elements(ui, e);
  e->items = NULL;
}

void ui_detach(Element* e, u32 index) {
  if (!e) {
    return;
  }
  if (index < e->count && e->count > 0) {
    ui_detach_elements(&e->items[index]);
    memmove(&e->items[index], &e->items[index + 1], sizeof(Element) * ((e->count - index) - 1));
    e->count -= 1;
    return;
  }
  e->count = 0;
}

void ui_detach_last(Element* e) {
  ASSERT(e != NULL);
  if (e->count > 0) {
    ui_detach(e, e->count - 1);
  }
}

Element* ui_replace_element(Element* e, Element* new_element) {
  ASSERT(e != NULL);
  UI_state* ui = &ui_state;
  ui_detach_elements(e);
  *e = *new_element;
  if (e->id < 2) {
    e->id = ui->id_counter++;
  }
  return e;
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
  e.placement = PLACEMENT_BLOCK;

  e.render = true;
  e.scissor = true;
  e.title_bar = (Title_bar) {
    .title = title,
    .padding = UI_TITLE_BAR_PADDING,
    .top = true,
  };
  e.border_thickness = UI_BORDER_THICKNESS;
  if (title) {
    // NOTE(lucas): set roundness to 0 when using title bars because rounded titlebars are not supported yet
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

Element ui_text_ex(char* text, bool text_wrapping) {
  Element e = ui_text(text);
  e.data.text.text_wrapping = text_wrapping;
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
  e.background_color = lerp_color(UI_BACKGROUND_COLOR, UI_INTERPOLATION_COLOR, 0.2f);
  e.secondary_color = UI_BUTTON_COLOR;
  return e;
}

Element ui_slider_int(i32* value, i32 min_value, i32 max_value) {
  Element e = ui_slider(value, VALUE_TYPE_INTEGER, RANGE(min_value, max_value));
  return e;
}

Element ui_slider_float(f32* value, f32 min_value, f32 max_value) {
  Element e = ui_slider(value, VALUE_TYPE_FLOAT, RANGE_FLOAT(min_value, max_value));
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
  e.data.input.value_hash = 0;
  e.data.input.value = NULL;
  e.background = true;
  e.border = true;
  e.scissor = false;
  e.roundness = UI_BUTTON_ROUNDNESS;
  e.background_color = lerp_color(UI_BACKGROUND_COLOR, UI_INTERPOLATION_COLOR, 0.2f);
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
      MEMORY_TAG("ui.ui_input_ex2: buffer_new_from_fmt");
      e.data.input.buffer = buffer_new_from_fmt(32, "%g", *(f32*)value);
      break;
    }
    case VALUE_TYPE_INTEGER: {
      MEMORY_TAG("ui.ui_input_ex2: buffer_new_from_fmt");
      e.data.input.buffer = buffer_new_from_fmt(32, "%d", *(i32*)value);
      break;
    }
    default:
      break;
  }
  e.data.input.cursor = e.data.input.buffer.count;
  return e;
}

Element ui_input_int(char* preview, i32* value) {
  return ui_input_ex2(preview, value, INPUT_NUMBER, VALUE_TYPE_INTEGER);
}

Element ui_input_float(char* preview, f32* value) {
  return ui_input_ex2(preview, value, INPUT_NUMBER, VALUE_TYPE_FLOAT);
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
      stb_dprintf(fd, "\"%s\"\n", string);
      break;
    }
    case ELEMENT_TOGGLE: {
      stb_dprintf(fd, "%s\n", bool_str[*e->data.toggle.value == true]);
      break;
    }
    case ELEMENT_SLIDER: {
      stb_dprintf(fd, "{ ");
      if (e->data.slider.type == VALUE_TYPE_FLOAT) {
        stb_dprintf(fd, "value: %g, ", *e->data.slider.v.f);
        stb_dprintf(fd, "range: { %g, %g }", e->data.slider.range.f_min, e->data.slider.range.f_max);
      }
      else if (e->data.slider.type == VALUE_TYPE_INTEGER) {
        stb_dprintf(fd, "value: %d, ", *e->data.slider.v.i);
        stb_dprintf(fd, "range: { %d, %d }", e->data.slider.range.i_min, e->data.slider.range.i_max);
      }
      stb_dprintf(fd, " }\n");
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
    f32 advance = 0.0f;

    if (font.glyphs[glyph_index].advanceX == 0) {
      advance = ((f32)font.recs[glyph_index].width * scale_factor + spacing);
    }
    else {
      advance = ((f32)font.glyphs[glyph_index].advanceX * scale_factor + spacing);
    }

    if (x_offset + advance >= max_line_width && max_line_width > 0) {
      if (allow_overflow) {
        return false;
      }
      else {
        x_offset = 0.0f;
        y_offset += line_spacing;
      }
    }
    if (codepoint == '\n') {
      x_offset = 0.0f;
      y_offset += line_spacing;
    }
    else {
      x_offset += advance;
    }
    if (x_offset_largest < x_offset) {
      x_offset_largest = x_offset;
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
    bool allow_overflow,
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
  i32 x = 0;
  i32 y = 0;

  f32 scale_factor = font_size / (f32)font.baseSize;

  i32 max_line_width = 0;
  if (text_wrapping || allow_overflow) {
    max_line_width = box->w;
  }

  for (size_t i = 0; i < length;) {
    i32 codepoint_size = 0;
    i32 codepoint = GetCodepointNext(&text[i], &codepoint_size);
    i32 glyph_index = GetGlyphIndex(font, codepoint);
    f32 advance = 0.0f;

    if (font.glyphs[glyph_index].advanceX == 0) {
      advance = ((f32)font.recs[glyph_index].width * scale_factor + spacing);
    }
    else {
      advance = ((f32)font.glyphs[glyph_index].advanceX * scale_factor + spacing);
    }

    if (((x_offset + advance) > max_line_width) && max_line_width > 0) {
      if (allow_overflow) {
        return;
      }
      else {
        x_offset = 0.0f;
        y_offset += line_spacing;
      }
    }
    if (codepoint == '\n') {
      y_offset += line_spacing;
      x_offset = 0.0f;
    }
    else {
      x = box->x + x_offset;
      y = box->y + y_offset;
      if (!ui_overlap(x, y, *box)) {
        break;
      }
      if ((codepoint != ' ') && (codepoint != '\t')) {
        DrawTextCodepoint(font, codepoint, (Vector2) { x, y }, font_size, tint);
      }

      x_offset += advance;
    }
    i += codepoint_size;
  }
}

void ui_update_input(UI_state* ui, Element* e) {
  if (e->readonly) {
    return;
  }
  Buffer* buffer = &e->data.input.buffer;
  char ch = 0;
  bool mod_key = IsKeyDown(KEY_LEFT_CONTROL);
  i32 keycode = GetLastSoftKeyCode();
  for (;;) {
#ifdef TARGET_ANDROID
    ch = GetLastSoftKeyChar();
    if (ch >= ' ' && ch <= '~') {
      ClearLastSoftKey();
    }
#else
    ch = GetCharPressed();
#endif
    const char* clipboard = NULL;
    bool paste = mod_key && IsKeyPressed(KEY_V);
    size_t num_keys_pressed = 1;
    if (paste) {
      clipboard = GetClipboardText();
      if (!clipboard) {
        break;
      }
      num_keys_pressed = strlen(clipboard);
      ch = clipboard[0];
    }
    if (ch == 0) {
      break;
    }

    ui->blink_timer = 0;
    for (size_t i = 0; i < num_keys_pressed; ++i) {
      if (paste) {
        if (!clipboard) {
          break;
        }
        if (clipboard[i] == 0) {
          break;
        }
        ch = clipboard[i];
      }
      switch (e->data.input.input_type) {
        case INPUT_TEXT: {
          buffer_insert(buffer, ch, e->data.input.cursor);
          e->data.input.cursor += 1;
          break;
        }
        case INPUT_NUMBER: {
          if ((ch >= '0' && ch <= '9') || ch == '.' || ch == '-') {
            buffer_insert(buffer, ch, e->data.input.cursor);
            e->data.input.cursor += 1;
          }
          break;
        }
        default:
          break;
      }
    }

    e->oninput(e, ch);
    if (paste) {
      break;
    }
#ifdef TARGET_ANDROID
    break;
#endif
  }
  if (keycode != 0) {
    ClearLastSoftKey();
  }
  if (mod_key && KEY_PRESSED(KEY_C) && buffer->data) {
    SetClipboardText((const char*)buffer->data);
  }
  if (KEY_PRESSED(KEY_BACKSPACE) || keycode == 67) {
    ui->blink_timer = 0;
    if (e->data.input.cursor > 0) {
      buffer_erase(buffer, e->data.input.cursor - 1);
      e->data.input.cursor -= 1;
    }
  }
  if (KEY_PRESSED(KEY_DELETE)) {
    ui->blink_timer = 0;
    if (buffer->count > 0 && e->data.input.cursor < buffer->count) {
      buffer_erase(buffer, e->data.input.cursor);
    }
  }
  if (KEY_PRESSED(KEY_LEFT)) {
    ui->blink_timer = 0;
    if (e->data.input.cursor > 0) {
      e->data.input.cursor -= 1;
    }
  }
  if (KEY_PRESSED(KEY_RIGHT)) {
    ui->blink_timer = 0;
    e->data.input.cursor += 1;
    if (e->data.input.cursor > buffer->count) {
      e->data.input.cursor = buffer->count;
    }
  }
  if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || keycode == 66) {
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
    HideSoftKeyboard();
  }
}

void ui_render_input(UI_state* ui, Element* e) {
  if (e->readonly) {
    return;
  }
  const i32 cursor = e->data.input.cursor;
  ui_render_rectangle_lines(e->box, e->border_thickness, e->roundness, UI_FOCUS_COLOR);
  Box cursor_box = BOX(
    e->box.x + cursor * FONT_SIZE/2,
    e->box.y,
    1,
    e->box.h
  );
  cursor_box = ui_pad_box_ex(cursor_box, 0, e->box.h*0.5f - FONT_SIZE*0.5f);
  cursor_box.w = 1;
  const f32 blink = cosf(ui->blink_timer * 5);
  if (!ui_overlap(cursor_box.x, cursor_box.y, e->box)) {
    return;
  }
  if (blink > 0) {
    ui_render_rectangle(cursor_box, 0, UI_TEXT_COLOR);
  }
}
