// This is a copy of the rope API using simple C strings.
//
// Its used for testing and benchmarking.

#ifndef librope_slowstring_h
#define librope_slowstring_h

#include <stdint.h>

typedef struct {
  uint8_t *mem;
  size_t capacity;
  size_t len;
  size_t num_chars;
} _string;

_string *str_create();

void str_insert(_string *s, size_t pos, const uint8_t *str);

void str_del(_string *s, size_t pos, size_t len);

void str_destroy(_string *s);

size_t str_num_chars(const _string *s);


#endif
