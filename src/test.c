// test.c

#define USE_STB_SPRINTF
#define STB_SPRINTF_IMPLEMENTATION
#include "ext/stb/stb_sprintf.h"

#define COMMON_IMPLEMENTATION
#include "common.h"

typedef struct Color {
  u8 r;
  u8 g;
  u8 b;
  u8 a;
} Color;

#define COLOR_RGB(R, G, B) ((Color) { .r = R, .g = G, .b = B, .a = 255 })

#include "misc.h"
#include "random.h"
#include "misc.c"
#include "random.c"

i32 main(void) {
  char hex[7] = "000000";
  random_init(1234);
  size_t n = 100000;
  for (size_t i = 0; i < n; ++i) {
    u8 r = random_number() % 255;
    u8 g = random_number() % 255;
    u8 b = random_number() % 255;
    stb_snprintf(hex, LENGTH(hex), "%02x%02x%02x", r, g, b);
    Color color = hex_string_to_color(hex);
    ASSERT(r == color.r && g == color.g && b == color.b);
  }
  return 0;
}
