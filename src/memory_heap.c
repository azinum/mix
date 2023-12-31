// memory_heap.c

typedef size_t Alloc_header;

Result memory_init(void) {
  memory_state.num_allocs = 0;
  memory_state.num_deallocs = 0;
  memory_state.usage = 0;
  memory_state.max_usage = Gb(1);
  return Ok;
}

void memory_sweep_and_collect(void) {
  // nothing to do
}

void* memory_alloc(size_t size) {
#ifdef NO_MEMORY_TRACKING
  return malloc(size);
#endif
  void* p = malloc(size + sizeof(Alloc_header));
  if (!p) {
    return NULL;
  }
  *(Alloc_header*)p = size;
  memory_state.num_allocs += 1;
  memory_state.usage += size;
  return (u8*)p + sizeof(Alloc_header);
}

void* memory_calloc(size_t n, size_t size) {
  void* p = memory_alloc(n * size);
  if (!p) {
    return NULL;
  }
  memset(p, 0, n * size);
  return p;
}

void* memory_realloc(void* p, size_t size) {
  if (!p) {
    return memory_alloc(size);
  }
#ifdef NO_MEMORY_TRACKING
  return realloc(p, size);
#else
  Alloc_header old_size = *(Alloc_header*)((u8*)p - sizeof(Alloc_header));
  void* new_ptr = realloc((u8*)p - sizeof(Alloc_header), size + sizeof(Alloc_header));
  if (!new_ptr) {
    return NULL;
  }
  *(Alloc_header*)new_ptr = size;
  memory_state.usage += size;
  memory_state.usage -= old_size;
  return (u8*)new_ptr + sizeof(Alloc_header);
#endif
}

void memory_free(void* p) {
  if (!p) {
    return;
  }
#ifdef NO_MEMORY_TRACKING
  free(p);
  return;
#endif
  Alloc_header size = *(Alloc_header*)((u8*)p - sizeof(Alloc_header));
  memory_state.num_deallocs += 1;
  memory_state.usage -= size;
  free((u8*)p - sizeof(Alloc_header));
}

void memory_print_info(i32 fd) {
  memory_print_stats(fd);
}

void memory_test(void) {

}
