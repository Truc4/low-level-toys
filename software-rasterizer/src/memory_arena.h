#ifndef ARENA_H
#define ARENA_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  uint8_t *base;
  size_t capacity;
  size_t used;
} MemoryArena;

// Initializes the arena with the given size (in bytes)
static inline void arena_init(MemoryArena *arena, size_t size) {
  uint8_t *temp = (uint8_t *)malloc(size);
  assert(temp != NULL);
  arena->base = temp;
  arena->capacity = size;
}

// Frees the memory associated with the arena
static inline void arena_free(MemoryArena *arena) {
  free(arena->base);
  arena->used = 0;
  arena->capacity = 0;
}

// Allocates a block of memory from the arena
static inline void *arena_alloc(MemoryArena *arena, size_t size) {
  assert(size + arena->used <= arena->capacity);
  void *ptr = arena->base + arena->used;
  arena->used += size;
  return ptr;
}

// Resets the arena to reuse all allocated memory
static inline void arena_reset(MemoryArena *arena) { arena->used = 0; }

#endif // ARENA_H
