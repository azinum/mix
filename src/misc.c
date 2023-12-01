// misc.c

inline f32 lerpf32(f32 a, f32 b, f32 t) {
  return (1.0f - t) * a + t * b;
}

inline Color lerpcolor(Color a, Color b, f32 t) {
  return COLOR_RGB(
    (u8)lerpf32(a.r, b.r, t),
    (u8)lerpf32(a.g, b.g, t),
    (u8)lerpf32(a.b, b.b, t)
  );
}

void print_bits(u32 fd, char byte) {
  for (i32 bit = 7; bit >= 0; --bit) {
    stb_dprintf(fd, "%d", EXTRACTBIT(bit, byte) != 0);
  }
}
