// mix.c

#define STB_SPRINTF_IMPLEMENTATION
#define USE_STB_SPRINTF
#include "ext/stb_sprintf.h"

#define COMMON_IMPLEMENTATION
#include "common.h"
#include "mix.h"
#define ARENA_IMPLEMENTATION
#include "arena.h"

#include "config.c"
#include "log.c"
#include "memory.c"
#include "entity.c"
#include "module.c"
#include "audio.c"

static Color COLOR_BG = (Color) { .r = 25, .g = 25, .b = 32, .a = 255, };

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
    TIMER_START();
    BeginDrawing();
    ClearBackground(COLOR_BG);
    mix_update(&mix);
    EndDrawing();
    mix.dt = TIMER_END();
    if (mix.dt < DT_MIN) {
      mix.dt = DT_MIN;
    }
    mix.fps = 1.0f / mix.dt;
  }

  CloseWindow();
  mix_free(&mix);
  return EXIT_SUCCESS;
}

void mix_update(Mix* m) {

  if (IsKeyPressed(KEY_R)) {
    mix_reset(m);
  }
  m->mouse = GetMousePosition();
RLAPI void DrawText(const char *text, int posX, int posY, int fontSize, Color color);       // Draw text (using default font)

{
  char text[128] = {0};
  snprintf(text, sizeof(text), "mouse: %d, %d\nfps: %g", (i32)m->mouse.x, (i32)m->mouse.y, m->fps);
  DrawText(text, 4, 4, 10, COLOR_RGB(255, 255, 255));
}
#if 0
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
  if (IsMouseButtonPressed(1)) {
    Entity* e = m->hover;
    if (e) {
      if (e->state == STATE_ACTIVE) {
        m->select = e;
        m->grab = true;
        m->grab_offset = (Vector2) {
          e->x - m->mouse.x,
          e->y - m->mouse.y
        };
      }
    }
  }
  if (IsMouseButtonReleased(1)) {
    Entity* e = m->select;
    if (e && m->grab) {
      m->grab = false;
      m->select = NULL;
    }
  }
  if (m->grab && m->select) {
    Entity* e = m->select;
    e->x = m->grab_offset.x + m->mouse.x;
    e->y = m->grab_offset.y + m->mouse.y;
  }

  entities_render(m);
#endif
}

Result mix_init(Mix* m) {
  log_init(is_terminal(STDOUT_FILENO) && is_terminal(STDERR_FILENO));
  memory_init();
  config_init();
  mix_reset(m);
  audio_engine = audio_engine_new(SAMPLE_RATE, FRAMES_PER_BUFFER);
  if (audio_engine_start(&audio_engine) != Ok) {
    log_print(STDERR_FILENO, LOG_TAG_WARN, "failed to initialize audio engine\n");
  }
  return Ok;
}

void mix_reset(Mix* m) {
  m->mouse = (Vector2) {0, 0};
  m->grab_offset = (Vector2) {0, 0};
  m->grab = false;
  m->fps = 0;
  m->dt = DT_MIN;
}

void mix_free(Mix* m) {
  (void)m;
  audio_engine_exit(&audio_engine);
}
