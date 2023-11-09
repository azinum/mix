// memory_static.c

#define BLOCK_HEADER_ALIGNMENT 16
// don't care to split a free block if the difference is less than this
#define FREE_BLOCK_DIFF_DONT_CARE BLOCK_HEADER_ALIGNMENT*2

#ifndef MEMORY_ALLOC_STATIC_SIZE
  #define MEMORY_ALLOC_STATIC_SIZE ALIGN(Mb(8), BLOCK_HEADER_ALIGNMENT)
#endif

#ifndef MEMORY_SWEEP_ON_FREE
  #define MEMORY_SWEEP_ON_FREE 1
#endif

typedef struct {
  u8 data[MEMORY_ALLOC_STATIC_SIZE];
  size_t size;
  size_t index;
} __attribute__((aligned(sizeof(size_t)))) Memory;

typedef enum {
  BLOCK_TAG_FREE = 0,
  BLOCK_TAG_USED,

  MAX_BLOCK_TAG,
} Block_tag;

typedef struct {
  Block_tag tag;
  size_t size;
} __attribute__((aligned(BLOCK_HEADER_ALIGNMENT))) Block_header;

static const char* block_tag_str[MAX_BLOCK_TAG] = { "free", "used", };

static Memory memory = {0};

static Result block_header_from_pointer(Memory* const m, void* p, Block_header** header);
static void* allocate_block(Memory* const m, const size_t size, Block_tag tag, Block_header** block_header);
static Result protected_write(Memory* const m, const size_t index, const void* data, const size_t size);
static void memory_sweep(Memory* const m);

Result block_header_from_pointer(Memory* const m, void* p, Block_header** header) {
  ASSERT(header != NULL);
  (void)m;
  *header = ((Block_header*)p) - 1;
  return Ok;
}

// linear search, but good enough for now
void* allocate_block(Memory* const m, const size_t size, Block_tag tag, Block_header** block_header) {
  const size_t aligned_size = ALIGN(size, BLOCK_HEADER_ALIGNMENT); // always align to block header size
  if (m->size < aligned_size) {
    return NULL;
  }
  for (m->index = 0; m->index < m->size; ) {
    Block_header* header = (Block_header*)&m->data[m->index];
    if (header->tag == BLOCK_TAG_FREE) {
      if (aligned_size <= header->size) {
        // success, found a block that we can use
        header->tag = tag;
        // calculate the size diff of the wanted size and the found free block size
        size_t size_diff = header->size - aligned_size;
        if (size_diff < FREE_BLOCK_DIFF_DONT_CARE) {
          // use the whole block even if the requested size was less than it
        }
        else {
          // use the aligned size
          header->size = aligned_size;
          // split the free block by allocating another block and set its' size to the diff (minus header size)
          Block_header* free_block = (Block_header*)&m->data[m->index + header->size + sizeof(Block_header)];
          free_block->tag = BLOCK_TAG_FREE;
          free_block->size = size_diff - sizeof(Block_header); // this should be aligned, TODO(lucas): check to be sure
        }
        // give back the allocated block
        if (block_header) {
          *block_header = header;
        }
        return header + 1;
      }
    }
    m->index += sizeof(Block_header) + ALIGN(header->size, BLOCK_HEADER_ALIGNMENT);
  }
  return NULL;
}

Result protected_write(Memory* const m, const size_t index, const void* data, const size_t size) {
  if (index + size < m->size) {
    memcpy(&m->data[index], data, size);
    return Ok;
  }
  return Error;
}

void memory_sweep(Memory* const m) {
  size_t size = 0;
  Block_header* head = NULL;
  for (size_t i = 0;;) {
    Block_header* header = (Block_header*)&m->data[i];
    if (header->tag == BLOCK_TAG_FREE) {
      size += header->size;
      if (head == NULL) {
        head = header;
      }
      if (head != header) {
        head->size = size + sizeof(Block_header);
        head = NULL;
        size = 0;
      }
    }
    else {
      head = NULL;
      size = 0;
    }
    i += header->size + sizeof(Block_header);
    break_if(i + sizeof(Block_header) >= m->size);
  }
}

Result memory_init(void) {
  memory_state.num_allocs = 0;
  memory_state.num_deallocs = 0;
  memory_state.usage = 0;
  memory_state.max_usage = sizeof(memory.data);
  memory.size = sizeof(memory.data);
  memory.index = 0;
  Block_header header = {
    .tag = BLOCK_TAG_FREE,
    .size = memory.size - sizeof(Block_header),
  };
  return protected_write(&memory, 0, &header, sizeof(header));
}

void* memory_alloc(const size_t size) {
#ifdef NO_MEMORY_TRACKING
  return allocate_block(&memory, size, BLOCK_TAG_USED, NULL);
#endif
  Block_header* header = NULL;
  void* p = allocate_block(&memory, size, BLOCK_TAG_USED, &header);
  if (!p) {
    return NULL;
  }
  memory_state.num_allocs += 1;
  memory_state.usage += header->size;
  return p;
}

void* memory_calloc(const size_t n, const size_t size) {
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
  Block_header* header = NULL;
  if (block_header_from_pointer(&memory, p, &header) != Ok) {
    return NULL;
  }
  ASSERT(header->tag == BLOCK_TAG_USED);
  void* new_ptr = memory_alloc(size);
  if (!new_ptr) {
    return NULL;
  }
  memcpy(new_ptr, p, header->size);
  header->tag = BLOCK_TAG_FREE;
#ifndef NO_MEMORY_TRACKING
  memory_state.usage -= header->size;
#endif
  return new_ptr;
}

void memory_free(void* p) {
  if (!p) {
    return;
  }
  Memory* m = &memory;
  Block_header* header = NULL;
  if (block_header_from_pointer(m, p, &header) != Ok) {
    return;
  }
  ASSERT(header->tag == BLOCK_TAG_USED);
  header->tag = BLOCK_TAG_FREE;
#ifndef NO_MEMORY_TRACKING
  memory_state.num_deallocs += 1;
  memory_state.usage -= header->size;
#endif
#if MEMORY_SWEEP_ON_FREE
  memory_sweep(m);
#else
  (void)memory_sweep;
#endif
}

void memory_print_info(i32 fd) {
  dprintf(fd, "Static memory: %zu/%zu bytes\n", (size_t)memory_state.usage, (size_t)MEMORY_ALLOC_STATIC_SIZE);
  const Memory* const m = &memory;
  for (size_t i = 0;;) {
    Block_header* header = (Block_header*)&m->data[i];
    size_t total_size = header->size + sizeof(Block_header);
#ifdef MEMORY_ALLOC_STATIC_PRINT_OVERHEAD
    dprintf(fd, "  block (%s) from %9lu to %9lu, %p, size: %9lu bytes, %10.4f Kb\n", block_tag_str[header->tag], i, i + header->size + sizeof(Block_header), (void*)header, total_size, (f32)total_size / Kb(1));
#else
    dprintf(fd, "  block (%s) from %9lu to %9lu, %p, size: %9lu bytes, %10.4f Kb\n", block_tag_str[header->tag], i, i + header->size, (void*)header, header->size, (f32)header->size / Kb(1));
#endif
    i += total_size;
    break_if(i + sizeof(Block_header) >= m->size);
  }
  memory_print_stats(fd);
}

void memory_test(void) {
  log_print(STDOUT_FILENO, LOG_TAG_INFO, "starting memory test...\n");
  Result result = Ok;

  const size_t alloc_size = 512 + sizeof(Block_header);
  const size_t num_allocs = MEMORY_ALLOC_STATIC_SIZE / alloc_size;
  ASSERT(!(alloc_size % BLOCK_HEADER_ALIGNMENT));
  for (size_t i = 0; i < num_allocs; ++i) {
    void* p = memory_alloc(alloc_size - sizeof(Block_header));
    (void)p;
  }
  Memory* m = &memory;
  for (size_t i = 0;;) {
    Block_header* header = (Block_header*)&m->data[i];
    if (!((header->tag == BLOCK_TAG_FREE) || (header->tag == BLOCK_TAG_USED))) {
      log_print(STDERR_FILENO, LOG_TAG_ERROR, "bad header tag %d at index %zu\n", i);
      result = Error;
      break;
    }
    i += header->size + sizeof(Block_header);
    break_if(i + sizeof(Block_header) >= m->size);
  }
  if (result == Ok) {
    log_print(STDOUT_FILENO, LOG_TAG_SUCCESS, "memory test success!\n");
  }
  else {
    log_print(STDERR_FILENO, LOG_TAG_ERROR, "memory test failed...\n");
  }
}
