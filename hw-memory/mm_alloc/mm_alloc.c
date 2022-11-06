/*
 * mm_alloc.c
 */

#include "mm_alloc.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

struct metadata {
  size_t size;
  bool free;
  struct metadata *prev;
  struct metadata *next;
};

size_t metadata_size = sizeof(struct metadata);
struct metadata *sentinel = NULL;

void* mm_malloc(size_t size) {
  //TODO: Implement malloc
  if (size == 0) return NULL;
  if (sentinel == NULL) {
    sentinel = sbrk(metadata_size);
    sentinel->size = 0;
    sentinel->free = false;
    sentinel->prev = NULL;
    sentinel->next = NULL;
  }
  // iterate through allocated linked list to find sufficiently large block of memory
  struct metadata *el = sentinel->next, *last = sentinel;
  for (; el != NULL; last = el, el = el->next) {
    if (el->free && el->size >= size) {
      if (el->size > metadata_size + size) {
        // split blocks
        void *el_mem_loc = (void*)el + metadata_size;
        void *new_metadata_loc = el_mem_loc + size;
        // void *new_mem_loc = new_metadata_loc + metadata_size;
        memset(el_mem_loc, 0, el->size);
        struct metadata *new = new_metadata_loc;
        new->size = el->size - size - metadata_size;
        new->free = true;
        new->prev = el;
        new->next = el->next;
        if (el->next) el->next->prev = new;
        el->size = size;
        el->free = false;
        el->next = new;
        return el_mem_loc;
      } else {
        el->free = false;
        void *mem_loc = (void*)el + metadata_size;
        memset(mem_loc, 0, el->size);
        return mem_loc;
      }
    }
  }
  // no large free block so create more space on heap
  struct metadata *heap = sbrk(size + metadata_size);
  if (heap == -1) return NULL;
  last->next = heap;
  heap->size = size;
  heap->free = false;
  heap->prev = last;
  heap->next = NULL;
  void *mem_loc = (void*)heap + metadata_size;
  memset(mem_loc, 0, size);
  return mem_loc;
}

void* mm_realloc(void* ptr, size_t size) {
  //TODO: Implement realloc
  if (size == 0) {
    mm_free(ptr);
    return NULL;
  }
  if (ptr == NULL) {
    return mm_malloc(size);
  }
  struct metadata *meta = ptr - metadata_size;
  if (size <= meta->size) {
    size_t old_size = meta->size;
    // meta->size = size;
    if (old_size > metadata_size + size) {
      // split blocks
      // void *el_mem_loc = (void*)el + metadata_size;
      void *new_metadata_loc = ptr + size;
      // void *new_mem_loc = new_metadata_loc + metadata_size;
      struct metadata *new = new_metadata_loc;
      new->size = old_size - size - metadata_size;
      new->free = true;
      new->prev = meta;
      new->next = meta->next;
      if (meta->next) meta->next->prev = new;
      meta->size = size;
      meta->free = false;
      meta->next = new;
      return ptr;
    } else {
      return ptr;
    }
  } else {
    size_t old_size = meta->size;
    char tmp[old_size];
    memcpy(tmp, ptr, old_size);
    mm_free(ptr);
    void *new = mm_malloc(size);
    if (new == NULL) {
      new = mm_malloc(old_size);
    }
    memcpy(new, tmp, old_size);
    return new;
  }
}

void mm_free(void* ptr) {
  //TODO: Implement free
  if (ptr == NULL) return;
  struct metadata *free_ptr = ptr - metadata_size;
  free_ptr->free = true;
  if (free_ptr->next != NULL && free_ptr->next->free == true) {
    // coalesce next block
    struct metadata *next_free = free_ptr->next;
    free_ptr->size += metadata_size + next_free->size;
    if (next_free->next) next_free->next->prev = free_ptr;
    free_ptr->next = next_free->next;
    memset((void*)free_ptr + metadata_size, 0, free_ptr->size);
  }
  if (free_ptr->prev != NULL && free_ptr->prev->free == true) {
    // coalesce prev block
    struct metadata *prev_free = free_ptr->prev;
    prev_free->size += metadata_size + free_ptr->size;
    prev_free->next = free_ptr->next;
    if (free_ptr->next) free_ptr->next->prev = prev_free;
    memset((void*)prev_free + metadata_size, 0, prev_free->size);
  }
}
