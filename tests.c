// Tests for librope.

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/time.h>

#include "rope.h"

static float rand_float() {
  return (float)random() / INT32_MAX;
}

// s is an approximate size. Might use fewer bytes than that.
static void random_unicode_string(uint8_t *buffer, size_t s) {
  uint8_t *pos = buffer;
  
  while(buffer - pos > 6) {
    uint8_t byte;
    do {
      byte = random() % 0xff;
    } while(byte == 0);
    
    int trailing_bytes;
    if (byte <= 0x7f) { trailing_bytes = 0; }
    else if (byte <= 0xdf) { trailing_bytes = 1; }
    else if (byte <= 0xef) { trailing_bytes = 2; }
    else if (byte <= 0xf7) { trailing_bytes = 3; }
    else if (byte <= 0xfb) { trailing_bytes = 4; }
    else if (byte <= 0xfd) { trailing_bytes = 5; }
    else {
      // 0xfd is the highest valid first byte in a utf8 string.
      // I'll just map it back to an 'a'.
      byte = 'a';
      trailing_bytes = 0;
    }
    
    *pos++ = byte;
    for (int i = 0; i < trailing_bytes; i++) {
      *pos++ = (random() % 0x3f) | 0x80;
    }
  }
}

static const char CHARS[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
"0123456789!@#$%^&*()[]{}<>?,./";
static void random_ascii_string(uint8_t *buffer, size_t len) {
  assert(len);
  for (int i = 0; i < len - 1; i++) {
    buffer[i] = CHARS[random() % (sizeof(CHARS) - 1)];
  }
  buffer[len - 1] = '\0';
}

size_t utf8_strlen(uint8_t *data) {
  size_t numchars = 0;
  
  while (*data) {
    if ((*data++ & 0xC0) != 0x80) {
      ++numchars;
    }
  }
  
  return numchars;
}

void test(int cond) {
  if (!cond) {
    fprintf(stderr, "Test failed\n");
    assert(0);
  }
}

void check(rope *rope, char *expected) {
  test(rope_byte_count(rope) == strlen(expected));
  uint8_t *cstr = rope_createcstr(rope, NULL);
  test(strcmp((char *)cstr, expected) == 0);
  free(cstr);
}

static void test_empty_rope_has_no_content() {
  rope *r = rope_new();
  check(r, "");
  test(rope_char_count(r) == 0);
  rope_free(r);
}

// A rope initialized with a string has that string as its content
static void test_new_string_has_content() {
  rope *r = rope_new_with_utf8((uint8_t *)"Hi there");
  check(r, "Hi there");
  test(rope_char_count(r) == strlen("Hi there"));
  rope_free(r);
  
  // If need be, this could be rewritten as an array of bytes...
  r = rope_new_with_utf8((uint8_t *)"κόσμε");
  check(r, "κόσμε");
  test(rope_char_count(r) == 5);
  rope_free(r);
}

static void test_insert_at_location() {
  rope *r = rope_new();

  rope_insert(r, 0, (uint8_t *)"AAA");
  check(r, "AAA");

  rope_insert(r, 0, (uint8_t *)"BBB");
  check(r, "BBBAAA");

  rope_insert(r, 6, (uint8_t *)"CCC");
  check(r, "BBBAAACCC");

  rope_insert(r, 5, (uint8_t *)"DDD");
  check(r, "BBBAADDDACCC");

  test(rope_char_count(r) == 12);

  rope_free(r);
}

static void test_delete_at_location() {
  rope *r = rope_new_with_utf8((uint8_t *)"012345678");
  
  rope_del(r, 8, 1);
  check(r, "01234567");
  
  rope_del(r, 0, 1);
  check(r, "1234567");
  
  rope_del(r, 5, 1);
  check(r, "123457");
  
  rope_del(r, 5, 1);
  check(r, "12345");
  
  rope_del(r, 0, 5);
  check(r, "");
  
  test(rope_char_count(r) == 0);
  
  rope_free(r);
}

static void test_delete_past_end_of_string() {
  rope *r = rope_new();
  
  rope_del(r, 0, 100);
  check(r, "");
  
  rope_insert(r, 0, (uint8_t *)"hi there");
  rope_del(r, 3, 10);
  check(r, "hi ");
  
  test(rope_char_count(r) == 3);
  
  rope_free(r);
}

static void test_really_long_ascii_string() {
  size_t len = 1000000;
  uint8_t *str = malloc(len + 1);
  random_ascii_string(str, len + 1);
  
  rope *r = rope_new_with_utf8((uint8_t *)str);
  test(rope_char_count(r) == len);
  check(r, (char *)str);
  
  // Delete everything but the first and last characters.
  rope_del(r, 1, len - 2);
  char *contents = (char *)rope_createcstr(r, NULL);
  test(contents[0] == str[0]);
  test(contents[1] == str[len - 1]);
  free(contents);
  
  rope_free(r);
}

// TODO: Should add a test for really long unicode strings as well

#define MIN(x,y) ((x) > (y) ? (y) : (x))

#ifndef __APPLE__
#warning Not running random edits test
static void test_random_edits() {}
#else
#include <CoreFoundation/CoreFoundation.h>
static void test_random_edits() {
  // This string should always have the same content as the rope.
  CFMutableStringRef str = CFStringCreateMutable(NULL, 0);
  rope *r = rope_new();
  
  const size_t bufsize = 100000000;
  char *buffer = malloc(bufsize);
  
  const size_t max_stringsize = 1000;
  uint8_t strbuffer[max_stringsize];
  
  for (int i = 0; i < 10000; i++) {
    // First, some sanity checks.
    CFStringGetCString(str, buffer, bufsize, kCFStringEncodingUTF8);
    check(r, buffer);
    test(rope_byte_count(r) == strlen(buffer));
    size_t len = utf8_strlen((uint8_t *)buffer);
    test(rope_char_count(r) == len);
    
    if (len == 0 || rand_float() < 0.5f) {
      // Insert.
      //uint8_t *text = random_ascii_string(11);
      random_unicode_string(strbuffer, random() % max_stringsize);
      size_t pos = random() % (len + 1);
      
      //printf("inserting %s at %zd\n", text, pos);
      rope_insert(r, pos, strbuffer);
      
      CFStringRef ins = CFStringCreateWithCString(NULL, (char *)strbuffer, kCFStringEncodingUTF8);
      CFStringInsert(str, pos, ins);
      
      CFRelease(ins);
    } else {
      // Delete
      size_t pos = random() % len;
      
      size_t dellen = random() % 10;
      dellen = MIN(len - pos, dellen);
      
      //printf("deleting %zd chars at %zd\n", dellen, pos);

      //deletedText = str[pos...pos + length]
      //test.strictEqual deletedText, r.substring pos, length

      rope_del(r, pos, dellen);
      
      CFStringDelete(str, CFRangeMake(pos, dellen));
    }
  }
  
  free(buffer);
}
#endif

void test_all() {
  test_empty_rope_has_no_content();
  test_new_string_has_content();
  test_insert_at_location();
  test_delete_at_location();
  test_delete_past_end_of_string();
  test_really_long_ascii_string();
  test_random_edits();
}

void benchmark() {
  long iterations = 1000;
  struct timeval start, end;
  
  rope *r = rope_new();
  uint8_t *strings[100];
  for (int i = 0; i < 100; i++) {
    size_t len = i * i + 1;
    strings[i] = calloc(1, len + 1);
    random_ascii_string(strings[i], len);
  }
  
  gettimeofday(&start, NULL);
  
  for (long i = 0; i < iterations; i++) {
    if (rand_float() < 0.95f) {
      // insert. (Inserts are way more common in practice than deletes.)
      uint8_t *str = strings[random() % 100];
      rope_insert(r, random() % (r->num_chars + 1), str);
    } else {
      size_t pos = random() % r->num_chars;
      size_t length = MIN(r->num_chars - pos, 1 + random() % 1000);
      rope_del(r, pos, length);
    }
  }
  
  gettimeofday(&end, NULL);
  
  for (int i = 0; i < 100; i++) {
    free(strings[i]);
  }
  double elapsedTime = end.tv_sec - start.tv_sec;
  elapsedTime += (end.tv_usec - start.tv_usec) / 1e6;
  printf("did %ld iterations in %f ms: %e iter/sec\n",
         iterations, elapsedTime * 1000, iterations / elapsedTime);
  printf("final string length: %zi", r->num_chars);
  rope_free(r);
}


int main(int argc, const char * argv[]) {
  printf("Running tests...\n");
  test_all();
  printf("Done!\n");
  
  benchmark();
  return 0;
}

