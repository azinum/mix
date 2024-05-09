// misc.c

inline bool is_alpha(char ch);
inline bool is_digit(char ch);
inline char to_lower(char ch);

bool is_alpha(char ch) {
  return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

bool is_digit(char ch) {
  return (ch >= '0') && (ch <= '9');
}

char to_lower(char ch) {
  if ((ch >= 'A') && (ch <= 'Z')) {
    return ch + 32;
  }
  return ch;
}

inline f32 lerp_f32(f32 a, f32 b, f32 t) {
  return (1.0f - t) * a + t * b;
}

inline Color lerp_color(Color a, Color b, f32 t) {
  return COLOR_RGB(
    (u8)lerp_f32(a.r, b.r, t),
    (u8)lerp_f32(a.g, b.g, t),
    (u8)lerp_f32(a.b, b.b, t)
  );
}

inline Color warmer_color(Color a, u8 amount) {
  return COLOR_RGB(
    CLAMP(a.r + amount, 0, 255),
    a.g,
    CLAMP(a.b - amount, 0, 255)
  );
}

Color invert_color(Color a) {
  return COLOR_RGB(
    255 - a.r,
    255 - a.g,
    255 - a.b
  );
}

Color saturate_color(Color a, f32 amount) {
  Hsv hsv = rgb_to_hsv(a);
  hsv.s = CLAMP(hsv.s + amount, 0, 1);
  return hsv_to_rgb(hsv);
}

Color brighten_color(Color a, f32 amount) {
  Hsv hsv = rgb_to_hsv(a);
  hsv.v = CLAMP(hsv.v + amount, 0, 1);
  return hsv_to_rgb(hsv);
}

// https://www.calculatorology.com/rgb-to-hsv-conversion/
Hsv rgb_to_hsv(Color color) {
  const f32 r = color.r / 255.0f;
  const f32 g = color.g / 255.0f;
  const f32 b = color.b / 255.0f;
  const f32 c_min = MIN(MIN(r, g), b);
  const f32 c_max = MAX(MAX(r, g), b);
  const f32 dc = c_max - c_min;
  Hsv hsv = {0, 0, 0};

  if (r == c_max && dc != 0) {
    hsv.h = 60.0f * fmod((g - b) / dc, 6.0f);
  }
  else if (g == c_max && dc != 0) {
    hsv.h = 60.0f * (((b - r) / dc) + 2);
  }
  else if (b == c_max && dc != 0) {
    hsv.h = 60.0f * (((r - g) / dc) + 4);
  }

  if (hsv.h < 0) {
    hsv.h += 360;
  }
  hsv.h /= 360.0f; // normalize to be between 0.0-1.0

  if (c_max != 0) {
    hsv.s = dc / c_max;
  }
  else {
    hsv.s = 0;
  }

  hsv.v = c_max;
  return hsv;
}

// http://www.easyrgb.com/en/math.php
Color hsv_to_rgb(Hsv hsv) {
  if (hsv.s == 0) {
    return COLOR_RGB(
      (u8)(hsv.v * 255),
      (u8)(hsv.v * 255),
      (u8)(hsv.v * 255)
    );
  }
  // h = hue when hue < 1, otherwize h = 0
  const f32 h = hsv.h * (hsv.h < 1) * 6;
  const f32 i = (i32)h;
  const f32 x = hsv.v * (1 - hsv.s);
  const f32 y = hsv.v * (1 - hsv.s * (h - i));
  const f32 z = hsv.v * (1 - hsv.s * (1 - (h - i)));

  f32 r = 0;
  f32 g = 0;
  f32 b = 0;

  switch ((i32)i) {
    case 0: {
      r = hsv.v;
      g = z;
      b = x;
      break;
    }
    case 1: {
      r = y;
      g = hsv.v;
      b = x;
      break;
    }
    case 2: {
      r = x;
      g = hsv.v;
      b = z;
      break;
    }
    case 3: {
      r = x;
      g = y;
      b = hsv.v;
      break;
    }
    case 4: {
      r = z;
      g = x;
      b = hsv.v;
      break;
    }
    default: {
      r = hsv.v;
      g = x;
      b = y;
      break;
    }
  }

  return COLOR_RGB(
    (u8)(r * 255),
    (u8)(g * 255),
    (u8)(b * 255)
  );
}

void print_bits(i32 fd, char byte) {
  for (i32 bit = 7; bit >= 0; --bit) {
    stb_dprintf(fd, "%d", EXTRACTBIT(bit, byte) != 0);
  }
}

Color hex_string_to_color(char* hex) {
#define MAX_HEX_LENGTH 8
  Color color = { .r = 255, .g = 0, .b = 255, .a = 255};
  size_t length = strnlen(hex, MAX_HEX_LENGTH);
  if (!(length == 6 || length == 8)) {
    return color;
  }

  u8 copy[MAX_HEX_LENGTH] = {0};
  stb_snprintf((char*)copy, MAX_HEX_LENGTH, "%s", hex);
  for (size_t i = 0; i < length; ++i) {
    if (is_alpha(hex[i])) {
      copy[i] = (to_lower(hex[i]) - 'a' + 10) << 4;
    }
    if (is_digit(hex[i])) {
      copy[i] = (hex[i] - '0') << 4;
    }
  }

  for (size_t i = 0; i < length; i += 2) {
    copy[i] = copy[i] + (copy[i+1] >> 4);
  }

  color.r = copy[0];
  color.g = copy[2];
  color.b = copy[4];
  if (length == 8) {
    color.a = copy[6];
  }

  return color;
}

f32 get_time(void) {
  struct timespec ts = {0};
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec / 1000000000.0f;
}

i32 fd_open(const char* path, i32 flags, ...) {
  char mode_str[16] = {0};
  i32 mode_index = 0;
  if ((flags & O_RDONLY) == O_RDONLY) {
    mode_index += snprintf(&mode_str[mode_index], sizeof(mode_str) - mode_index, "r");
  }
  if ((flags & O_WRONLY) == O_WRONLY) {
    mode_index += snprintf(&mode_str[mode_index], sizeof(mode_str) - mode_index, "w");
  }
  mode_index += snprintf(&mode_str[mode_index], sizeof(mode_str) - mode_index, "b");
  FILE* fp = fopen(path, mode_str);
  if (!fp) {
    return -1;
  }
  return fileno(fp);
}

char* file_extension(const char* path) {
  char* result = (char*)path;
  for (;;) {
    char ch = *path++;
    if (ch == 0) {
      return result;
    }
    if (ch == '.') {
      return (char*)path - 1;
    }
  }
  return result;
}

inline f32 dot_product(f32 a, f32 b) {
  return (a * b);
}

inline f32 v2_dot_product(Vector2 a, Vector2 b) {
  return dot_product(a.x, b.x) + dot_product(a.y, b.y);
}

inline bool is_aligned(const void* p, size_t boundary) {
  return ((size_t)p % boundary) == 0;
}
