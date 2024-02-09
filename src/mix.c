// mix.c

#define STB_SPRINTF_IMPLEMENTATION
#define USE_STB_SPRINTF
#include "ext/stb/stb_sprintf.h"

#define COMMON_IMPLEMENTATION
#include "common.h"
#include "mix.h"
#define ARENA_IMPLEMENTATION
#include "arena.h"

#ifdef TARGET_ANDROID
  #include "helper.c"
  #include "features.c"
#endif

#include "ext/lua/luaone.c"

#include "platform.c"
#include "memory.c"
#include "thread.c"
#include "glob.c"
#include "hash.c"
#include "random.c"
#include "buffer.c"
#include "config.c"
#include "misc.c"
#include "colors.c"
#include "log.c"
#include "module.c"
#include "ui.c"
#include "settings.c"
// instruments
#include "instrument.c"
#include "wave_shaper.c"
#include "dummy.c"
// effects/filters
#include "effect.c"
#include "fx_distortion.c"

#include "instrument_picker.c"
#include "control_panel.c"
#include "effect_chain.c"
#include "effect_picker.c"
#include "audio.c"

#ifdef TEST_UI
  #include "test_ui.c"
#endif

static f32 delta_buffer[128] = {0};

Mix mix_state = {0};
Assets assets = {0};

Result mix_init(Mix* mix);
void mix_reset(Mix* mix);
void mix_update_and_render(Mix* mix);
void mix_free(Mix* mix);
void mix_ui_new(Mix* mix);
void render_delta_buffer(Mix* mix);
void assets_load(Assets* a);
void assets_unload(Assets* a);

i32 mix_main(i32 argc, char** argv) {
  (void)argc;
  (void)argv;

  TIMER_START();

  Mix* mix = &mix_state;

  if (mix_init(mix) != Ok) {
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

  mix_ui_new(mix);

  while (!WindowShouldClose()) {
    TIMER_START();
    BeginDrawing();
    ClearBackground(MAIN_BACKGROUND_COLOR);
    mix_update_and_render(mix);
    EndDrawing();
    mix->dt = TIMER_END();
    if (mix->dt < DT_MIN) {
      mix->dt = DT_MIN;
    }
    if (mix->dt > DT_MAX) {
      mix->dt = DT_MAX;
    }
    f32 timestamp = mix->timer_start + ((60.0f / mix->bpm) / SUBTICKS);
    if (mix->timer >= timestamp) {
      f32 delta = mix->timer - timestamp;
      mix->timer_start = mix->timer - delta;
      mix->timed_tick += 1;
    }
    mix->fps = 1.0f / mix->dt;
    if (!mix->paused) {
      mix->timer += mix->dt;
      mix->tick += 1;
    }
  }
  config_free();
  mix_free(mix);
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

void mix_assets_load(void) {
  assets_load(&assets);
}

void mix_assets_unload(void) {
  assets_unload(&assets);
}

void mix_update_and_render(Mix* mix) {
  Audio_engine* audio = &audio_engine;

  mix->mouse = GetMousePosition();
  delta_buffer[mix->tick % LENGTH(delta_buffer)] = mix->dt;

  if (audio->restart) {
    mix_restart_audio_engine();
    ui_free();
    ui_init();
    mix_ui_new(mix);
  }

  if (!ui_input_interacting()) {
    if (IsKeyPressed(KEY_R)) {
      if (IsKeyDown(KEY_LEFT_CONTROL)) {
        mix_restart_audio_engine();
      }
      ui_free();
      ui_init();
      mix_reset(mix);
      mix_ui_new(mix);
    }
    if (IsKeyPressed(KEY_SPACE)) {
      mix->paused = !mix->paused;
    }
    if (IsKeyPressed(KEY_L)) {
      config_load(CONFIG_PATH);
    }
    if (IsKeyPressed(KEY_M)) {
      memory_print_info(STDOUT_FILENO);
    }
  }
  instrument_update(&audio->instrument, mix);

  ui_update(mix->dt);
  ui_render();

#if 1
  static char debug_text[256] = {0};
  stb_snprintf(
    debug_text,
    sizeof(debug_text),
    "%zu/%zu bytes (%.2g %%)\n"
    "%g ms ui latency\n"
    "%u/%u ui element updates"
    ,
    memory_state.usage, memory_state.max_usage,
    100 * ((f32)memory_state.usage / memory_state.max_usage),
    1000 * ui_state.latency,
    ui_state.element_update_count,
    ui_state.element_count
  );
  SetTextLineSpacing(FONT_SIZE_SMALLEST);
  DrawText(debug_text, 32, GetScreenHeight() - (FONT_SIZE_SMALLEST) * 3 - 16, FONT_SIZE_SMALLEST, COLOR_RGB(0xfc, 0xeb, 0x2f));
  SetTextLineSpacing(UI_LINE_SPACING);
  render_delta_buffer(mix);
#endif
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

Result mix_init(Mix* mix) {
  log_init(is_terminal(STDOUT_FILENO) && is_terminal(STDERR_FILENO));
  memory_init();
  random_init(time(0));
  config_init();
  if (config_load(CONFIG_PATH) != Ok) {
    log_print(STDERR_FILENO, LOG_TAG_WARN, "config file `%s` does not exist, creating new with default settings\n", CONFIG_PATH);
    config_store(CONFIG_PATH);
  }
  mix_reset(mix);
  mix->ins_container = NULL;
  mix->effect_chain = NULL;
  ui_init();
  if (audio_engine_start_new(&audio_engine) != Ok) {
    log_print(STDERR_FILENO, LOG_TAG_WARN, "failed to initialize audio engine\n");
  }
  return Ok;
}

void mix_reset(Mix* mix) {
  mix->mouse = (Vector2) {0, 0};
  mix->fps = 0;
  mix->dt = DT_MIN;
  mix->tick = 0;
  mix->timed_tick = 0;
  mix->bpm = BPM;
  mix->timer = 0.0f;
  mix->timer_start = 0.0f;
  mix->paused = false;
}

void mix_free(Mix* m) {
  (void)m;
  audio_engine_exit(&audio_engine);
  ui_free();
  assets_unload(&assets);
  CloseWindow();
}

void mix_ui_new(Mix* mix) {
#ifdef TEST_UI
  {
    Element e = test_ui_new();
    ui_attach_element(NULL, &e);
  }
#else
  Audio_engine* audio = &audio_engine;
  Element* container = NULL;
  {
    Element e = ui_container(NULL);
    e.scissor = false;
    e.border = false;
    e.background = false;
    e.placement = PLACEMENT_BLOCK;
    e.padding = UI_PADDING / 2;
    container = ui_attach_element(NULL, &e);
  }
  {
    Element e = ui_container(NULL);
    e.sizing = SIZING_PERCENT(100, 12);
    e.scissor = true;
    e.border = true;
    e.placement = PLACEMENT_BLOCK;
    e.background = true;
    Element* control_panel = ui_attach_element(container, &e);
    control_panel_ui_new(mix, control_panel);
  }
  {
    Element e = ui_container("instrument");
    e.sizing = SIZING_PERCENT(70, 88);
    e.border = true;
    e.scissor = true;
    e.placement = PLACEMENT_BLOCK;
    e.background = true;
    mix->ins_container = ui_attach_element(container, &e);
    if (audio->instrument.initialized) {
      instrument_ui_new(&audio->instrument, mix->ins_container);
    }
  }
  {
    Element e = instrument_picker_ui_new(mix);
    e.sizing = SIZING_PERCENT(30, 88);
    ui_attach_element(container, &e);
  }
  {
    Element e = ui_container("effect chain");
    e.sizing = SIZING_PERCENT(70, 100);
    e.border = true;
    e.scissor = true;
    e.placement = PLACEMENT_BLOCK;
    e.background = true;
#ifdef TARGET_ANDROID
    e.padding = UI_PADDING * 4;
#endif
    mix->effect_chain = ui_attach_element(container, &e);
    effect_chain_ui_new(mix, mix->effect_chain);
  }
  {
    Element e = ui_container("effect picker");
    e.sizing = SIZING_PERCENT(30, 100);
    e.border = true;
    e.scissor = true;
    e.placement = PLACEMENT_BLOCK;
    e.background = true;
    Element* effect_picker = ui_attach_element(container, &e);
    effect_picker_ui_new(mix, effect_picker);
  }
#endif
}

void render_delta_buffer(Mix* mix) {
  i32 w = LENGTH(delta_buffer);
  i32 h = 38;
  i32 x = GetScreenWidth() - w - 32;
  i32 y = GetScreenHeight() - h - 16;
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
    if (i == (mix->tick % LENGTH(delta_buffer))) {
      color = COLOR(255, 255, 255, 150);
      DrawLine(x + i, y+h, x + i, y, color);
    }
  }
  DrawRectangleLines(x, y, w, h, COLOR(255, 255, 255, 100));

  dt_avg /= window_size;
  static char text[32] = {0};
  stb_snprintf(text, sizeof(text), "%g ms (average)\n%d fps", dt_avg * 1000, (i32)mix->fps);
  SetTextLineSpacing(FONT_SIZE_SMALLEST);
  DrawText(text, x, y - (FONT_SIZE_SMALLEST) * 2, FONT_SIZE_SMALLEST, COLOR_RGB(0xfc, 0xeb, 0x2f));
  SetTextLineSpacing(UI_LINE_SPACING);
}

void assets_load(Assets* assets) {
  assets->font = LoadFontEx(UI_FONT, UI_FONT_BASE_SIZE, NULL, 0);
  SetTextLineSpacing(UI_LINE_SPACING);
}

void assets_unload(Assets* assets) {
  UnloadFont(assets->font);
}
