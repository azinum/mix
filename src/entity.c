// entity.c

static void entity_update(Mix* m, Entity* e);
static void entity_render(Mix* m, Entity* e);

void entity_init(Entity* e) {
  e->id     = 0;
  memset(&e->name[0], 0, sizeof(e->name));
  e->x      = 0;
  e->y      = 0;
  e->w      = 0;
  e->h      = 0;
  e->type   = ENTITY_NONE;
  e->state  = STATE_DEAD;
  e->render = false;
  e->hover  = false;
}

Entity* entity_alloc(struct Mix* m) {
  for (size_t i = 0; i < MAX_ENTITY; ++i) {
    Entity* e = &m->entities[i];
    if (e->state == STATE_DEAD) {
      entity_init(e);
      e->id = ++m->id;
      e->state = STATE_ACTIVE;
      return e;
    }
  }
  return NULL;
}

void entity_delete(struct Mix* m, Entity* e) {
  if (m->hover == e) {
    m->hover = NULL;
  }
  if (m->select == e) {
    m->select = NULL;
  }
  e->state = STATE_DEAD;
}

void entities_init(Mix* m) {
  for (size_t i = 0; i < MAX_ENTITY; ++i) {
    Entity* e = &m->entities[i];
    entity_init(e);
  }
}

void entities_update(Mix* m) {
  if (IsKeyPressed(KEY_E)) {
    Entity* e = entity_alloc(m);
    if (e) {
      strncpy(e->name, "module", MAX_ENTITY_NAME_LENGTH);
      i32 w = 256;
      i32 h = 128;
      e->x = m->mouse.x - w/2;
      e->y = m->mouse.y - h/2;
      e->w = w;
      e->h = h;
      e->type = ENTITY_MODULE;
      e->render = true;
    }
  }
  for (size_t i = 0; i < MAX_ENTITY; ++i) {
    Entity* e = &m->entities[i];
    continue_if(e->state == STATE_DEAD);
    entity_update(m, e);
    if (IsMouseButtonPressed(1)) {
      if (e == m->hover) {
        m->select = e;
      }
      if (m->hover == NULL) {
        m->select = NULL;
        m->grab = false;
      }
    }
  }
}

void entities_render(Mix* m) {
  for (size_t i = 0; i < MAX_ENTITY; ++i) {
    Entity* e = &m->entities[i];
    continue_if(e->state == STATE_DEAD);
    continue_if(!e->render);
    entity_render(m, e);
  }
}

void entity_update(Mix* m, Entity* e) {
  bool hover = OVERLAP(m->mouse.x, m->mouse.y, e->x, e->y, e->w, e->h);
  e->hover = hover;
  if (hover && m->hover == NULL) {
    m->hover = e;
  }
}

void entity_render(Mix* m, Entity* e) {
  const f32 roundness = 0.1f;
  const i32 thickness = 2;
  const i32 segments  = 4;
  Rectangle rect = (Rectangle) { e->x, e->y, e->w, e->h };

  Color color_active = COLOR_RGB(200, 200, 200);
  Color color_text = color_active;
  Color color_inactive = COLOR_RGB(130, 130, 130);
  Color color_bg = COLOR_RGB(40, 44, 60);

  if (e->hover && m->hover == e) {
    color_active = COLOR_RGB(250, 250, 250);
  }
  if (e == m->select) {
    color_active = COLOR_RGB(250, 100, 100);
  }

  DrawRectangleRounded(rect, roundness, segments, color_bg);
  if (e->state == STATE_ACTIVE) {
    DrawRectangleRoundedLines(rect, roundness, segments, thickness, color_active);
    DrawText(e->name, e->x + 4, e->y - FONT_SIZE_SMALLER, FONT_SIZE_SMALLER, color_active);
    return;
  }
  if (e->state == STATE_INACTIVE) {
    DrawRectangleRoundedLines(rect, roundness, segments, thickness, color_inactive);
    return;
  }
}
