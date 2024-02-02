// thread.c

inline void spin_wait(void) {
#ifdef USE_SIMD
  _mm_pause();
#else
  sleep(0);
#endif
}

inline u32 thread_get_id(void) {
  u32 thread_id = 0;
#if defined(__APPLE__) && defined(__x86_64__)
  asm("mov %%gs:0x00, %0" : "=r" (thread_id));
#elif defined(__i386__)
  asm("mov %%gs:0x08, %0" : "=r" (thread_id));
#elif defined(__x86_64__)
  asm("mov %%fs:0x10, %0" : "=r" (thread_id));
#else
  // #error "thread_get_id: unsupported architecture."
#endif
  return thread_id;
}

size_t atomic_fetch_add(volatile size_t* target, size_t value) {
  size_t result = __atomic_fetch_add(target, value, __ATOMIC_SEQ_CST); // Fetch value and then add
  return result;
}

size_t atomic_compare_exchange(volatile size_t* target, size_t value, size_t expected) {
  size_t result = __atomic_compare_exchange(target, &expected, &value, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  return result;
}

inline void ticket_mutex_begin(Ticket_mutex* mutex) {
  size_t ticket = atomic_fetch_add(&mutex->ticket, 1);
  while (ticket != mutex->serving) {
    spin_wait();
  };
}

inline void ticket_mutex_end(Ticket_mutex* mutex) {
  atomic_fetch_add(&mutex->serving, 1);
}
