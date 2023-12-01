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
  #error "thread_get_id: unsupported architecture."
#endif
  return thread_id;
}

