//
//  afl.c
//  librope
//
//  Created by Joseph Gentle on 11/12/2014.
//  Copyright (c) 2014 Joseph Gentle. All rights reserved.
//

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "rope.h"

int main() {
  printf("AFL test harness\n");
  rope *r = rope_new();
  
  //FILE *stream = fopen("/Users/josephg/src/librope/death1", "r");
  FILE *stream = stdin;
  
  char *buffer = NULL;
  size_t buf_cap = 0;
  while (true) {
    // First read the position we're editing the rope
    ssize_t bytes_read = getline(&buffer, &buf_cap, stream);
    if (bytes_read == -1) break;
    
    int pos = atoi(buffer);
    int length = (int)rope_char_count(r);
    pos = pos < 0 ? 0 : pos > length ? length : pos;
    
    // Now read the characters to insert
    bytes_read = getline(&buffer, &buf_cap, stream);
    if (bytes_read == -1) break;

    if (bytes_read > 0 && buffer[0] == '-') {
      // Delete some characters
      int to_del = atoi(&buffer[1]);
      rope_del(r, pos, to_del);
    } else {
      // Delete the newline.
      if (bytes_read > 0) buffer[bytes_read - 1] = '\0';
      ROPE_RESULT result = rope_insert(r, pos, (uint8_t *)buffer);
      if (result == ROPE_INVALID_UTF8) {
        fprintf(stderr, "invalid utf8 - insert ignored\n");
      }
    }
  }
  
  _rope_check(r);
  printf("Final length: %zu\n", rope_char_count(r));
  rope_free(r);
}

