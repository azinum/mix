// log.c
// TODO:
//  - log levels

struct {
  u32 use_colors;
} log_state = {
  .use_colors = false,
};

static const char* color_str[MAX_COLOR] = {
  [COLOR_NONE]       = "",
  [COLOR_RESET]      = "\033[0;00m",
  [COLOR_BLUE]       = "\033[0;34m",
  [COLOR_RED]        = "\033[0;31m",
  [COLOR_GREEN]      = "\033[0;32m",
  [COLOR_BOLD_WHITE] = "\033[1;37m",
  [COLOR_YELLOW]     = "\033[1;33m",
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

void log_init(u32 use_colors) {
#ifndef NO_COLORS
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

  if (log_state.use_colors) {
    STB_WRAP(dprintf)(fd, "%s", color_str[tag_color]);
  }
  STB_WRAP(dprintf)(fd, "[%s]: ", tag);
  if (log_state.use_colors) {
    STB_WRAP(dprintf)(fd, "%s", color_str[COLOR_RESET]);
  }
}
