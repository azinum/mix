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
#include "misc.c"
#include "log.c"
#include "memory.c"
#include "entity.c"
#include "module.c"
#include "audio.c"
#include "wave_shaper.c"

#define CONFIG_PATH "data/default.cfg"

static Color COLOR_BG = (Color) { .r = 25, .g = 25, .b = 32, .a = 255, };

Mix mix = {0};

Result mix_init(Mix* m);
void mix_reset(Mix* m);
void mix_update(Mix* m);
void mix_free(Mix* m);

i32 mix_main(i32 argc, char** argv) {
  (void)argc;
  (void)argv;

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
  config_store(CONFIG_PATH);
  return EXIT_SUCCESS;
}

void mix_update(Mix* m) {
  if (IsKeyPressed(KEY_R)) {
    mix_reset(m);
  }
  m->mouse = GetMousePosition();

  waveshaper_update(m, &m->waveshaper);
  waveshaper_render(m, &m->waveshaper);

{
  char text[512] = {0};
  snprintf(text, sizeof(text), "mouse: %d, %d\nfps: %g", (i32)m->mouse.x, (i32)m->mouse.y, m->fps);
  DrawText(text, 4, 4, FONT_SIZE_SMALLEST, COLOR_RGB(255, 255, 255));
}
}

Result mix_init(Mix* m) {
  log_init(is_terminal(STDOUT_FILENO) && is_terminal(STDERR_FILENO));
  memory_init();
  config_init();
  mix_reset(m);
  audio_engine = audio_engine_new(SAMPLE_RATE, FRAMES_PER_BUFFER, CHANNEL_COUNT);
  m->waveshaper = waveshaper_new(audio_engine.frames_per_buffer * audio_engine.channel_count);
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
  waveshaper_free(&m->waveshaper);
}
