// config.h

#ifndef _CONFIG_H
#define _CONFIG_H

static size_t WINDOW_WIDTH      = 1600;
static size_t WINDOW_HEIGHT     = 900;
static bool   WINDOW_RESIZABLE  = true;
static bool   MSAA_4X           = true;
static size_t FONT_SIZE         = 22;
static size_t FONT_SIZE_SMALLER = 20;
static Color  COLOR_BG          = (Color) { .r = 32, .g = 32, .b = 32, .a = 255, };
static size_t TARGET_FPS        = 60;

#endif // _CONFIG_H
