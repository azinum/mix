// config.c

typedef enum Type {
  T_INT,
  T_FLOAT,
  T_STRING,

  MAX_TYPE,
} Type;

static void write_variable(i32 fd, const char* name, Type type, void* data);

void config_init(void) {

}

Result config_store(const char* path) {
  i32 flags = O_WRONLY | O_CREAT | O_TRUNC;
  i32 mode = 0664;
  i32 fd = open(path, flags, mode);
  if (fd < 0) {
    log_print(STDERR_FILENO, LOG_TAG_ERROR, "failed to open file `%s` for writing\n", path);
    return Error;
  }
  write_variable(fd, "window_width", T_INT, &WINDOW_WIDTH);
  write_variable(fd, "window_height", T_INT, &WINDOW_HEIGHT);
  write_variable(fd, "window_resizable", T_INT, &WINDOW_RESIZABLE);
  write_variable(fd, "msaa_4x", T_INT, &MSAA_4X);
  write_variable(fd, "font_size", T_INT, &FONT_SIZE);
  write_variable(fd, "font_size_smaller", T_INT, &FONT_SIZE_SMALLER);
  write_variable(fd, "font_size_smallest", T_INT, &FONT_SIZE_SMALLEST);
  write_variable(fd, "target_fps", T_INT, &TARGET_FPS);
  write_variable(fd, "frames_per_buffer", T_INT, &FRAMES_PER_BUFFER);
  write_variable(fd, "sample_rate", T_INT, &SAMPLE_RATE);
  write_variable(fd, "channel_count", T_INT, &CHANNEL_COUNT);

  close(fd);
  return Ok;
}

Result config_load(const char* path) {
  (void)path;
  return Ok;
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
