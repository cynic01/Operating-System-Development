#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

/* Function pointers to hw3 functions */
void* (*mm_malloc)(size_t);
void* (*mm_realloc)(void*, size_t);
void (*mm_free)(void*);

static void* try_dlsym(void* handle, const char* symbol) {
  char* error;
  void* function = dlsym(handle, symbol);
  if ((error = dlerror())) {
    fprintf(stderr, "%s\n", error);
    exit(EXIT_FAILURE);
  }
  return function;
}

static void load_alloc_functions() {
  void* handle = dlopen("hw3lib.so", RTLD_NOW);
  if (!handle) {
    fprintf(stderr, "%s\n", dlerror());
    exit(EXIT_FAILURE);
  }

  mm_malloc = try_dlsym(handle, "mm_malloc");
  mm_realloc = try_dlsym(handle, "mm_realloc");
  mm_free = try_dlsym(handle, "mm_free");
}

int main() {
  load_alloc_functions();

  // int* data = mm_malloc(sizeof(int));
  // assert(data != NULL);
  // data[0] = 0x162;
  // mm_free(data);
  // puts("malloc test successful!");

  // char *data0 = mm_malloc(sizeof(char));
  // short *data1 = mm_malloc(sizeof(short));
  // int *data2 = mm_malloc(sizeof(int));
  // long long *data3 = mm_malloc(sizeof(long long));
  // assert(data0 != NULL);
  // assert(data1 != NULL);
  // assert(data2 != NULL);
  // assert(data3 != NULL);
  // *data0 = 45;
  // *data1 = 123;
  // *data2 = 789;
  // *data3 = 123456789;
  // mm_free(data3);
  // mm_free(data2);
  // mm_free(data1);
  // mm_free(data0);
  // int *data_big = mm_malloc(15);
  // assert(data_big != NULL);
  // assert(data0 == data_big);
  // mm_free(data_big);
  // puts("coalesce test successful!");

  int *p = mm_malloc(10 * sizeof(int));
  int *q = mm_malloc(10 * sizeof(int));
  assert(p != NULL && q != NULL);
  int *old_p = p;
  p = mm_realloc(p, 20 * sizeof(int));
  assert(p != NULL);
  assert(p != old_p);
  int *s = mm_malloc(10 * sizeof(int));
  assert(s == old_p);
  int *p_null = mm_realloc(p, 0xffffffff);
  assert(p_null == NULL);
  mm_free(p);
  mm_free(q);
  mm_free(s);
  puts("realloc test successful!");
}
