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
