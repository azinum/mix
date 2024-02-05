// misc.h

#ifndef _MISC_H
#define _MISC_H

typedef struct Hsv {
  f32 h;
  f32 s;
  f32 v;
} Hsv;

extern f32 lerp_f32(f32 a, f32 b, f32 t);
extern Color lerp_color(Color a, Color b, f32 t);
extern Color warmer_color(Color a, u8 amount);
extern Color invert_color(Color a);
extern Hsv rgb_to_hsv(Color color);
extern Color hsv_to_rgb(Hsv hsv);
extern void print_bits(i32 fd, char byte);
extern Color hex_string_to_color(char* hex);
extern f32 get_time(void);
extern i32 fd_open(const char* path, i32 flags, ...);

#endif // _MISC_H
