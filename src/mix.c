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

#include "thread.c"
#include "hash.c"
#include "random.c"
#include "buffer.c"
#include "config.c"
#include "misc.c"
#include "log.c"
#include "memory.c"
#include "module.c"
#include "ui.c"
#include "instrument.c"
#include "wave_shaper.c"
#include "audio.c"

#define CONFIG_PATH "data/init.lua"

static f32 delta_buffer[128] = {0};

Mix mix = {0};
Assets assets = {0};

Result mix_init(Mix* m);
void mix_reset(Mix* m);
void mix_update_and_render(Mix* m);
void mix_free(Mix* m);
void mix_ui_init(Mix* m);
void render_delta_buffer(Mix* m);
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
    ClearBackground(MAIN_BACKGROUND_COLOR);
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
  config_free();
  mix_free(&mix);
  return EXIT_SUCCESS;
}

Result mix_restart_audio_engine(void) {
  Audio_engine* audio = &audio_engine;
  audio_engine_exit(audio);
  if (audio_engine_start_new(audio) != Ok) {
    log_print(STDERR_FILENO, LOG_TAG_WARN, "failed to initialize audio engine\n");
    return Error;
  }
  return Ok;
}

void mix_update_and_render(Mix* m) {
  Audio_engine* audio = &audio_engine;

  m->mouse = GetMousePosition();
  delta_buffer[m->tick % LENGTH(delta_buffer)] = m->dt;

  if (audio->restart) {
    ui_free();
    ui_init();
    mix_restart_audio_engine();
    mix_ui_init(m);
  }

  if (IsKeyPressed(KEY_R)) {
    ui_free();
    ui_init();
    if (IsKeyDown(KEY_LEFT_CONTROL)) {
      mix_restart_audio_engine();
    }
    mix_ui_init(m);
    mix_reset(m);
  }
  if (IsKeyPressed(KEY_L)) {
    config_load(CONFIG_PATH);
  }

  instrument_update(&audio->instrument, m);

  ui_update(m->dt);
  ui_render();

#if 1
  static char debug_text[256] = {0};
  stb_snprintf(
    debug_text,
    sizeof(debug_text),
    "%zu/%zu bytes (%.2g %%)\n"
    "%g ms ui latency\n"
    "%u ui element updates"
    ,
    memory_state.usage, memory_state.max_usage,
    100 * ((f32)memory_state.usage / memory_state.max_usage),
    1000 * ui_state.latency,
    ui_state.element_update_count
  );

  DrawText(debug_text, 4, GetScreenHeight() - (0.5 * UI_LINE_SPACING + FONT_SIZE_SMALLEST) * 3, FONT_SIZE_SMALLEST, COLOR_RGB(0xfc, 0xeb, 0x2f));
#endif
  render_delta_buffer(m);
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
  if (audio_engine_start_new(&audio_engine) != Ok) {
    log_print(STDERR_FILENO, LOG_TAG_WARN, "failed to initialize audio engine\n");
  }
  mix_ui_init(m);
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
  Audio_engine* audio = &audio_engine;
  Element* container = NULL;
  random_init(1234);

  {
    Element e = ui_container(NULL);
    e.scissor = false;
    e.border = false;
    e.background = false;
    e.placement = PLACEMENT_BLOCK;
    e.padding = 2;
    container = ui_attach_element(NULL, &e);
  }
  {
    Element e = instrument_ui_new(&audio->instrument);
    e.sizing = SIZING_PERCENT(70, 100);
    ui_attach_element(container, &e);
  }
  Element* settings = NULL;
  {
    Element e = ui_container("settings");
    e.sizing = SIZING_PERCENT(30, 100);
    e.border = true;
    e.scissor = true;
    e.placement = PLACEMENT_BLOCK;
    e.background = true;
    settings = ui_attach_element(container, &e);
  }
  {
    Element e = ui_text("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(settings, &e);
  }
  {
    Element e = ui_text("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Elementum facilisis leo vel fringilla est ullamcorper eget nulla facilisi. Amet nisl purus in mollis nunc sed. Aliquet eget sit amet tellus. Mi in nulla posuere sollicitudin aliquam ultrices sagittis orci. Adipiscing bibendum est ultricies integer quis auctor. Et malesuada fames ac turpis egestas sed tempus. Est ante in nibh mauris. Volutpat maecenas volutpat blandit aliquam etiam erat velit. Gravida rutrum quisque non tellus. Faucibus vitae aliquet nec ullamcorper sit amet. Eu ultrices vitae auctor eu. In tellus integer feugiat scelerisque varius morbi enim. In arcu cursus euismod quis. Auctor augue mauris augue neque gravida in fermentum et sollicitudin. Duis ultricies lacus sed turpis tincidunt id. Urna condimentum mattis pellentesque id nibh tortor id aliquet. Lacus laoreet non curabitur gravida arcu ac tortor dignissim. At volutpat diam ut venenatis tellus in metus.");
    e.sizing = SIZING_PERCENT(100, 0);
    ui_attach_element(settings, &e);
  }
  for (size_t i = 0; i < 64; ++i) {
    Element e = ui_button("test");
    e.box = BOX(0, 0, 44 + random_number() % 64, 32 + random_number() % 64);
    e.onclick = onclick_test;
    ui_attach_element(settings, &e);
  }
}

void render_delta_buffer(Mix* m) {
  i32 w = LENGTH(delta_buffer);
  i32 h = 38;
  i32 x = GetScreenWidth() - w - 8;
  i32 y = GetScreenHeight() - h - 8;
  f32 dt_avg = 0.0f;
  i32 window_size = 0;
  f32 sample = 0;
  f32 prev_sample = 0;
  Color green = COLOR_RGB(50, 255, 50);
  Color red = COLOR_RGB(255, 50, 50);
  sample = delta_buffer[0];
  for (size_t i = 0; i < LENGTH(delta_buffer); ++i) {
    prev_sample = sample;
    sample = delta_buffer[i];
    if (sample <= 0) {
      continue;
    }
    window_size += 1;
    dt_avg += sample;

    f32 f = sample / DT_MAX;
    Color color = lerp_color(green, red, f);
    if (sample > prev_sample) {
      f32 delta = sample / prev_sample;
      // 30% increase from the previous sample
      if (delta > 1.3f) {
        color = red;
      }
    }
    DrawLine(x + i, (y + h) - h*(prev_sample / DT_MAX), x + i + 1, (y + h) - h*(sample / DT_MAX), color);
    if (i == (m->tick % LENGTH(delta_buffer))) {
      color = COLOR(255, 255, 255, 150);
      DrawLine(x + i, y+h, x + i, y, color);
    }
  }
  DrawRectangleLines(x, y, w, h, COLOR(255, 255, 255, 100));

  dt_avg /= window_size;
  static char text[32] = {0};
  stb_snprintf(text, sizeof(text), "%g ms (average)\n%d fps", dt_avg * 1000, (i32)m->fps);
  DrawText(text, x, y - (0.5f*UI_LINE_SPACING + FONT_SIZE_SMALLEST) * 2, FONT_SIZE_SMALLEST, COLOR_RGB(0xfc, 0xeb, 0x2f));
}

void assets_load(Assets* a) {
  a->font = LoadFontEx(UI_FONT, UI_FONT_BASE_SIZE, NULL, 0);
  SetTextLineSpacing(UI_LINE_SPACING);
}

void assets_unload(Assets* a) {
  UnloadFont(a->font);
}
