// config.c

typedef enum Type {
  T_INT,
  T_FLOAT,
  T_STRING,

  MAX_TYPE,
} Type;

typedef struct Variable {
  const char* name;
  Type type;
  void* data;
} Variable;

static Variable variables[] = {
  { "window_width", T_INT, &WINDOW_WIDTH },
  { "window_height", T_INT, &WINDOW_HEIGHT },
  { "window_resizable", T_INT, &WINDOW_RESIZABLE },
  { "msaa_4x", T_INT, &MSAA_4X },
  { "font_size", T_INT, &FONT_SIZE },
  { "font_size_small", T_INT, &FONT_SIZE_SMALL },
  { "font_size_smallest", T_INT, &FONT_SIZE_SMALLEST },
  { "target_fps", T_INT, &TARGET_FPS },
  { "frames_per_buffer", T_INT, &FRAMES_PER_BUFFER },
  { "sample_rate", T_INT, &SAMPLE_RATE },
  { "channel_count", T_INT, &CHANNEL_COUNT },
  { "ui_padding", T_INT, &UI_PADDING },
  { "ui_font_base_size", T_INT, &UI_FONT_BASE_SIZE },
  { "ui_line_spacing", T_INT, &UI_LINE_SPACING },
};

static const luaL_Reg lualibs[] = {
  { "base", luaopen_base, },
  { NULL, NULL, },
};

static void write_variable(i32 fd, const char* name, Type type, void* data);
static void read_variable(const char* name, Type type, void* data);

static void lua_open_libs(lua_State* l);

struct {
  lua_State* l;
} config = {
  .l = NULL,
};

void config_init(void) {
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

  close(fd);
  return Ok;
}

Result config_load(const char* path) {
  dofile(config.l, path);
  for (size_t i = 0; i < LENGTH(variables); ++i) {
    Variable* v = &variables[i];
    read_variable(v->name, v->type, v->data);
  }
  return Ok;
}

void config_free(void) {
  if (config.l) {
    lua_close(config.l);
    config.l = NULL;
  }
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
      NOT_IMPLEMENTED();
      break;
    }
    default:
      break;
  }
}

void read_variable(const char* name, Type type, void* data) {
  lua_State* l = config.l;
  lua_getglobal(l, name);
  // TODO(lucas): handle errors
  switch (type) {
    case T_INT: {
      if (lua_isnumber(l, -1)) {
        i32 value = lua_tointeger(l, -1);
        *(i32*)data = value;
      }
      lua_pop(l, 1);
      break;
    }
    case T_FLOAT: {
      if (lua_isnumber(l, -1)) {
        f32 value = (f32)lua_tonumber(l, -1);
        *(f32*)data = value;
      }
      lua_pop(l, 1);
      break;
    }
    case T_STRING: {
      NOT_IMPLEMENTED();
      break;
    }
    default:
      break;
  }
}

void lua_open_libs(lua_State* l) {
  for (const luaL_Reg* lib = lualibs; lib->func != NULL; lib++) {
    lib->func(l);
    lua_settop(l, 0);
  }
}
