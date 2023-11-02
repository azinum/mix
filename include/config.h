// config.h

#ifndef _CONFIG_H
#define _CONFIG_H

static i32 WINDOW_WIDTH       = 1600;
static i32 WINDOW_HEIGHT      = 900;
static i32 WINDOW_RESIZABLE   = true;
static i32 MSAA_4X            = false;
static i32 FONT_SIZE          = 22;
static i32 FONT_SIZE_SMALLER  = 20;
static i32 FONT_SIZE_SMALLEST = 10;
static i32 TARGET_FPS         = 60;
static i32 FRAMES_PER_BUFFER  = 512;
static i32 SAMPLE_RATE        = 44100;
static i32 CHANNEL_COUNT      = 2;

void config_init(void);
Result config_store(const char* path);
Result config_load(const char* path);

#endif // _CONFIG_H
