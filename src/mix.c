// mix.c

#define STB_SPRINTF_IMPLEMENTATION
#define USE_STB_SPRINTF
#include "ext/stb/stb_sprintf.h"

#ifdef TARGET_ANDROID
  #include "helper.c"
  #include "features.c"
#endif

#define COMMON_IMPLEMENTATION
#include "common.h"
#include "mix.h"
#define ARENA_IMPLEMENTATION
#include "arena.h"

#define THREAD_IMPLEMENTATION
#include "thread.h"

#ifdef DEVELOPER

static bool show_debug_info = true;

#endif

#include "platform.c"
#include "memory.c"
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
#include "midi.c"
#include "midi_settings.c"
#include "keyboard.c"
// instruments
#include "instrument.c"
#include "wave_shaper.c"
#include "dummy.c"
#include "noise.c"
#include "audio_input.c"
#include "basic_poly_synth.c"
#include "tracker.c"
#include "physical.c"
// effects/filters
#include "effect.c"
#include "fx_clip_distortion.c"
#include "fx_filter.c"
#include "fx_delay.c"
#include "fx_smooth.c"
#include "fx_interpolator.c"
#include "fx_reverb.c"

#include "instrument_picker.c"
#include "control_panel.c"
#include "effect_chain.c"
#include "effect_picker.c"
#include "audio.c"
#include "ui_audio.c"

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
void render_delta_buffer(Mix* mix, bool update_text);
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
  SetTextLineSpacing(0);

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

    mix->dt = CLAMP(TIMER_END(), DT_MIN, DT_MAX);
    mix->bpm = CLAMP(mix->bpm, 1, 999);

    const f32 bps = 60.0f / mix->bpm;
    f32 timestamp = (bps / SUBTICKS);
    if (mix->timer >= timestamp && !mix->paused) {
      f32 delta = mix->timer - timestamp;
      mix->timed_tick += 1;
      mix->tick_delta = delta;
      mix->timer = delta;
    }

    mix->fps = 1.0f / mix->dt;
    if (!mix->paused) {
      mix->timer += mix->dt;
      mix->tick += 1;
    }
  }
  config_free();
  mix_free(mix);
  if (memory_state.usage != 0) {
    log_print(STDERR_FILENO, LOG_TAG_ERROR, "memory leak (%zu Kb)\n", memory_state.usage / Kb(1));
  }
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

void mix_send_midi_event(Midi_event event) {
  Mix* mix = &mix_state;
  if (mix->midi_event_count < MAX_MIDI_EVENTS) {
    mix->midi_events[mix->midi_event_count++] = event;
  }
}

void mix_pause(void) {
  Mix* mix = &mix_state;
  mix->paused = true;
}

void mix_play(void) {
  Mix* mix = &mix_state;
  mix->paused = false;
}

void mix_stop(void) {
  Mix* mix = &mix_state;
  mix_pause();
  mix->tick = mix->timed_tick = 0;
}

void mix_set_bpm(i32 bpm) {
  Mix* mix = &mix_state;
  mix->bpm = CLAMP(bpm, BPM_MIN, BPM_MAX);
}

void mix_reset_tick(void) {
  Mix* mix = &mix_state;
  mix->tick = mix->timed_tick = 0;
}

size_t mix_get_tick(void) {
  Mix* mix = &mix_state;
  return mix->tick;
}

void mix_reload_ui(void) {
  Mix* mix = &mix_state;
  ui_free();
  ui_init();
  mix_reset(mix);
  mix_ui_new(mix);
}

void mix_update_and_render(Mix* mix) {
  Audio_engine* audio = &audio_engine;

  mix->mouse = GetMousePosition();
  delta_buffer[mix->tick % LENGTH(delta_buffer)] = mix->dt;

  keyboard_update();

  // handle midi-events
  mix->midi_event_count = midi_read_events(&mix->midi_events[0], MAX_MIDI_EVENTS);
  Midi_event event = {0};
  while (keyboard_query_event(&event)) {
    mix_send_midi_event(event);
  }

  bool mod_key = IsKeyDown(KEY_LEFT_CONTROL);

  if (audio->restart) {
    mix_restart_audio_engine();
    ui_free();
    ui_init();
    mix_ui_new(mix);
  }

  if (!ui_input_interacting()) {
    if (mod_key) {
      if (IsKeyPressed(KEY_R)) {
        if (IsKeyDown(KEY_LEFT_SHIFT)) {
          mix_restart_audio_engine();
        }
        audio_engine_clear_effects();
        ui_free();
        ui_init();
        mix_reset(mix);
        mix_ui_new(mix);
      }
      if (IsKeyPressed(KEY_L)) {
        config_load(CONFIG_PATH);
      }
      if (IsKeyPressed(KEY_M)) {
        memory_print_info(STDOUT_FILENO);
      }
      if (IsKeyPressed(KEY_SPACE)) {
        mix_stop();
      }
    }
    else {
      if (IsKeyPressed(KEY_SPACE)) {
        mix->paused = !mix->paused;
      }
      if (IsKeyPressed(KEY_KP_1) || IsKeyPressed(KEY_ONE)) {
        ui_switch_state(0);
      }
      if (IsKeyPressed(KEY_KP_2) || IsKeyPressed(KEY_TWO)) {
        ui_switch_state(1);
      }
      if (IsKeyPressed(KEY_KP_3) || IsKeyPressed(KEY_THREE)) {
        ui_switch_state(2);
      }
      if (IsKeyPressed(KEY_KP_4) || IsKeyPressed(KEY_FOUR)) {
        ui_switch_state(3);
      }
      if (IsKeyPressed(KEY_KP_5) || IsKeyPressed(KEY_FIVE)) {
        ui_switch_state(4);
      }
      if (IsKeyPressed(KEY_TAB)) {
        if (IsKeyDown(KEY_LEFT_SHIFT)) {
          ui_switch_state(ui_get_current_tag() - 1);
        }
        else {
          ui_switch_state(ui_get_current_tag() + 1);
        }
      }
#ifdef DEVELOPER
      if (IsKeyPressed(KEY_F1)) {
        show_debug_info = !show_debug_info;
      }
#endif
    }
  }

  instrument_update(&audio->instrument, mix);

  ui_update(mix->dt);
  ui_render();

#ifdef DEVELOPER
  if (show_debug_info) {
    static size_t debug_tick = 0;
    const size_t debug_text_update_interval = 2;
    static char debug_text[256] = {0};
    debug_tick += 1;
    bool update_text = !(debug_tick % debug_text_update_interval);
    if (update_text) {
      stb_snprintf(
        debug_text,
        sizeof(debug_text),
        "%zu/%zu bytes (%.2g %%)\n"
        "%.2g ms ui latency (rendering)\n"
        "%u/%u ui element updates\n"
        "%.2g/%.2g ms audio latency (%zu samples)"
        ,
        memory_state.usage, memory_state.max_usage,
        100 * ((f32)memory_state.usage / memory_state.max_usage),
        1000 * ui_state.render_latency,
        ui_state.element_update_count,
        ui_state.element_count,
        audio->dt * 1000,
        ((audio->frames_per_buffer * audio->channel_count) / (f32)audio->sample_rate) * 1000,
        audio->frames_per_buffer * audio->channel_count
      );
    }
    SetTextLineSpacing(0);
    DrawText(debug_text, 32, GetScreenHeight() - (FONT_SIZE_SMALLEST) * 4 - 16, FONT_SIZE_SMALLEST, COLOR_RGB(0xfc, 0xeb, 0x2f));
    render_delta_buffer(mix, update_text);
  }
#else
  (void)render_delta_buffer;
#endif
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
  mix->tick_delta = 0;
  mix->bpm = BPM_DEFAULT;
  mix->timer = 0.0f;
  mix->paused = false;
  mix->midi_event_count = 0;
  midi_init();
  keyboard_init();
  if (midi_open_device(MIDI_DEVICE_PATH) != Ok) {
    log_print(STDOUT_FILENO, LOG_TAG_WARN, "failed to open midi device `%s`\n", MIDI_DEVICE_PATH);
  }
}

void mix_free(Mix* m) {
  (void)m;
  audio_engine_exit(&audio_engine);
  ui_free();
  assets_unload(&assets);
  CloseWindow();
  midi_close();
}

void mix_ui_new(Mix* mix) {
#ifdef TEST_UI
  {
    Element e = test_ui_new(mix);
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
    e.x_padding = UI_CONTAINER_X_PADDING / 2;
    e.y_padding = UI_CONTAINER_Y_PADDING / 2;
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
  ui_switch_state(UI_TAG_EFFECT_CHAIN);

  Element* effect_chain_container = NULL;
  {
    Element chain = ui_container_ex(NULL, false);
    chain.border = false;
    chain.scissor = false;
    chain.placement = PLACEMENT_BLOCK;
    chain.background = true;
    effect_chain_container = ui_attach_element(NULL, &chain);
  }

  {
    Element e = ui_container("effect chain");
    e.sizing = SIZING_PERCENT(70, 100);
    e.border = true;
    e.scissor = true;
    e.placement = PLACEMENT_BLOCK;
    e.background = true;
#ifdef TARGET_ANDROID
    e.x_padding = UI_CONTAINER_X_PADDING * 4;
    e.y_padding = UI_CONTAINER_Y_PADDING * 2;
#endif
    mix->effect_chain = ui_attach_element(effect_chain_container, &e);
    effect_chain_ui_new(mix, effect_chain_container);
  }
  {
    Element e = ui_container("effect picker");
    e.sizing = SIZING_PERCENT(30, 100);
    e.border = true;
    e.scissor = true;
    e.placement = PLACEMENT_BLOCK;
    e.background = true;
    Element* effect_picker = ui_attach_element(effect_chain_container, &e);
    effect_picker_ui_new(mix, effect_picker);
  }

  ui_switch_state(UI_TAG_SETTINGS);
  ui_attach_element_v2(NULL, settings_ui_new(mix));

  ui_switch_state(UI_TAG_MIDI_SETTINGS);
  ui_attach_element_v2(NULL, midi_settings_ui_new(mix));

  ui_switch_state(UI_TAG_MAIN);
#endif
}

void render_delta_buffer(Mix* mix, bool update_text) {
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
  if (update_text) {
    snprintf(text, sizeof(text), "%g ms (average)\n%d fps", dt_avg * 1000, (i32)mix->fps);
  }
  SetTextLineSpacing(0);
  DrawText(text, x, y - (FONT_SIZE_SMALLEST) * 2, FONT_SIZE_SMALLEST, COLOR_RGB(0xfc, 0xeb, 0x2f));
}

void assets_load(Assets* assets) {
  assets->font = LoadFontEx(UI_FONT, UI_FONT_BASE_SIZE, NULL, 0);
  assets->sdf = LoadShader(0, SDF_SHADER);
  SetTextLineSpacing(UI_LINE_SPACING);
}

void assets_unload(Assets* assets) {
  UnloadFont(assets->font);
}
