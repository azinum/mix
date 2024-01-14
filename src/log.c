// log.c
// TODO:
//  - log levels

struct {
  bool use_colors;
} log_state = {
  .use_colors = false,
};

static const char* tags[MAX_LOG_TAG] = {
  [LOG_TAG_NONE]    = "",
  [LOG_TAG_INFO]    = "info",
  [LOG_TAG_ERROR]   = "error",
  [LOG_TAG_SUCCESS] = "success",
  [LOG_TAG_WARN]    = "warning",
};

static Log_color tag_colors[MAX_LOG_TAG] = {
  COLOR_NONE,
  COLOR_BLUE,
  COLOR_RED,
  COLOR_GREEN,
  COLOR_YELLOW,
};

void log_init(bool use_colors) {
#ifndef NO_COLORS
  colors_init(use_colors && enable_vt100_mode());
  log_state.use_colors = use_colors && enable_vt100_mode();
#else
  log_state.use_colors = false;
#endif
}

void log_print(i32 fd, Log_tag tag, const char* fmt, ...) {
  ASSERT(tag < MAX_LOG_TAG && "invalid tag");
  log_print_tag(fd, tags[tag], tag_colors[tag]);
  va_list argp;
  va_start(argp, fmt);
  STB_WRAP(vprintf)(fmt, argp);
  va_end(argp);
}

void log_print_tag(i32 fd, const char* tag, Log_color tag_color) {
  ASSERT(tag != NULL);
  ASSERT(tag_color < MAX_COLOR);

  color_begin_fd(fd, tag_color);
  STB_WRAP(dprintf)(fd, "[%s]: ", tag);
  color_end_fd(fd);
}
