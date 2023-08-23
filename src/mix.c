// mix.c

#include <raylib.h>

#define STB_SPRINTF_IMPLEMENTATION
#define USE_STB_SPRINTF
#include "ext/stb_sprintf.h"

#define COMMON_IMPLEMENTATION
// #define NO_STDLIB
#define NO_STDIO
#include "common.h"
#include "mix.h"
#define ARENA_IMPLEMENTATION
#include "arena.h"

#include "config.c"
#include "log.c"
#define MEMORY_ALLOC_STATIC
#include "memory.c"
#include "entity.c"

Result mix_init(Mix* m);
void mix_reset(Mix* m);
void mix_update(Mix* m);
void mix_free(Mix* m);

i32 mix_main(i32 argc, char** argv) {
  (void)argc;
  (void)argv;

  Mix mix;
  if (mix_init(&mix) != Ok) {
    return EXIT_FAILURE;
  }

  SetTraceLogLevel(LOG_WARNING);
  ConfigFlags config_flags = 0;
  if (WINDOW_RESIZABLE) {
    config_flags |= FLAG_WINDOW_RESIZABLE;
  }
  if (MSAA_4X) {
    config_flags |= FLAG_MSAA_4X_HINT;
  }
  SetConfigFlags(config_flags);

  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "mix");
  SetTargetFPS(TARGET_FPS);
  SetExitKey(KEY_NULL);

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(COLOR_BG);
    mix_update(&mix);
    EndDrawing();
  }
  CloseWindow();
  mix_free(&mix);
  return EXIT_SUCCESS;
}

void mix_update(Mix* m) {
  m->hover = NULL;
  if (IsKeyPressed(KEY_R)) {
    mix_reset(m);
  }
  m->mouse = GetMousePosition();
  entities_update(m);

  if (IsKeyPressed(KEY_X)) {
    if (m->hover) {
      entity_delete(m, m->hover);
    }
  }
  if (IsKeyPressed(KEY_M)) {
    if (m->hover) {
      if (m->hover == m->select) {
        m->select = NULL; // de-select if toggling active state
      }
      if (m->hover->state == STATE_INACTIVE) {
        m->hover->state = STATE_ACTIVE;
      }
      else {
        m->hover->state = STATE_INACTIVE;
      }
    }
  }
  if (IsKeyPressed(KEY_G)) {
    Entity* e = m->select;
    if (e) {
      if (m->grab) {
        m->grab = false;
        m->select = NULL;
      }
      else {
        m->grab_offset = (Vector2) {
          e->x - m->mouse.x,
          e->y - m->mouse.y
        };
        m->grab = true;
      }
    }
  }
  if (m->grab && m->select) {
    Entity* e = m->select;
    e->x = m->grab_offset.x + m->mouse.x;
    e->y = m->grab_offset.y + m->mouse.y;
  }

  entities_render(m);
}

Result mix_init(Mix* m) {
  log_init(is_terminal(STDOUT_FILENO) && is_terminal(STDERR_FILENO));
  memory_init();
  mix_reset(m);
  return Ok;
}

void mix_reset(Mix* m) {
  entities_init(m);
  m->id = 0;
  m->select = NULL;
  m->hover = NULL;
  m->mouse = (Vector2) {0, 0};
  m->grab_offset = (Vector2) {0, 0};
  m->grab = false;
}

void mix_free(Mix* m) {
  (void)m;
}
