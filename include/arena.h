// arena.h

#ifndef _ARENA_H
#define _ARENA_H

typedef struct Arena {
  u8* data;
  size_t index;
  size_t size;
} Arena;

Arena arena_new(const size_t size);
void* arena_alloc(Arena* arena, const size_t size);
void arena_reset(Arena* arena);
void arena_free(Arena* arena);

#endif // _ARENA_H

#ifdef ARENA_IMPLEMENTATION

Arena arena_new(const size_t size) {
  Arena arena = {
    .data = memory_alloc(size),
    .index = 0,
    .size = size,
  };
  ASSERT(arena.data != NULL && "out of memory");
  return arena;
}

void* arena_alloc(Arena* arena, const size_t size) {
  ASSERT(arena != NULL);
  if (arena->index + size <= arena->size) {
    void* p = (void*)&arena->data[arena->index];
    arena->index += size;
    return p;
  }
  return NULL;
}

void arena_reset(Arena* arena) {
  arena->index = 0;
}

void arena_free(Arena* arena) {
  ASSERT(arena != NULL);
  memory_free(arena->data);
  arena->index = 0;
  arena->size = 0;
}

#endif // ARENA_IMPLEMENTATION
