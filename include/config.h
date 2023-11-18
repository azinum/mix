// config.h

#ifndef _CONFIG_H
#define _CONFIG_H

#define MAX_CONFIG_STRING_SIZE 64

static i32 WINDOW_WIDTH       = 1600;
static i32 WINDOW_HEIGHT      = 900;
static i32 WINDOW_RESIZABLE   = true;
static i32 MSAA_4X            = false;
static i32 FONT_SIZE          = 32;
static i32 FONT_SIZE_SMALL    = 20;
static i32 FONT_SIZE_SMALLEST = 10;
static i32 TARGET_FPS         = 60;
static i32 FRAMES_PER_BUFFER  = 512;
static i32 SAMPLE_RATE        = 44100;
static i32 CHANNEL_COUNT      = 2;
static i32 UI_PADDING         = 8;
static const char* UI_FONT    = "data/fonts/notomono-regular.ttf";
static i32 UI_FONT_BASE_SIZE  = 20;
static i32 UI_LINE_SPACING    = 20;

void config_init(void);
Result config_store(const char* path);
Result config_load(const char* path);
void config_free(void);

#endif // _CONFIG_H
