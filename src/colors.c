// colors.c

static bool colors_are_enabled = true;

const char* color_str[MAX_COLOR] = {
  [COLOR_NONE]       = "",
  [COLOR_RESET]      = "\033[0;00m",
  [COLOR_BLUE]       = "\033[0;34m",
  [COLOR_RED]        = "\033[0;31m",
  [COLOR_GREEN]      = "\033[0;32m",
  [COLOR_BOLD_WHITE] = "\033[1;37m",
  [COLOR_YELLOW]     = "\033[1;33m",
};

void colors_init(bool use_colors) {
  colors_are_enabled = use_colors;
}

void color_begin(Log_color color) {
  color_begin_fd(STDOUT_FILENO, color);
}

void color_end(void) {
  color_end_fd(STDOUT_FILENO);
}

void color_begin_fd(i32 fd, Log_color color) {
#ifndef NO_COLORS
  if (colors_are_enabled && (color >= 0) && (color < MAX_COLOR)) {
    STB_WRAP(dprintf)(fd, "%s", color_str[color]);
  }
#endif
}

void color_end_fd(i32 fd) {
#ifndef NO_COLORS
  if (colors_are_enabled) {
    STB_WRAP(dprintf)(fd, "%s", color_str[COLOR_RESET]);
  }
#endif
}
