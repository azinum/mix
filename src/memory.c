// memory.c

#if defined(MEMORY_ALLOC_STATIC)
  #include "memory_static.c"
#else
  #include "memory_heap.c"
  #define MEMORY_TAG(name)
#endif

void memory_print_stats(i32 fd) {
  stb_dprintf(
    fd,
    "Memory statistics:\n"
    "  allocations: %zu, deallocations: %zu, total: %zu bytes (%.4f Kb)\n",
    memory_state.num_allocs,
    memory_state.num_deallocs,
    (size_t)memory_state.usage,
    (f32)memory_state.usage / 1024
  );
}
