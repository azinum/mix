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

void log_init(bool use_colors);
void log_print(i32 fd, Log_tag tag, const char* fmt, ...);
void log_print_tag(i32 fd, const char* tag, Log_color tag_color);

#endif // _LOG_H
