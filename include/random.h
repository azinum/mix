// random.h

#ifndef _RANDOM_H
#define _RANDOM_H

#ifndef Random
  typedef size_t Random;
#endif

void random_init(Random seed);
Random random_lc(void);
Random random_xor_shift(void);
Random random_number(void);

#endif // _RANDOM_H
