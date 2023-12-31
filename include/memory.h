// memory.h

#ifndef _MEMORY_H
#define _MEMORY_H

struct {
  size_t num_allocs;
  size_t num_deallocs;
  ssize_t usage;
  ssize_t max_usage;
} memory_state = {0};

void memory_print_stats(i32 fd);

Result memory_init(void);
void memory_sweep_and_collect(void);
void* memory_alloc(size_t size);
void* memory_calloc(size_t n, size_t size);
void* memory_realloc(void* p, size_t size);
void memory_free(void* p);
void memory_print_info(i32 fd);
void memory_test(void);

#endif // _MEMORY_H
