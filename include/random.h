// random.h

#ifndef _RANDOM_H
#define _RANDOM_H

#ifndef Random
  typedef size_t Random;
#endif

void random_init(Random seed);
Random random_get_current_seed(void);
Random random_lc(void);
Random random_xor_shift(void);
Random random_number(void);
f32 random_f32(void);

#endif // _RANDOM_H
