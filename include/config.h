// config.h

#ifndef _CONFIG_H
#define _CONFIG_H

#define MAX_CONFIG_STRING_SIZE 64

static i32 WINDOW_WIDTH         = 0;
static i32 WINDOW_HEIGHT        = 0;
static i32 WINDOW_RESIZABLE     = true;
static i32 WINDOW_FULLSCREEN    = true;
static i32 VSYNC                = false;
static i32 MSAA_4X              = false;
static i32 FONT_SIZE            = 20;
static i32 FONT_SIZE_SMALL      = 16;
static i32 FONT_SIZE_SMALLEST   = 10;
static i32 TARGET_FPS           = 60;
static i32 FRAMES_PER_BUFFER    = 512;
static i32 SAMPLE_RATE          = 44100;
static i32 CHANNEL_COUNT        = 2;
static i32 UI_PADDING           = 8;
static char UI_FONT[MAX_CONFIG_STRING_SIZE] = "data/fonts/notomono-regular.ttf";
static i32 UI_FONT_BASE_SIZE    = 20;
static i32 UI_LINE_SPACING      = 20;
static i32 AUDIO_INPUT          = 0;
static i32 AUDIO_PA_IN_PORT_ID  = -1;
static i32 AUDIO_PA_OUT_PORT_ID = -1;

static Color MAIN_BACKGROUND_COLOR = COLOR_RGB(0x17, 0x17, 0x17);
static Color UI_BACKGROUND_COLOR = COLOR_RGB(0x30, 0x30, 0x30);
static Color UI_BORDER_COLOR = COLOR_RGB(0x12, 0x12, 0x12);
static Color UI_BUTTON_COLOR = COLOR_RGB(0x48, 0x58, 0x95);
static Color UI_TEXT_COLOR = COLOR_RGB(0xf0, 0xf0, 0xf0);
static Color UI_FOCUS_COLOR = COLOR_RGB(0x95, 0x95, 0xd0);
static Color UI_TITLE_BAR_COLOR = COLOR_RGB(0x27, 0x27, 0x27);
static Color UI_INTERPOLATION_COLOR = COLOR_RGB(0xff, 0xff, 0xff);
static f32 UI_BORDER_THICKNESS = 1.1f;
static i32 UI_TITLE_BAR_PADDING = 8;
static f32 UI_BUTTON_ROUNDNESS = 0.1f;
static i32 UI_SLIDER_INNER_PADDING = 2;
static char UI_THEME[MAX_CONFIG_STRING_SIZE] = "dark";
static f32 UI_ROUNDNESS = 0.0f;

Result config_init(void);
Result config_store(const char* path);
Result config_load(const char* path);
void config_free(void);

#endif // _CONFIG_H
