// colors.h

#ifndef _COLORS_H
#define _COLORS_H

typedef enum {
  COLOR_NONE = 0,
  COLOR_RESET,
  COLOR_BLUE,
  COLOR_RED,
  COLOR_GREEN,
  COLOR_BOLD_WHITE,
  COLOR_YELLOW,

  MAX_COLOR,
} Log_color;

#define USE_COLOR_FD(FD, LOG_COLOR, ...) { color_begin(LOG_COLOR); __VA_ARGS__; color_end(); }
#define USE_COLOR(LOG_COLOR, ...) USE_COLOR_FD(STDOUT_FILENO, LOG_COLOR, __VA_ARGS__)

extern const char* color_str[MAX_COLOR];

void colors_init(bool use_colors);
void color_begin(Log_color color);
void color_end(void);
void color_begin_fd(i32 fd, Log_color color);
void color_end_fd(i32 fd);

#endif // _COLORS_H
