//
//  slowstring.c
//  librope
//
//  Created by Joseph Gentle on 28/08/12.
//  Copyright (c) 2012 Joseph Gentle. All rights reserved.
//

#include <stdlib.h>
#include <string.h>

#include "slowstring.h"

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

static size_t strlen_utf8(const uint8_t *str) {
  const uint8_t *p = str;
  size_t i = 0;
  while (*p) {
    p += codepoint_size(*p);
    i++;
  }
  return i;
}

_string *str_create() {
  _string *s = (_string *)malloc(sizeof(_string));
  s->capacity = 64; // A reasonable capacity considering...
  s->mem = (uint8_t *)malloc(s->capacity);
  s->mem[0] = '\0';
  s->len = 0;
  s->num_chars = 0;
  return s;
}

void str_insert(_string *s, size_t pos, const uint8_t *str) {
  size_t num_inserted_bytes = strlen((char *)str);
  // Offset to insert at in the string.
  size_t offset = count_bytes_in_chars(s->mem, pos);
  size_t end_size = s->len - offset;
  
  // Resize if needed.
  s->len += num_inserted_bytes;
  if (s->len >= s->capacity) {
    while (s->len >= s->capacity) {
      s->capacity *= 2;
    }
    s->mem = (uint8_t *)realloc(s->mem, s->capacity);
  }
  s->num_chars += strlen_utf8(str);
  
  memmove(&s->mem[offset + num_inserted_bytes], &s->mem[offset], end_size);
  memcpy(&s->mem[offset], str, num_inserted_bytes);
  s->mem[s->len] = '\0';
}

void str_del(_string *s, size_t pos, size_t len) {
  // Offset to delete at in the string.
  size_t offset = count_bytes_in_chars(s->mem, pos);
  size_t num_bytes = count_bytes_in_chars(s->mem + offset, len);
  size_t end_size = s->len - offset - num_bytes;
  
  if (end_size > 0) {
    memmove(&s->mem[offset], &s->mem[offset + num_bytes], end_size);
  }
  s->len -= num_bytes;
  s->num_chars -= len;
  s->mem[s->len] = '\0';
}

void str_destroy(_string *s) {
  free(s->mem);
  free(s);
}

size_t str_num_chars(const _string *s) {
  return s->num_chars;
}
