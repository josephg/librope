/* UTF-8 Rope implementation by Joseph Gentle
 *
 * This library implements a heavyweight utf8 string type with fast
 * insert-at-position and delete-at-position operations.
 * 
 * It uses skip lists instead of trees. Trees might be faster - who knows?
 *
 * Ropes are not syncronized. Do not access the same rope from multiple threads
 * simultaneously.
 */

#ifndef librope_rope_h
#define librope_rope_h

#include <stdint.h>
#include <stddef.h>

// Whether or not the rope should support converting UTF-8 character offsets to
// wchar array positions. This is useful when interoperating with strings in
// JS, Objective-C and many other languages. See
// http://josephg.com/post/31707645955/string-length-lies
//
// Adding wchar conversion support decreases performance by about 30%.
#ifndef ROPE_WCHAR
#define ROPE_WCHAR 0
#endif

// These two magic values seem to be approximately optimal given the benchmark
// in tests.c which does lots of small inserts.

// Must be <= UINT16_MAX. Benchmarking says this is pretty close to optimal
// (tested on a mac using clang 4.0 and x86_64).
#ifndef ROPE_NODE_STR_SIZE
#if ROPE_WCHAR
#define ROPE_NODE_STR_SIZE 64
#else
#define ROPE_NODE_STR_SIZE 136
#endif
#endif

// The likelyhood (%) a node will have height (n+1) instead of n
#ifndef ROPE_BIAS
#define ROPE_BIAS 25
#endif

// The rope will stop being efficient after the string is 2 ^ ROPE_MAX_HEIGHT
// nodes.
#ifndef ROPE_MAX_HEIGHT
#define ROPE_MAX_HEIGHT 60
#endif

struct rope_node_t;

// The number of characters in str can be read out of nexts[0].skip_size.
typedef struct {
  // The number of _characters_ between the start of the current node
  // and the start of next.
  size_t skip_size;

  // For some reason, librope runs about 1% faster when this next pointer is
  // exactly _here_ in the struct.
  struct rope_node_t *node;

#if ROPE_WCHAR
  // The number of wide characters contained in space.
  size_t wchar_size;
#endif
} rope_skip_node;

typedef struct rope_node_t {
  uint8_t str[ROPE_NODE_STR_SIZE];

  // The number of bytes in str in use
  uint16_t num_bytes;
  
  // This is the number of elements allocated in nexts.
  // Each height is 1/2 as likely as the height before. The minimum height is 1.
  uint8_t height;
  
  rope_skip_node nexts[];
} rope_node;

typedef struct {
  // The total number of characters in the rope.
  size_t num_chars;
  
  // The total number of bytes which the characters in the rope take up.
  size_t num_bytes;
  
  void *(*alloc)(size_t bytes);
  void *(*realloc)(void *ptr, size_t newsize);
  void (*free)(void *ptr);

  // The first node exists inline in the rope structure itself.
  rope_node head;
} rope;

#ifdef __cplusplus
extern "C" {
#endif
  
// Create a new rope with no contents
rope *rope_new();

// Create a new rope using custom allocators.
rope *rope_new2(void *(*alloc)(size_t bytes),
    void *(*realloc)(void *ptr, size_t newsize),
    void (*free)(void *ptr));

// Create a new rope containing a copy of the given string. Shorthand for
// r = rope_new(); rope_insert(r, 0, str);
rope *rope_new_with_utf8(const uint8_t *str);

// Make a copy of an existing rope
rope *rope_copy(const rope *r);

// Free the specified rope
void rope_free(rope *r);

// Get the number of characters in a rope
size_t rope_char_count(const rope *r);

// Get the number of bytes which the rope would take up if stored as a utf8
// string
size_t rope_byte_count(const rope *r);

// Copies the rope's contents into a utf8 encoded C string. Also copies a
// trailing '\0' character.
// Returns the number of bytes written, which is rope_byte_count(r) + 1.
size_t rope_write_cstr(rope *r, uint8_t *dest);

// Create a new C string which contains the rope. The string will contain
// the rope encoded as utf8, followed by a trailing '\0'.
// Use rope_byte_count(r) to get the length of the returned string.
uint8_t *rope_create_cstr(rope *r);

// If you try to insert data into the rope with an invalid UTF8 encoding,
// nothing will happen and we'll return ROPE_INVALID_UTF8.
typedef enum { ROPE_OK, ROPE_INVALID_UTF8 } ROPE_RESULT;
  
// Insert the given utf8 string into the rope at the specified position.
ROPE_RESULT rope_insert(rope *r, size_t pos, const uint8_t *str);

// Delete num characters at position pos. Deleting past the end of the string
// has no effect.
void rope_del(rope *r, size_t pos, size_t num);
  
// This macro expands to a for() loop header which loops over the segments in a
// rope.
//
// Eg:
//  rope *r = rope_new_with_utf8(str);
//  ROPE_FOREACH(r, iter) {
//    printf("%s", rope_node_data(iter));
//  }
#define ROPE_FOREACH(rope, iter) \
  for (rope_node *iter = &(rope)->head; iter != NULL; iter = iter->nexts[0].node)

// Get the actual data inside a rope node.
static inline uint8_t *rope_node_data(rope_node *n) {
  return n->str;
}

// Get the number of bytes inside a rope node. This is useful when you're
// looping through a rope.
static inline size_t rope_node_num_bytes(rope_node *n) {
  return n->num_bytes;
}

// Get the number of characters inside a rope node.
static inline size_t rope_node_chars(rope_node *n) {
  return n->nexts[0].skip_size;
}
  
#if ROPE_WCHAR
// Get the number of wchar characters in the rope
size_t rope_wchar_count(rope *r);

// Insert the given utf8 string into the rope at the specified wchar position.
// This is compatible with NSString, Javascript, etc. The string still needs to
// be passed in using UTF-8.
//
// Returns the insertion position in characters.
size_t rope_insert_at_wchar(rope *r, size_t wchar_pos, const uint8_t *utf8_str);
  
// Delete wchar_num wide characters at the specified wchar position offset.
// If the range is inside character boundaries, behaviour is undefined.
//
// Returns the deletion position in characters. *char_len_out is set to the
// deletion length, in chars if its not null.
size_t rope_del_at_wchar(rope *r, size_t wchar_pos, size_t wchar_num, size_t *char_len_out);
  
// Get the number of wchars inside a rope node. This is useful when you're
// looping throuhg a rope.
static inline size_t rope_node_wchars(rope_node *n) {
  return n->nexts[0].wchar_size;
}
#endif


  
// For debugging.
void _rope_check(rope *r);
void _rope_print(rope *r);

#ifdef __cplusplus
}
#endif

#endif
