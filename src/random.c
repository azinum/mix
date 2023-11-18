// random.c

static Random current_seed = 2147483647;

void random_init(Random seed) {
  current_seed = seed;
}

Random random_lc(void) {
  const Random a = 16807;
  const Random multiplier = 2147483647;
  const Random increment = 13;
  return (current_seed = (current_seed * a + increment) % multiplier);
}

Random random_xor_shift(void) {
  current_seed ^= current_seed << 13;
  current_seed ^= current_seed >> 17;
  current_seed ^= current_seed << 5;
  return current_seed;
}

Random random_number(void) {
  return random_lc();
}
