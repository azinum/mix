// config.c
//
// flags:
//  CONFIG_VERBOSE_OUTPUT
//
// TODO:
//  - control when lua does GC sweeps

#define MAX_DIFF_BUFF_SIZE MAX_CONFIG_STRING_SIZE

#define CONFIG_VERBOSE_OUTPUT

#ifdef CONFIG_VERBOSE_OUTPUT
  #define O(...) __VA_ARGS__
#else
  #define O(...)
#endif

typedef enum Type {
  T_INT,
  T_FLOAT,
  T_STRING,
  T_COLOR,

  MAX_TYPE,
} Type;

static size_t type_size[] = {
  sizeof(i32),
  sizeof(f32),
  sizeof(Color),
  MAX_CONFIG_STRING_SIZE,
};

typedef struct Variable {
  const char* name;
  Type type;
  void* data;
  void (*hook)(struct Variable*); // hook callback function to call when variable is modified
} Variable;

static const luaL_Reg lualibs[] = {
  { "base", luaopen_base, },
  { "table", luaopen_table, },
  { NULL, NULL, },
};

static void hook_default(Variable* v);
static void hook_target_fps(Variable* v);
static void hook_warn_restart(Variable* v);
static void hook_restart_audio_engine(Variable* v);

static void write_variable(i32 fd, const char* name, Type type, void* data);
static Result read_variable(const char* name, Type type, void* data);
static void lua_open_libs(lua_State* l);

static Variable variables[] = {
  { "window_width", T_INT, &WINDOW_WIDTH, hook_default },
  { "window_height", T_INT, &WINDOW_HEIGHT, hook_default },
  { "window_resizable", T_INT, &WINDOW_RESIZABLE, hook_warn_restart },
  { "window_fullscreen", T_INT, &WINDOW_FULLSCREEN, hook_warn_restart },
  { "vsync", T_INT, &VSYNC, hook_warn_restart },
  { "msaa_4x", T_INT, &MSAA_4X, hook_warn_restart },
  { "font_size", T_INT, &FONT_SIZE, hook_default },
  { "font_size_small", T_INT, &FONT_SIZE_SMALL, hook_default },
  { "font_size_smallest", T_INT, &FONT_SIZE_SMALLEST, hook_default },
  { "target_fps", T_INT, &TARGET_FPS, hook_target_fps },
  { "frames_per_buffer", T_INT, &FRAMES_PER_BUFFER, hook_restart_audio_engine },
  { "sample_rate", T_INT, &SAMPLE_RATE, hook_restart_audio_engine },
  { "channel_count", T_INT, &CHANNEL_COUNT, hook_restart_audio_engine },
  { "ui_padding", T_INT, &UI_PADDING, hook_default },
  { "ui_font", T_STRING, &UI_FONT, hook_warn_restart },
  { "ui_font_base_size", T_INT, &UI_FONT_BASE_SIZE, hook_warn_restart },
  { "ui_line_spacing", T_INT, &UI_LINE_SPACING, hook_default },
  { "audio_input", T_INT, &AUDIO_INPUT, hook_restart_audio_engine },
  { "audio_pa_in_port_id", T_INT, &AUDIO_PA_IN_PORT_ID, hook_restart_audio_engine },
  { "audio_pa_out_port_id", T_INT, &AUDIO_PA_OUT_PORT_ID, hook_restart_audio_engine },
  { "main_background_color", T_COLOR, &MAIN_BACKGROUND_COLOR, hook_default },
  { "ui_background_color", T_COLOR, &UI_BACKGROUND_COLOR, hook_default },
  { "ui_border_color", T_COLOR, &UI_BORDER_COLOR, hook_default },
  { "ui_button_color", T_COLOR, &UI_BUTTON_COLOR, hook_default },
  { "ui_text_color", T_COLOR, &UI_TEXT_COLOR, hook_default },
  { "ui_border_thickness", T_FLOAT, &UI_BORDER_THICKNESS, hook_default },
  { "ui_title_bar_padding", T_INT, &UI_TITLE_BAR_PADDING, hook_default },
  { "ui_button_roundness", T_FLOAT, &UI_BUTTON_ROUNDNESS, hook_default },
  { "ui_slider_inner_padding", T_INT, &UI_SLIDER_INNER_PADDING, hook_default },
  { "ui_slider_knob_size", T_INT, &UI_SLIDER_KNOB_SIZE, hook_default },
  { "ui_theme", T_STRING, &UI_THEME, hook_default },
};

struct {
  lua_State* l;
  u32 load_count; // number of times config has been loaded since startup
} config = {
  .l = NULL,
  .load_count = 0,
};

void config_init(void) {
  ASSERT(MAX_CONFIG_STRING_SIZE >= sizeof(size_t));

  // unused functions from lua
  (void)print_usage;
  (void)print_version;
  (void)createargtable;
  (void)handle_script;
  (void)collectargs;
  (void)runargs;
  (void)handle_luainit;
  (void)doREPL;

  config.l = luaL_newstate();
  config.load_count = 0;
  if (!config.l) {
    log_print(STDERR_FILENO, LOG_TAG_ERROR, "failed to initialize lua state\n");
    return;
  }
  lua_open_libs(config.l);
}

Result config_store(const char* path) {
  i32 flags = O_WRONLY | O_CREAT | O_TRUNC;
  i32 mode = 0664;
  i32 fd = open(path, flags, mode);
  if (fd < 0) {
    log_print(STDERR_FILENO, LOG_TAG_ERROR, "failed to open file `%s` for writing\n", path);
    return Error;
  }
  stb_dprintf(fd, "-- %s\n", path);
  for (size_t i = 0; i < LENGTH(variables); ++i) {
    Variable* v = &variables[i];
    write_variable(fd, v->name, v->type, v->data);
  }
  stb_dprintf(fd, "require ('data/theme')\n");
  close(fd);
  return Ok;
}

Result config_load(const char* path) {
  TIMER_START();
  config_init();
  if (dofile(config.l, path) != LUA_OK) {
    return Error;
  }
  char diff_buff[MAX_DIFF_BUFF_SIZE] = {0};
  size_t num_hooks_called = 0;

  for (size_t i = 0; i < LENGTH(variables); ++i) {
    Variable* v = &variables[i];
    size_t data_size = type_size[v->type];
    memcpy(diff_buff, v->data, data_size); // copy current value
    if (read_variable(v->name, v->type, v->data) == Ok) {
      ASSERT(v->hook != NULL);
      // only need to run hooks when loading config more than once and when a value has actually changed
      if (config.load_count > 0 && hash_djb2((u8*)diff_buff, data_size) != hash_djb2((u8*)v->data, data_size)) {
        v->hook(v);
        num_hooks_called += 1;
        O(log_print(STDOUT_FILENO, LOG_TAG_INFO, "setting `%s` was changed\n", v->name));
      }
    }
  }
  f32 dt = TIMER_END();
  log_print(STDOUT_FILENO, LOG_TAG_INFO, "loaded config `%s` in %g ms (%zu hooks)\n", path, 1000 * dt, num_hooks_called);
  config.load_count += 1;
  config_free();
  return Ok;
}

void config_free(void) {
  if (config.l) {
    lua_close(config.l);
    config.l = NULL;
  }
}

void hook_default(Variable* v) {
  (void)v;
}

void hook_target_fps(Variable* v) {
  (void)v;
  SetTargetFPS(TARGET_FPS);
}

void hook_warn_restart(Variable* v) {
  log_print(STDOUT_FILENO, LOG_TAG_WARN, "setting `%s` was changed, program needs to be restarted for it to take effect\n", v->name);
}

void hook_restart_audio_engine(Variable* v) {
  log_print(STDOUT_FILENO, LOG_TAG_WARN, "setting `%s` was changed, restarting audio engine...\n", v->name);
  audio_engine_restart();
}

void write_variable(i32 fd, const char* name, Type type, void* data) {
  switch (type) {
    case T_INT: {
      stb_dprintf(fd, "%s = %d\n", name, *(i32*)data);
      break;
    }
    case T_FLOAT: {
      stb_dprintf(fd, "%s = %.6f\n", name, *(f32*)data);
      break;
    }
    case T_STRING: {
      stb_dprintf(fd, "%s = \"%s\"\n", name, (char*)data);
      break;
    }
    case T_COLOR: {
      Color* c = (Color*)data;
      stb_dprintf(fd, "%s = \"%02x%02x%02x\"\n", name, c->r, c->g, c->b);
      break;
    }
    default:
      break;
  }
}

Result read_variable(const char* name, Type type, void* data) {
  lua_State* l = config.l;
  lua_getglobal(l, name);
  // TODO(lucas): handle errors
  switch (type) {
    case T_INT: {
      if (lua_isnumber(l, -1)) {
        i32 value = lua_tointeger(l, -1);
        *(i32*)data = value;
        lua_pop(l, 1);
        return Ok;
      }
      lua_pop(l, 1);
      return Error;
    }
    case T_FLOAT: {
      if (lua_isnumber(l, -1)) {
        f32 value = (f32)lua_tonumber(l, -1);
        *(f32*)data = value;
        lua_pop(l, 1);
        return Ok;
      }
      lua_pop(l, 1);
      return Error;
    }
    case T_STRING: {
      if (lua_isstring(l, -1)) {
        const char* value = lua_tostring(l, -1);
        strncpy((char*)data, value, MAX_CONFIG_STRING_SIZE);
        lua_pop(l, 1);
        return Ok;
      }
      lua_pop(l, 1);
      break;
    }
    case T_COLOR: {
      if (lua_isstring(l, -1)) {
        const char* value = lua_tostring(l, -1);
        Color color = hex_string_to_color((char*)value);
        *(Color*)data = color;
        return Ok;
      }
      lua_pop(l, 1);
      break;
    }
    default:
      break;
  }
  return Error;
}

void lua_open_libs(lua_State* l) {
  luaopen_package(l);
  for (const luaL_Reg* lib = lualibs; lib->func != NULL; lib++) {
    lib->func(l);
    lua_settop(l, 0);
  }
}
