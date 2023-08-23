// log.h

#ifndef _LOG_H
#define _LOG_H

typedef enum {
  LOG_TAG_NONE = 0,
  LOG_TAG_INFO,
  LOG_TAG_ERROR,
  LOG_TAG_SUCCESS,
  LOG_TAG_WARN,

  MAX_LOG_TAG,
} Log_tag;

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

void log_init(u32 use_colors);
void log_print(i32 fd, Log_tag tag, const char* fmt, ...);
void log_print_tag(i32 fd, const char* tag, Log_color tag_color);

#endif // _LOG_H
