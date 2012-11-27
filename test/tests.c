// Tests for librope.

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "tests.h"
#include "slowstring.h"
#include "rope.h"

static float rand_float() {
  return (float)random() / INT32_MAX;
}

// A selection of different unicode characters to pick from.
// As far as I can tell, there are no unicode characters assigned which
// take up more than 4 bytes in utf-8.
static const char *UCHARS[] = {
  "a", "b", "c", "1", "2", "3", " ", "\n", // ASCII
  "©", "¥", "½", // The Latin-1 suppliment (U+80 - U+ff)
  "Ύ", "Δ", "δ", "Ϡ", // Greek (U+0370 - U+03FF)
  "←", "↯", "↻", "⇈", // Arrows (U+2190 – U+21FF)
  "𐆐", "𐆔", "𐆘", "𐆚", // Ancient roman symbols (U+10190 – U+101CF)
};

// s is the size of the buffer, including the \0. This function might use
// fewer bytes than that.
void random_unicode_string(uint8_t *buffer, size_t s) {
  if (s == 0) { return; }
  uint8_t *pos = buffer;
  
  while(1) {
    uint8_t *c = (uint8_t *)UCHARS[random() % (sizeof(UCHARS) / sizeof(UCHARS[0]))];
    
    size_t bytes = strlen((char *)c);
    
    size_t remaining_space = buffer + s - pos - 1;
    
    if (remaining_space < bytes) {
      break;
    }
    
    memcpy(pos, c, bytes);
    pos += bytes;
  }
  
  *pos = '\0';
}

static const char CHARS[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
"0123456789!@#$%^&*()[]{}<>?,./";
void random_ascii_string(uint8_t *buffer, size_t len) {
  assert(len);
  for (int i = 0; i < len - 1; i++) {
    buffer[i] = CHARS[random() % (sizeof(CHARS) - 1)];
  }
  buffer[len - 1] = '\0';
}

static size_t strlen_utf8(uint8_t *data) {
  size_t numchars = 0;
  
  while (*data) {
    if ((*data++ & 0xC0) != 0x80) {
      ++numchars;
    }
  }
  
  return numchars;
}

#if ROPE_WCHAR
// Count the number of wchars this string would take up if it was encoded using utf16.
static size_t wchar_size_count(uint8_t *data) {
  size_t num = 0;
  
  while (*data) {
    if ((*data & 0xC0) != 0x80) {
      ++num;
      if ((*data & 0xf0) == 0xf0) {
        // It'll take up 2 wchars, not just one.
        ++num;
      }
    }
    
    ++data;
  }
  
  return num;
}

static size_t count_wchars_in_utf8(const uint8_t *str, size_t num_chars) {
  size_t wchars = num_chars;
  while (num_chars) {
    if ((*str & 0xf0) == 0xf0) {
      wchars++;
    }
    if ((*str & 0xc0) != 0x80) {
      num_chars--;
    }
    ++str;
  }
  return wchars;
}
#endif

void test(int cond) {
  if (!cond) {
    fprintf(stderr, "Test failed\n");
    assert(0);
  }
}

void check(rope *rope, char *expected) {
  _rope_check(rope);
  test(rope_byte_count(rope) == strlen(expected));
  uint8_t *cstr = rope_create_cstr(rope);
  test(strcmp((char *)cstr, expected) == 0);
  free(cstr);
}

static void test_empty_rope_has_no_content() {
  rope *r = rope_new();
  check(r, "");
  test(rope_char_count(r) == 0);
  
  uint8_t *bytes = rope_create_cstr(r);
  test(bytes[0] == '\0');
  free(bytes);
  
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
  
  rope_insert(r, 2, (uint8_t *)"𝕐𝕆𝌀");
  check(r, "κό𝕐𝕆𝌀σμε");
  test(rope_char_count(r) == 8);
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

static void test_wchar() {
#if ROPE_WCHAR
  rope *r = rope_new_with_utf8((uint8_t *)"𐆔𐆚𐆔");
  test(rope_wchar_count(r) == 6);
  
  size_t len;
  size_t pos = rope_del_at_wchar(r, 2, 2, &len);
  check(r, "𐆔𐆔");
  test(pos == 1);
  test(len == 1);
  
  pos = rope_insert_at_wchar(r, 2, (uint8_t *)"abcde");
  check(r, "𐆔abcde𐆔");
  test(pos == 1);
  
  pos = rope_insert_at_wchar(r, 5, (uint8_t *)"𐆚");
  check(r, "𐆔abc𐆚de𐆔");
  test(pos == 4);
  
  rope_free(r);
#else
  printf("Skipping wchar tests - wchar conversion support disabled.\n");
#endif
}

static void test_really_long_ascii_string() {
  size_t len = 2000;
  uint8_t *str = malloc(len + 1);
  random_ascii_string(str, len + 1);
  
  rope *r = rope_new_with_utf8((uint8_t *)str);
  test(rope_char_count(r) == len);
  check(r, (char *)str);
  
  // Delete everything but the first and last characters.
  rope_del(r, 1, len - 2);
  assert(r->num_bytes == 2);
  assert(r->num_chars == 2);
  char *contents = (char *)rope_create_cstr(r);
  _rope_check(r);
  test(contents[0] == str[0]);
  test(contents[1] == str[len - 1]);
  free(contents);
  
  rope_free(r);
}

static int alloced_regions = 0;

void *_alloc(size_t size) {
  alloced_regions++;
  return malloc(size);
}

void _free(void *mem) {
  alloced_regions--;
  free(mem);
}

static void test_custom_allocator() {
  // Its really hard to test that malloc is never called, but I can make sure
  // custom frees match custom allocs.
  rope *r = rope_new2(_alloc, realloc, _free);
  for (int i = 0; i < 100; i++) {
    rope_insert(r, random() % (rope_char_count(r) + 1),
        (uint8_t *)"Whoa super happy fun times!\n");
  }

  rope_free(r);

  test(alloced_regions == 0);
}

static void test_copy() {
  // Copy an empty string.
  rope *r1 = rope_new();
  rope *r2 = rope_copy(r1);
  check(r2, "");
  rope_free(r2);
  
  // Insert some text (less than one node worth)
  rope_insert(r1, 0, (uint8_t *)"Eureka!");
  r2 = rope_copy(r1);
  check(r2, "Eureka!");
  
  rope_free(r1);
  rope_free(r2);
}

static void test_random_edits() {
  // This string should always have the same content as the rope.
  _string *str = str_create();
  rope *r = rope_new();
  
  const size_t max_stringsize = 1000;
  uint8_t strbuffer[max_stringsize + 1];
  
  for (int i = 0; i < 1000; i++) {
    // First, some sanity checks.
    check(r, (char *)str->mem);
    
    rope *r2 = rope_copy(r);
    check(r2, (char *)str->mem);
    rope_free(r2);
    
//    printf("String contains '%s'\n", str->mem);
    test(rope_byte_count(r) == str->len);
    size_t len = strlen_utf8(str->mem);
    test(rope_char_count(r) == len);
    test(str_num_chars(str) == len);
    
    if (len == 0 || rand_float() < 0.5f) {
      // Insert.
      random_unicode_string(strbuffer, 1 + random() % max_stringsize);
      size_t pos = random() % (len + 1);
      
//      printf("inserting %s at %zd\n", strbuffer, pos);
      rope_insert(r, pos, strbuffer);
      str_insert(str, pos, strbuffer);
    } else {
      // Delete
      size_t pos = random() % len;
      
      size_t dellen = random() % 10;
      dellen = MIN(len - pos, dellen);
      
//      printf("deleting %zd chars at %zd\n", dellen, pos);
      rope_del(r, pos, dellen);
      str_del(str, pos, dellen);
    }
  }
  
  rope_free(r);
  str_destroy(str);
}

static void test_random_wchar_edits() {
#if ROPE_WCHAR
  // This string should always have the same content as the rope.
  // Both are stored using UTF-8, but we'll make edits using the wchar functions.
  _string *str = str_create();
  rope *r = rope_new();
  
  const size_t max_stringsize = 1000;
  uint8_t strbuffer[max_stringsize + 1];
  
  for (int i = 0; i < 1000; i++) {
    check(r, (char *)str->mem);
    
//    printf("String contains '%s'\n", str->mem);
    test(rope_byte_count(r) == str->len);
    size_t len = strlen_utf8(str->mem);
    test(rope_char_count(r) == len);
    test(str_num_chars(str) == len);
    test(rope_wchar_count(r) == wchar_size_count(str->mem));
    
    if (len == 0 || rand_float() < 0.5f) {
      // Insert.
      random_unicode_string(strbuffer, 1 + random() % max_stringsize);
      size_t pos = random() % (len + 1);
      
      // We need to convert pos to the wchar offset. There's a private function in rope.c for this
      // but ...
      size_t wchar_pos = count_wchars_in_utf8(str->mem, pos);
      
//      printf("inserting '%s' at %zd\n", strbuffer, pos);
      rope_insert_at_wchar(r, wchar_pos, strbuffer);
      str_insert(str, pos, strbuffer);
    } else {
      // Delete
      size_t pos = random() % len;
      
      size_t dellen = random() % 10;
      dellen = MIN(len - pos, dellen);
      
      size_t wchar_pos = count_wchars_in_utf8(str->mem, pos);
      size_t wchar_len = count_wchars_in_utf8(str->mem, pos + dellen) - wchar_pos;
//      printf("deleting %zd (%zd) chars at %zd (%zd)\n", dellen, wchar_len, pos, wchar_pos);
      rope_del_at_wchar(r, wchar_pos, wchar_len, NULL);
      str_del(str, pos, dellen);
    }
  }
  
  rope_free(r);
  str_destroy(str);
#endif
}


void test_all() {
  printf("Running tests...\n");
  test_empty_rope_has_no_content();
  test_insert_at_location();
  test_new_string_has_content();
  test_delete_at_location();
  test_delete_past_end_of_string();
  test_wchar();
  test_really_long_ascii_string();
  test_custom_allocator();
  test_copy();
  printf("Normal tests passed. Running randomizers...\n");
  test_random_edits();
  test_random_wchar_edits();
  printf("Done!\n");
}

int main(int argc, const char * argv[]) {
  test_all();
  
  if (argc > 1 && strcmp(argv[1], "-b") == 0) {
    benchmark();
  }
  
  return 0;
}

