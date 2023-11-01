// misc.c

inline f32 lerpf32(f32 a, f32 b, f32 t) {
  return (1.0f - t) * a + t * b;
}
