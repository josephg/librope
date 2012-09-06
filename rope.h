/* UTF-8 Rope implementation by Joseph Gentle
 *
 * This library implements a heavyweight utf8 string type with fast
 * insert-at-position and delete-at-position operations.
 * 
 * It uses skip lists instead of trees. Trees might be faster - who knows?
 *
 * Ropes are NOT THREAD SAFE. Do not call multiple rope methods
 * simultaneously from different threads.
 */

#ifndef librope_rope_h
#define librope_rope_h

#include <stdint.h>
#include <stddef.h>

// These two magic values seem to be approximately optimal given the benchmark in
// tests.c which does lots of small inserts.

// Must be <= UINT16_MAX. Benchmarking says this is pretty close to optimal
// (tested on a mac using clang 4.0 and x86_64).
#define ROPE_NODE_STR_SIZE 138
// The likelyhood (%) a node will have height (n+1) instead of n
#define ROPE_BIAS 25

struct rope_node_t;

// The number of characters in str can be read out of nexts[0].skip_size.
typedef struct {
  // The number of _characters_ between the start of the current node
  // and the start of next.
  size_t skip_size;

  // For some reason, librope runs about 1% faster when this next pointer is
  // exactly _here_ in the struct.
  struct rope_node_t *node;

  size_t lines;
  size_t tombs;

} rope_next_node;

typedef struct {
  // The total number of characters in the list.
  size_t num_chars;
  
  // The total number of bytes used by the characters in the list.
  size_t num_bytes;
  
  // An array of the first nodes in the list at the given height.
  rope_next_node *heads;

  uint8_t height;
  uint8_t height_capacity;
  
  void *(*alloc)(size_t bytes);
  void *(*realloc)(void *ptr, size_t newsize);
  void (*free)(void *ptr);
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

// Create a new C string which contains the rope. The string will contain
// the rope encoded as utf-8.
// The length (in bytes) of the returned c string is returned via the len pointer.
// Use NULL if you're not interested in the length.
uint8_t *rope_createcstr(rope *r, size_t *len);

// Get the number of characters in a rope
size_t rope_char_count(rope *r);

// Get the number of bytes which the rope would take up if stored as a utf8
// string
size_t rope_byte_count(rope *r);


// Insert the given utf8 string into the rope at the specified position.
void rope_insert(rope *r, size_t pos, const uint8_t *str);

// Delete num characters at position pos. Deleting past the end of the string
// has no effect.
void rope_del(rope *r, size_t pos, size_t num);

// For debugging.
void _rope_check(rope *r);
void _rope_print(rope *r);

#ifdef __cplusplus
}
#endif

#endif
