// mix.c

#define STB_SPRINTF_IMPLEMENTATION
#define USE_STB_SPRINTF
#include "ext/stb/stb_sprintf.h"

#define COMMON_IMPLEMENTATION
#include "common.h"
#include "mix.h"
#define ARENA_IMPLEMENTATION
#include "arena.h"

#include "ext/lua/luaone.c"

#include "hash.c"
#include "random.c"
#include "buffer.c"
#include "config.c"
#include "misc.c"
#include "log.c"
#include "memory.c"
#include "entity.c"
#include "module.c"
#include "wave_shaper.c"
#include "audio.c"
#include "ui.c"

#define CONFIG_PATH "data/init.lua"

static f32 delta_buffer[256] = {0};

static Color COLOR_BG = (Color) { .r = 35, .g = 35, .b = 42, .a = 255, };

Mix mix = {0};
Assets assets = {0};

Result mix_init(Mix* m);
void mix_reset(Mix* m);
void mix_update_and_render(Mix* m);
void mix_render_delta_buffer(Mix* m);
void mix_free(Mix* m);
void mix_ui_init(Mix* m);
void assets_load(Assets* a);
void assets_unload(Assets* a);

i32 mix_main(i32 argc, char** argv) {
  (void)argc;
  (void)argv;

  TIMER_START();

  if (mix_init(&mix) != Ok) {
    return EXIT_FAILURE;
  }

  ConfigFlags config_flags = 0;
  if (WINDOW_RESIZABLE) {
    config_flags |= FLAG_WINDOW_RESIZABLE;
  }
  if (WINDOW_FULLSCREEN) {
    config_flags |= FLAG_FULLSCREEN_MODE;
  }
  if (VSYNC) {
    config_flags |= FLAG_VSYNC_HINT;
  }
  if (MSAA_4X) {
    config_flags |= FLAG_MSAA_4X_HINT;
  }
  SetConfigFlags(config_flags);
  SetTraceLogLevel(LOG_WARNING);

  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "mix");
  SetTargetFPS(TARGET_FPS);
  SetExitKey(KEY_NULL);

  assets_load(&assets);

  f32 dt = TIMER_END();
  log_print(STDOUT_FILENO, LOG_TAG_INFO, "startup time was %g ms\n", 1000 * dt);

  while (!WindowShouldClose()) {
    TIMER_START();
    BeginDrawing();
    ClearBackground(COLOR_BG);
    mix_update_and_render(&mix);
    EndDrawing();
    mix.dt = TIMER_END();
    if (mix.dt < DT_MIN) {
      mix.dt = DT_MIN;
    }
    if (mix.dt > DT_MAX) {
      mix.dt = DT_MAX;
    }
    mix.fps = 1.0f / mix.dt;
    mix.tick += 1;
  }
  // config_store(CONFIG_PATH);
  config_free();
  mix_free(&mix);
  return EXIT_SUCCESS;
}

void mix_update_and_render(Mix* m) {
  Audio_engine* e = &audio_engine;

  m->mouse = GetMousePosition();

  if (IsKeyPressed(KEY_R)) {
    ui_free();
    ui_init();
    mix_ui_init(m);
    mix_reset(m);
  }
  if (IsKeyPressed(KEY_L)) {
    config_load(CONFIG_PATH);
  }

  ui_update();
  ui_render();

#if 0
  waveshaper_update(m, &e->waveshaper);
  waveshaper_render(m, &e->waveshaper);
#endif

  static char debug_text[512] = {0};
  stb_snprintf(
    debug_text,
    sizeof(debug_text),
    "%d fps\n"
    "%zu/%zu bytes (%.2g %%)\n"
    "%g ms ui latency\n"
    "%u ui element updates"
    ,
    (i32)m->fps,
    memory_state.usage, memory_state.max_usage,
    100 * ((f32)memory_state.usage / memory_state.max_usage),
    1000 * ui_state.latency,
    ui_state.element_update_count
  );

  DrawText(debug_text, 4, GetScreenHeight() - (0.5*UI_LINE_SPACING + FONT_SIZE_SMALLEST) * 4, FONT_SIZE_SMALLEST, COLOR_RGB(0xfc, 0xeb, 0x2f));

  delta_buffer[m->tick % LENGTH(delta_buffer)] = m->dt;
  mix_render_delta_buffer(m);
}

void mix_render_delta_buffer(Mix* m) {
  i32 w = LENGTH(delta_buffer);
  i32 h = 48;
  i32 x = GetScreenWidth() - w - 8;
  i32 y = GetScreenHeight() - h - 8;
  f32 dt_avg = 0.0f;
  i32 window_size = 0;
  f32 sample = 0;
  f32 prev_sample = 0;
  for (size_t i = 0; i < LENGTH(delta_buffer); ++i) {
    prev_sample = sample;
    sample = delta_buffer[i];
    if (sample <= 0) {
      continue;
    }
    window_size += 1;
    dt_avg += sample;

    f32 f = sample / (DT_MAX * 1000);
    Color color = lerpcolor(COLOR_RGB(50, 255, 50), COLOR_RGB(255, 50, 50), f);
    if (sample > prev_sample) {
      f32 delta = sample / prev_sample;
      if (delta > 1.3f) { // 30% increase from the previous sample
        color = COLOR_RGB(255, 50, 50);
      }
    }

    if (i == (m->tick % LENGTH(delta_buffer))) {
      color = COLOR_RGB(255, 255, 255);
      DrawLine(x + i, y+h, x + i, y, color);
      continue;
    }
    DrawLine(x + i, y+h, x + i, (y + h) - h*(sample / DT_MAX), color);
  }
  DrawRectangleLines(x, y, w, h, COLOR(255, 255, 255, 100));

  dt_avg /= window_size;
  static char text[32] = {0};
  stb_snprintf(text, sizeof(text), "%g ms dt (average)", dt_avg * 1000);
  DrawText(text, x, y - FONT_SIZE_SMALLEST, FONT_SIZE_SMALLEST, COLOR_RGB(0xfc, 0xeb, 0x2f));
}

void onclick_test(Element* e) {
  stb_printf(
    "Element {\n"
    "  id: %u\n"
    "  box: {%d, %d, %d, %d}\n"
    "}\n"
    ,
    e->id,
    e->box.x,
    e->box.y,
    e->box.w,
    e->box.h
  );
}

Result mix_init(Mix* m) {
  log_init(is_terminal(STDOUT_FILENO) && is_terminal(STDERR_FILENO));
  memory_init();
  random_init(time(0));
  config_init();
  if (config_load(CONFIG_PATH) != Ok) {
    log_print(STDERR_FILENO, LOG_TAG_WARN, "config file `%s` does not exist, creating new with default settings\n", CONFIG_PATH);
    config_store(CONFIG_PATH);
  }
  mix_reset(m);
  ui_init();
  mix_ui_init(m);
  audio_engine = audio_engine_new(SAMPLE_RATE, FRAMES_PER_BUFFER, CHANNEL_COUNT);
  if (audio_engine_start(&audio_engine) != Ok) {
    log_print(STDERR_FILENO, LOG_TAG_WARN, "failed to initialize audio engine\n");
  }
  return Ok;
}

void mix_reset(Mix* m) {
  m->mouse = (Vector2) {0, 0};
  m->fps = 0;
  m->dt = DT_MIN;
  m->tick = 0;
}

void mix_free(Mix* m) {
  (void)m;
  audio_engine_exit(&audio_engine);
  ui_free();
  assets_unload(&assets);
  CloseWindow();
}

void mix_ui_init(Mix* m) {
  (void)m;
  Color colors[9] = {
    COLOR_RGB(233, 153, 204),
    COLOR_RGB(225, 153, 102),
    COLOR_RGB(133, 210, 96),

    COLOR_RGB(0, 102, 153),
    COLOR_RGB(0, 153, 255),
    COLOR_RGB(153, 102, 255),

    COLOR_RGB(56, 184, 123),
    COLOR_RGB(204, 0, 0),
    COLOR_RGB(51, 153, 102),
  };
  char* titles[] = {
    "title",
    "test",
    "settings",
    NULL,
  };
  u32 cols = 2;
  u32 rows = 2;
  Element* grid = NULL;
  {
    Element e = ui_grid(cols, true);
    grid = ui_attach_element(NULL, &e); // attach grid to root
    ASSERT(grid != NULL);
  }
  for (size_t i = 0; i < rows * cols; ++i) {
    Element* container = NULL;
    {
      Element e = ui_container(titles[i]);
      e.border = true;
      e.scissor = true;
      e.placement = PLACEMENT_BLOCK;
      if (i == 0) {
        e.placement = PLACEMENT_ROWS;
      }
      e.background = true;
      e.background_color = COLOR_RGB(75, 75, 95);
      container = ui_attach_element(grid, &e);
    }
    for (size_t n = 0; n < 8; ++n) {
      Element e = ui_button("test");
      e.scissor = false;
      e.box = BOX(0, 0, 64 + random_number() % 64, 32 + random_number() % 64);
      e.background = true;
      e.background_color = colors[random_number() % LENGTH(colors)];
      e.onclick = onclick_test;
      ui_attach_element(container, &e);
    }
  }
}

void assets_load(Assets* a) {
  a->font = LoadFontEx(UI_FONT, UI_FONT_BASE_SIZE, NULL, 0);
  SetTextLineSpacing(UI_LINE_SPACING);
}

void assets_unload(Assets* a) {
  UnloadFont(a->font);
}
