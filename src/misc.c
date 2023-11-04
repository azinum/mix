// misc.c

inline f32 lerpf32(f32 a, f32 b, f32 t) {
  return (1.0f - t) * a + t * b;
}

Color lerpcolor(Color a, Color b, f32 t) {
  return COLOR_RGB(
    (u8)lerpf32(a.r, b.r, t),
    (u8)lerpf32(a.g, b.g, t),
    (u8)lerpf32(a.b, b.b, t)
  );
}
