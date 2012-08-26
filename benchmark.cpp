
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "rope.h"
#include "tests.h"

// Wrapper for rope
static void *_rope_create() {
  return rope_new();
}

static void _rope_insert(void *r, size_t pos, uint8_t *str) {
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

// Private rope methods, stolen for utf8 support in the string.
static size_t codepoint_size(uint8_t byte) {
  if (byte <= 0x7f) { return 1; }
  else if (byte <= 0xdf) { return 2; }
  else if (byte <= 0xef) { return 3; }
  else if (byte <= 0xf7) { return 4; }
  else if (byte <= 0xfb) { return 5; }
  else if (byte <= 0xfd) { return 6; }
  else {
    // The codepoint is invalid... what do?
    //assert(0);
    return 1;
  }
}

// This little function counts how many bytes the some characters take up.
static size_t count_bytes_in_chars(const uint8_t *str, size_t num_chars) {
  const uint8_t *p = str;
  for (int i = 0; i < num_chars; i++) {
    p += codepoint_size(*p);
  }
  return p - str;
}

static size_t utf8_strlen(const uint8_t *str) {
  const uint8_t *p = str;
  while (*p) {
    p += codepoint_size(*p);
  }
  return p - str;
}

typedef struct {
  uint8_t *mem;
  size_t capacity;
  size_t len;
  size_t num_chars;
} _string;

static void *_str_create() {
  _string *s = malloc(sizeof(_string));
  s->capacity = 64; // A reasonable capacity considering...
  s->mem = malloc(s->capacity);
  s->len = 0;
  s->num_chars = 0;
  return s;
}

static void _str_insert(void *r, size_t pos, uint8_t *str) {
  _string *s = (_string *)r;
  
  size_t num_inserted_bytes = strlen((char *)str);
  // Offset to insert at in the string.
  size_t offset = count_bytes_in_chars(s->mem, pos);
  size_t end_size = s->len - offset;
  
  // Resize if needed.
  s->len += num_inserted_bytes;
  if (s->len > s->capacity) {
    while (s->len > s->capacity) {
      s->capacity *= 2;
    }
    s->mem = realloc(s->mem, s->capacity);
  }
  s->num_chars += utf8_strlen(str);
  
  memmove(&s->mem[offset + num_inserted_bytes], &s->mem[offset], end_size);
  memcpy(&s->mem[offset], str, num_inserted_bytes);
}

static void _str_del(void *r, size_t pos, size_t len) {
  _string *s = (_string *)r;
  
  // Offset to delete at in the string.
  size_t offset = count_bytes_in_chars(s->mem, pos);
  size_t num_bytes = count_bytes_in_chars(s->mem + offset, len);
  size_t end_size = s->len - offset - num_bytes;

  memmove(&s->mem[offset], &s->mem[offset + num_bytes], end_size);
  s->len -= num_bytes;
  s->num_chars -= len;
}

static void _str_destroy(void *r) {
  _string *s = (_string *)r;
  free(s->mem);
  free(s);
}

static size_t _str_num_chars(void *r) {
  _string *s = (_string *)r;
  return s->num_chars;
}

struct {
  char *name;
  void* (*create)();
  void (*insert)(void *r, size_t pos, uint8_t *str);
  void (*del)(void *r, size_t pos, size_t len);
  void (*destroy)();
  size_t (*num_chars)(void *r);
} types[] = {
  { "librope", &_rope_create, &_rope_insert, &_rope_del, &_rope_destroy, &_rope_num_chars },
  { "c string", &_str_create, &_str_insert, &_str_del, &_str_destroy, &_str_num_chars },
};

void benchmark() {
  printf("Benchmarking...\n");
  
  long iterations = 1000000;
  struct timeval start, end;
  
  uint8_t *strings[100];
  for (int i = 0; i < 100; i++) {
    size_t len = 1 + random() % 20;//i * i + 1;
    strings[i] = calloc(1, len + 1);
    random_ascii_string(strings[i], len + 1);
  }
  
  // We should pick the same random sequence each benchmark run.
  unsigned long *rvals = malloc(sizeof(long) * iterations);
  for (int i = 0; i < iterations; i++) {
    rvals[i] = random();
  }

  for (int t = 0; t < sizeof(types) / sizeof(types[0]); t++) {
    printf("benchmarking %s\n", types[t].name);
    void *r = types[t].create();
    gettimeofday(&start, NULL);
    
    for (long i = 0; i < iterations; i++) {
      if (types[t].num_chars(r) == 0 || i % 20 > 10) {
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
  
  for (int i = 0; i < 100; i++) {
    free(strings[i]);
  }
}

