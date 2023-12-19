// misc.h

#ifndef _MISC_H
#define _MISC_H

extern f32 lerpf32(f32 a, f32 b, f32 t);
extern Color lerpcolor(Color a, Color b, f32 t);
extern void print_bits(i32 fd, char byte);
extern Color hex_string_to_color(char* hex);

#endif // _MISC_H
