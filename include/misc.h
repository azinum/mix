// misc.h

#ifndef _MISC_H
#define _MISC_H

typedef struct Hsv {
  f32 h;
  f32 s;
  f32 v;
} Hsv;

// TODO: start from C0 @ ~16.35 hz
// C1 -> B8
// equal temperament scale
static const f32 freq_table[] = {
  // C          C#            D             D#            E             F             F#            G             G#            A             A#            B
  32.703194f,   34.647831f,   36.708096f,   38.890873f,   41.203445f,   43.653530f,   46.249302f,   48.999428f,   51.913086f,   55.000000f,   58.270473f,   61.735413f,
  65.406387f,   69.295662f,   73.416191f,   77.781746f,   82.406891f,   87.307060f,   92.498604f,   97.998856f,   103.826172f,  110.000000f,  116.540947f,  123.470825f,
  130.812775f,  138.591324f,  146.832382f,  155.563492f,  164.813782f,  174.614120f,  184.997208f,  195.997726f,  207.652344f,  220.000000f,  233.081863f,  246.941681f,
  261.625549f,  277.182617f,  293.664795f,  311.126984f,  329.627533f,  349.228241f,  369.994415f,  391.995392f,  415.304718f,  440.000000f,  466.163727f,  493.883362f,
  523.251099f,  554.365234f,  587.329590f,  622.253967f,  659.255066f,  698.456482f,  739.988831f,  783.990784f,  830.609436f,  880.000000f,  932.327698f,  987.766479f,
  1046.502197f, 1108.730591f, 1174.658936f, 1244.507935f, 1318.510376f, 1396.912842f, 1479.977661f, 1567.981934f, 1661.218506f, 1760.000000f, 1864.655396f, 1975.532959f,
  2093.004395f, 2217.461182f, 2349.317871f, 2489.015869f, 2637.020752f, 2793.825684f, 2959.955322f, 3135.963867f, 3322.437012f, 3520.000000f, 3729.310791f, 3951.065918f,
  4186.008789f, 4434.922363f, 4698.635742f, 4978.031738f, 5274.041504f, 5587.651367f, 5919.910645f, 6271.927734f, 6644.874023f, 7040.000000f, 7458.621582f, 7902.131836f,
};

extern f32 lerp_f32(f32 a, f32 b, f32 t);
extern Color lerp_color(Color a, Color b, f32 t);
extern Color warmer_color(Color a, u8 amount);
extern Color invert_color(Color a);
extern Color saturate_color(Color a, f32 amount);
extern Hsv rgb_to_hsv(Color color);
extern Color hsv_to_rgb(Hsv hsv);
extern void print_bits(i32 fd, char byte);
extern Color hex_string_to_color(char* hex);
extern f32 get_time(void);
extern i32 fd_open(const char* path, i32 flags, ...);
extern char* file_extension(const char* path);
extern f32 dot_product(f32 a, f32 b);
extern f32 v2_dot_product(Vector2 a, Vector2 b);

#endif // _MISC_H
