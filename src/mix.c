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

static Color COLOR_BG = (Color) { .r = 35, .g = 35, .b = 42, .a = 255, };

Mix mix = {0};
Assets assets = {0};

Result mix_init(Mix* m);
void mix_reset(Mix* m);
void mix_update_and_render(Mix* m);
void mix_free(Mix* m);
void mix_ui_init(Mix* m);
void assets_load(Assets* a);
void assets_unload(Assets* a);

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
  SetTraceLogLevel(LOG_WARNING);

  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "mix");
  SetTargetFPS(TARGET_FPS);
  SetExitKey(KEY_NULL);

  assets_load(&assets);

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
    mix.fps = 1.0f / mix.dt;
  }
  // config_store(CONFIG_PATH);
  config_free();
  mix_free(&mix);
  CloseWindow();
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
  static char title[512] = {0};
  stb_snprintf(title, sizeof(title), "%zu/%zu allocs/deallocs | %zu/%zu bytes (%.2g %%) | %g ms ui latency | %d fps | %g dt", memory_state.num_allocs, memory_state.num_deallocs, memory_state.usage, memory_state.max_usage, 100 * ((f32)memory_state.usage / memory_state.max_usage), 1000 * ui_state.latency, (i32)m->fps, 1000 * m->dt);
  SetWindowTitle(title);
}

static Element* grid = NULL;

void on_click(Element* e, void* userdata) {
  (void)userdata;
  Element button = ui_button("button");
  button.onclick = on_click;
  button.background = true;
  button.scissor = true;
  button.background_color = e->background_color;
  ui_attach_element(grid, &button);
}

void on_increment_font_size(Element* e, void* userdata) {
  (void)e;
  (void)userdata;
  FONT_SIZE_SMALL += 1;
}

void on_decrement_font_size(Element* e, void* userdata) {
  (void)e;
  (void)userdata;
  FONT_SIZE_SMALL -= 1;
}

void on_increment_line_spacing(Element* e, void* userdata) {
  (void)e;
  (void)userdata;
  UI_LINE_SPACING += 1;
  SetTextLineSpacing(UI_LINE_SPACING);
}

void on_decrement_line_spacing(Element* e, void* userdata) {
  (void)e;
  (void)userdata;
  UI_LINE_SPACING -= 1;
  SetTextLineSpacing(UI_LINE_SPACING);
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
}

void mix_free(Mix* m) {
  (void)m;
  audio_engine_exit(&audio_engine);
  ui_free();
  assets_unload(&assets);
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
#if 0
  u32 cols = 4;
  {
    Element grid_element = ui_grid(cols, true);
    grid = ui_attach_element(NULL, &grid_element);
  }
  for (size_t i = 0; i < cols; ++i){
    char* text = "spawn new button";
    if (i+1 == cols) {
      text = "here are\na\nfew\nlines.";
    }
    Element e = ui_button(text);
    e.onclick = on_click;
    e.background = true;
    e.scissor = true;
    e.background_color = colors[5];
    ui_attach_element(grid, &e);
  }

  Element grid_element = ui_grid(2, true);
  grid_element.border = false;
  Element* grid2 = ui_attach_element(grid, &grid_element);

  {
    Element e = ui_button("font+");
    e.onclick = on_increment_font_size;
    e.background = true;
    e.scissor = true;
    e.background_color = colors[0];
    ui_attach_element(grid2, &e);
  }
  {
    Element e = ui_button("font-");
    e.onclick = on_decrement_font_size;
    e.background = true;
    e.scissor = true;
    e.background_color = colors[0];
    ui_attach_element(grid2, &e);
  }
  {
    Element e = ui_button("line spacing+");
    e.onclick = on_increment_line_spacing;
    e.background = true;
    e.scissor = true;
    e.background_color = colors[1];
    ui_attach_element(grid2, &e);
  }
  {
    Element e = ui_button("line spacing-");
    e.onclick = on_decrement_line_spacing;
    e.background = true;
    e.scissor = true;
    e.background_color = colors[1];
    ui_attach_element(grid2, &e);
  }
#else
  Element* container = NULL;
  {
    Element e = ui_container(true);
    e.border = true;
    e.placement = PLACEMENT_ROWS;
    container = ui_attach_element(NULL, &e);
  }
  for (size_t i = 0; i < 32; ++i) {
    Element e = ui_button("test");
    e.scissor = false;
    e.box = BOX(0, 0, 32 + random_number() % 128, 32 + random_number() % 128);
    e.background = true;
    e.background_color = colors[random_number() % LENGTH(colors)];
    ui_attach_element(container, &e);
  }
#endif
}

void assets_load(Assets* a) {
  a->font = LoadFontEx(UI_FONT, UI_FONT_BASE_SIZE, NULL, 0);
  SetTextLineSpacing(UI_LINE_SPACING);
}

void assets_unload(Assets* a) {
  UnloadFont(a->font);
}
