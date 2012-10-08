
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "rope.h"
#include "tests.h"

#include "slowstring.h"

#ifdef __cplusplus
#include <ext/rope>
#endif

// Wrapper for rope
static void *_rope_create() {
  return (void *)rope_new();
}

static void _rope_insert(void *r, size_t pos, const uint8_t *str) {
  rope_insert((rope *)r, pos, str);
}
static void _rope_del(void *r, size_t pos, size_t len) {
  rope_del((rope *)r, pos, len);
}
static void _rope_destroy(void *r) {
  rope_free((rope *)r);
}

static size_t _rope_num_chars(void *r) {
  return rope_char_count((rope *)r);
}

// Wrapper for a vector-based string

static void *_str_create() {
  return (void *)str_create();
}

static void _str_insert(void *r, size_t pos, const uint8_t *str) {
  str_insert((_string *)r, pos, str);
}

static void _str_del(void *r, size_t pos, size_t len) {
  str_del((_string *)r, pos, len);
}

static void _str_destroy(void *r) {
  str_destroy((_string *)r);
}

static size_t _str_num_chars(void *r) {
  return str_num_chars((_string *)r);
}

// SGI C++ rope. To enable these benchmarks, compile this file using a C++ compiler. There's a
// bug with some versions of clang and the rope library - you might have to switch to gcc.
#ifdef __cplusplus
static void *_sgi_create() {
  return new __gnu_cxx::crope();
}

static void _sgi_insert(void *r, size_t pos, const uint8_t *str) {
  __gnu_cxx::crope *rope = (__gnu_cxx::crope *)r;
  rope->insert(pos, (const char *)str);
}
static void _sgi_del(void *r, size_t pos, size_t len) {
  __gnu_cxx::crope *rope = (__gnu_cxx::crope *)r;
  rope->erase(pos, len);
}
static void _sgi_destroy(void *r) {
  __gnu_cxx::crope *rope = (__gnu_cxx::crope *)r;
  delete rope;
}

static size_t _sgi_num_chars(void *r) {
  __gnu_cxx::crope *rope = (__gnu_cxx::crope *)r;
  return rope->size();
}
#endif


struct rope_implementation {
  const char *name;
  void* (*create)();
  void (*insert)(void *r, size_t pos, const uint8_t *str);
  void (*del)(void *r, size_t pos, size_t len);
  void (*destroy)(void *r);
  size_t (*num_chars)(void *r);
} types[] = {
  { "librope", &_rope_create, &_rope_insert, &_rope_del, &_rope_destroy, &_rope_num_chars },
#ifdef __cplusplus
  { "sgirope", &_sgi_create, &_sgi_insert, &_sgi_del, &_sgi_destroy, &_sgi_num_chars },
#endif
  { "c string", &_str_create, &_str_insert, &_str_del, &_str_destroy, &_str_num_chars },
};

void benchmark() {
  printf("Benchmarking... (node size = %d, wchar support = %d)\n",
         ROPE_NODE_STR_SIZE, ROPE_WCHAR);
  
  long iterations = 20000000;
//  long iterations = 1000000;
  struct timeval start, end;

  // Make the test stable
  srandom(1234);
  
  uint8_t *strings[100];
  for (int i = 0; i < 100; i++) {
    size_t len = 1 + random() % 2;//i * i + 1;
    strings[i] = (uint8_t *)calloc(1, len + 1);
    random_ascii_string(strings[i], len + 1);
//    random_unicode_string(strings[i], len + 1);
  }
  
  // We should pick the same random sequence each benchmark run.
  unsigned long *rvals = (unsigned long *)malloc(sizeof(unsigned long) * iterations);
  for (int i = 0; i < iterations; i++) {
    rvals[i] = random();
  }

//  for (int t = 0; t < sizeof(types) / sizeof(types[0]); t++) {
  for (int t = 0; t < 1; t++) {
    for (int i = 0; i < 5; i++) {
      printf("benchmarking %s\n", types[t].name);
      void *r = types[t].create();

      gettimeofday(&start, NULL);
      
      for (long i = 0; i < iterations; i++) {
        if (types[t].num_chars(r) == 0 || i % 20 > 0) {
          // insert. (Inserts are way more common in practice than deletes.)
          uint8_t *str = strings[i % 100];
          types[t].insert(r, rvals[i] % (types[t].num_chars(r) + 1), str);
        } else {
          size_t pos = rvals[i] % types[t].num_chars(r);
          size_t length = MIN(types[t].num_chars(r) - pos, 1 + (~rvals[i]) % 53);
          types[t].del(r, pos, length);
        }
        
        //printf("%s\n", rope_createcstr(r, NULL));
      }
      
      gettimeofday(&end, NULL);

      double elapsedTime = end.tv_sec - start.tv_sec;
      elapsedTime += (end.tv_usec - start.tv_usec) / 1e6;
      printf("did %ld iterations in %f ms: %f Miter/sec\n",
             iterations, elapsedTime * 1000, iterations / elapsedTime / 1000000);
      printf("final string length: %zi\n", types[t].num_chars(r));
      
      types[t].destroy(r);
    }
  }
  
  for (int i = 0; i < 100; i++) {
    free(strings[i]);
  }
}

