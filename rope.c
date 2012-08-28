// Implementation for rope library.

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "rope.h"

typedef struct rope_node_t {
  uint8_t str[ROPE_NODE_STR_SIZE];

  // The number of bytes in str in use
  uint16_t num_bytes;
  
  // This is the number of elements allocated in nexts.
  // Each height is 1/2 as likely as the height before. The minimum height is 1.
  uint8_t height;
  
  // unused for now - should be useful for object pools.
  //uint8_t height_capacity;
  
  rope_next_node nexts[0];
} rope_node;

// Create a new rope with no contents
rope *rope_new() {
  rope *r = (rope *)calloc(1, sizeof(rope));
  r->height = 0;
  r->height_capacity = 10;
  r->heads = (rope_next_node *)malloc(sizeof(rope_next_node) * 10);
  return r;
}

// Create a new rope with no contents
rope *rope_new_with_utf8(const uint8_t *str) {
  rope *r = rope_new();
  rope_insert(r, 0, str);
  return r;
}

// Free the specified rope
void rope_free(rope *r) {
  assert(r);
  rope_node *next;
  
  if (r->height > 0) {
    for (rope_node *n = r->heads[0].node; n != NULL; n = next) {
      next = n->nexts[0].node;
      free(n);
    }
  }
  
  free(r->heads);
  free(r);
}

// Create a new C string which contains the rope. The string will contain
// the rope encoded as utf-8.
uint8_t *rope_createcstr(rope *r, size_t *len) {
  size_t numbytes = rope_byte_count(r);
  uint8_t *bytes = (uint8_t *)malloc(numbytes + 1); // Room for a zero.
  bytes[numbytes] = '\0';
  
  if (numbytes == 0) {
    return bytes;
  }
  
  uint8_t *p = bytes;
  for (rope_node *n = r->heads[0].node; n != NULL; n = n->nexts[0].node) {
    memcpy(p, n->str, n->num_bytes);
    p += n->num_bytes;
  }
  
  assert(p == &bytes[numbytes]);
  
  if (len) {
    *len = numbytes;
  }
  
  return bytes;
}

// Get the number of characters in a rope
size_t rope_char_count(rope *r) {
  assert(r);
  return r->num_chars;
}

// Get the number of bytes which the rope would take up if stored as a utf8
// string
size_t rope_byte_count(rope *r) {
  assert(r);
  return r->num_bytes;
}

#define MIN(x,y) ((x) > (y) ? (y) : (x))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

static uint8_t random_height() {
  // This function is horribly inefficient. I'm throwing away heaps of entropy, and
  // the mod could be replaced by some clever shifting.
  //
  // However, random_height barely appears in the profiler output - so its probably
  // not worth investing the time to optimise.

  uint8_t height = 1;
  
  while(height < UINT8_MAX && (random() % 100) < ROPE_BIAS) {
    height++;
  }
  
  return height;
}

// Figure out how many bytes to allocate for a node with the specified height.
static size_t node_size(uint8_t height) {
  return sizeof(rope_node) + height * sizeof(rope_next_node);
}

// Allocate and return a new node. The new node will be full of junk, except
// for its height.
// This function should be replaced at some point with an object pool based version.
static rope_node *alloc_node(uint8_t height) {
  rope_node *node = (rope_node *)malloc(node_size(height));
  node->height = height;
  return node;
}

// Find out how many bytes the unicode character which starts with the specified byte
// will occupy in memory.
// Returns the number of bytes, or SIZE_MAX if the byte is invalid.
static inline size_t codepoint_size(uint8_t byte) {
  if (byte <= 0x7f) { return 1; } // 0x74 = 0111 1111
  else if (byte <= 0xbf) { return SIZE_MAX; } // 1011 1111. Invalid for a starting byte.
  else if (byte <= 0xdf) { return 2; } // 1101 1111
  else if (byte <= 0xef) { return 3; } // 1110 1111
  else if (byte <= 0xf7) { return 4; } // 1111 0111
  else if (byte <= 0xfb) { return 5; } // 1111 1011
  else if (byte <= 0xfd) { return 6; } // 1111 1101
  else { return SIZE_MAX; }
}

// This little function counts how many bytes a certain number of characters take up.
static size_t count_bytes_in_chars(const uint8_t *str, size_t num_chars) {
  const uint8_t *p = str;
  for (unsigned int i = 0; i < num_chars; i++) {
    p += codepoint_size(*p);
  }
  return p - str;
}

// Count the number of characters in a string.
static size_t utf8_strlen(const uint8_t *str) {
  const uint8_t *p = str;
  size_t i = 0;
  while (*p) {
    p += codepoint_size(*p);
    i++;
  }
  return i;
}

// Internal function for navigating to a particular character offset in the rope.
// The function returns the list of nodes which point past the position, as well as
// offsets of how far into their character lists the specified characters are.
static rope_node *go_to_node(rope *r, size_t pos, size_t *offset_out, rope_node *nodes[], size_t *tree_offsets) {
  rope_node *e = NULL;
  
  int height = r->height;
  // Offset stores how characters we still need to skip in the current node.
  size_t offset = pos;
  size_t skip;

  while (height--) {
    if (offset > (skip = r->heads[height].skip_size)) {

      offset -= skip;
      e = r->heads[height].node;

      break;
    } else {
      if (tree_offsets) {
        tree_offsets[height] = offset;
      }
      nodes[height] = NULL;
    }
  }
  
  // The list is empty or offset is 0.
  if (e == NULL) {
    *offset_out = offset;
    return e;
  }

  while (true) {
    skip = e->nexts[height].skip_size;
    if (offset > skip) {
      // Go right.
      offset -= skip;
      assert(e->num_bytes);
      e = e->nexts[height].node;
    } else {
      // Go down.
      if (tree_offsets) {
        tree_offsets[height] = offset;
      }
      nodes[height] = e;

      if (height == 0) {
        break;
      } else {
        height--;
      }
    }
  }
  
  assert(offset <= ROPE_NODE_STR_SIZE);
  
  *offset_out = offset;
  return e;
}

static void update_offset_list(rope *r, rope_node *nodes[], size_t amt) {
  for (int i = 0; i < r->height; i++) {
    if (nodes[i]) {
      nodes[i]->nexts[i].skip_size += amt;
    } else {
      r->heads[i].skip_size += amt;
    }
  }
}

// Internal method of rope_insert.
static void insert_at(rope *r, size_t pos, const uint8_t *str,
    size_t num_bytes, size_t num_chars, rope_node *nodes[], size_t tree_offsets[]) {
  // This describes how many of the nodes[] and tree_offsets[] arrays are filled in.
  uint8_t max_height = r->height;
  uint8_t new_height = random_height();
  rope_node *new_node = alloc_node(new_height);
  new_node->num_bytes = num_bytes;
  memcpy(new_node->str, str, num_bytes);
  
  // Ensure the rope has enough capacity to store the next pointers to the new object.
  if (new_height > max_height) {
    r->height = new_height;
    if (r->height > r->height_capacity) {
      do {
        r->height_capacity *= 2;
      } while (r->height_capacity < r->height);
      r->heads = (rope_next_node *)realloc(r->heads, sizeof(rope_next_node) * r->height_capacity);
    }
  }

  // Fill in the new node's nexts array.
  int i;
  for (i = 0; i < new_height; i++) {
    if (i < max_height) {
      rope_next_node *prev_node = (nodes[i] ? &nodes[i]->nexts[i] : &r->heads[i]);
      new_node->nexts[i].node = prev_node->node;
      new_node->nexts[i].skip_size = num_chars + prev_node->skip_size - tree_offsets[i];

      prev_node->node = new_node;
      prev_node->skip_size = tree_offsets[i];
    } else {
      // Edit the head node instead of editing the parent listed in nodes.
      new_node->nexts[i].node = NULL;
      new_node->nexts[i].skip_size = r->num_chars - pos + num_chars;
      
      r->heads[i].node = new_node;
      r->heads[i].skip_size = pos;
    }
    
    nodes[i] = new_node;
    tree_offsets[i] = num_chars;
  }
  
  for (; i < max_height; i++) {
    if (nodes[i]) {
      nodes[i]->nexts[i].skip_size += num_chars;
    } else {
      r->heads[i].skip_size += num_chars;
    }
    tree_offsets[i] += num_chars;
  }
  
  r->num_chars += num_chars;
  r->num_bytes += num_bytes;
}

// Insert the given utf8 string into the rope at the specified position.
void rope_insert(rope *r, size_t pos, const uint8_t *str) {
  assert(r);
  assert(str);
  pos = MIN(pos, r->num_chars);

  // There's a good chance we'll have to rewrite a bunch of next pointers and a bunch
  // of offsets. This variable will store pointers to the elements which need to
  // be changed.
  rope_node *nodes[UINT8_MAX];
  size_t tree_offsets[UINT8_MAX];

  // This is the number of characters to skip in the current node.
  size_t offset;
  
  // First we need to search for the node where we'll insert the string.
  rope_node *e = go_to_node(r, pos, &offset, nodes, tree_offsets);
  
  // offset contains how far (in characters) into the current element to skip.
  // Figure out how much that is in bytes.
  size_t offset_bytes = 0;
  if (e && offset) {
    assert(offset <= e->num_bytes);
    offset_bytes = count_bytes_in_chars(e->str, offset);
  }
  
  // Maybe we can insert the characters into the current node?
  size_t num_inserted_bytes = strlen((char *)str);

  // Can we insert into the current node?
  bool insert_here = e && e->num_bytes + num_inserted_bytes <= ROPE_NODE_STR_SIZE;
  
  // Can we insert into the subsequent node?
  bool insert_next = false;
  rope_node *next = NULL;
  if (!insert_here) {
    next = e ? e->nexts[0].node : (r->num_chars ? r->heads[0].node : NULL);
    // We can insert into the subsequent node if:
    // - We can't insert into the current node
    // - There _is_ a next node to insert into
    // - The insert would be at the start of the next node
    // - There's room in the next node
    insert_next = next
        && (e == NULL || offset_bytes == e->num_bytes)
        && next->num_bytes + num_inserted_bytes <= ROPE_NODE_STR_SIZE;
  }
  
  if (insert_here || insert_next) {
    if (insert_next) {
      offset = offset_bytes = 0;
      for (int i = 0; i < next->height; i++) {
        nodes[i] = next;
        // tree offset nodes not used.
      }
      e = next;
    }
    
    // First move the current bytes later on in the string.
    if (offset_bytes < e->num_bytes) {
      memmove(&e->str[offset_bytes + num_inserted_bytes],
              &e->str[offset_bytes],
              e->num_bytes - offset_bytes);
    }
    
    // Then copy in the string bytes
    memcpy(&e->str[offset_bytes], str, num_inserted_bytes);
    e->num_bytes += num_inserted_bytes;
    
    r->num_bytes += num_inserted_bytes;
    size_t num_inserted_chars = utf8_strlen(str);
    r->num_chars += num_inserted_chars;
    
    // .... aaaand update all the offset amounts.
    update_offset_list(r, nodes, num_inserted_chars);
  } else {
    // There isn't room. We'll need to add at least one new node to the rope.
    
    // If we're not at the end of the current node, we'll need to remove
    // the end of the current node's data and reinsert it later.
    size_t num_end_bytes = 0, num_end_chars;
    if (e) {
      num_end_bytes = e->num_bytes - offset_bytes;
      e->num_bytes = offset_bytes;
      if (num_end_bytes) {
        // Count out characters.
        num_end_chars = e->nexts[0].skip_size - offset;
        update_offset_list(r, nodes, -num_end_chars);
        
        r->num_chars -= num_end_chars;
        r->num_bytes -= num_end_bytes;
      }
    }
    
    // Now, we insert new node[s] containing the data. The data must
    // be broken into pieces of with a maximum size of ROPE_NODE_STR_SIZE.
    // Node boundaries do not occur in the middle of a utf8 codepoint.
    size_t str_offset = 0;
    while (str_offset < num_inserted_bytes) {
      size_t new_node_bytes = 0;
      size_t new_node_chars = 0;
      
      while (str_offset + new_node_bytes < num_inserted_bytes) {
        size_t cs = codepoint_size(str[str_offset + new_node_bytes]);
        if (cs + new_node_bytes > ROPE_NODE_STR_SIZE) {
          break;
        } else {
          new_node_bytes += cs;
          new_node_chars++;
        }
      }
      
      insert_at(r, pos, &str[str_offset], new_node_bytes, new_node_chars, nodes, tree_offsets);
      pos += new_node_chars;
      str_offset += new_node_bytes;
    }
    
    if (num_end_bytes) {
      insert_at(r, pos, &e->str[offset_bytes], num_end_bytes, num_end_chars, nodes, tree_offsets);
    }
  }
}

// Delete num characters at position pos. Deleting past the end of the string
// has no effect.
void rope_del(rope *r, size_t pos, size_t length) {
  assert(r);
  pos = MIN(pos, r->num_chars);
  length = MIN(length, r->num_chars - pos);
  
  rope_node *nodes[UINT8_MAX];
  
  // the number of characters to skip in the current node.
  size_t offset;
  
  // Search for the node where we'll insert the string.
  rope_node *e = go_to_node(r, pos, &offset, nodes, NULL);
  
  r->num_chars -= length;
  
  while (length) {
    if (e == NULL || offset == e->nexts[0].skip_size) {
      // Skip this node.
      e = (nodes[0] ? nodes[0]->nexts[0] : r->heads[0]).node;
      offset = 0;
    }
    
    size_t num_chars = e->nexts[0].skip_size;
    size_t removed = MIN(length, num_chars - offset);
    
    int i;
    if (removed < num_chars) {
      // Just trim this node down to size.
      size_t leading_bytes = count_bytes_in_chars(e->str, offset);
      size_t removed_bytes = count_bytes_in_chars(e->str + leading_bytes, removed);
      size_t trailing_bytes = e->num_bytes - leading_bytes - removed_bytes;
      
      if (trailing_bytes) {
        memmove(e->str + leading_bytes, e->str + leading_bytes + removed_bytes, trailing_bytes);
      }
      e->num_bytes -= removed_bytes;
      r->num_bytes -= removed_bytes;
      
      for (i = 0; i < e->height; i++) {
        e->nexts[i].skip_size -= removed;
      }
    } else {
      // Remove the node.
      for (i = 0; i < e->height; i++) {
        rope_next_node *next_node = (nodes[i] ? &nodes[i]->nexts[i] : &r->heads[i]);
        next_node->node = e->nexts[i].node;
        next_node->skip_size += e->nexts[i].skip_size - removed;
      }
      
      r->num_bytes -= e->num_bytes;
      // TODO: Recycle e.
      rope_node *next = e->nexts[0].node;
      free(e);
      e = next;
      offset = 0;
    }
    
    for (; i < r->height; i++) {
      if (nodes[i]) {
        nodes[i]->nexts[i].skip_size -= removed;
      } else {
        r->heads[i].skip_size -= removed;
      }
    }
    
    length -= removed;
  }
}

void _rope_check(rope *r) {
  if (r->height == 0) {
    assert(r->num_bytes == 0);
    assert(r->num_chars == 0);
    return;
  }
  
  size_t num_bytes = 0;
  size_t num_chars = 0;

  for (rope_node *n = r->heads[0].node; n != NULL; n = n->nexts[0].node) {
    num_bytes += n->num_bytes;
    num_chars += n->nexts[0].skip_size;
  }
  
  assert(r->num_bytes == num_bytes);
  assert(r->num_chars == num_chars);
}

// For debugging.
#include <stdio.h>
void _rope_print(rope *r) {
  printf("chars: %ld\tbytes: %ld\theight: %d\n", r->num_chars, r->num_bytes, r->height);

  printf("HEAD");
  for (int i = 0; i < r->height; i++) {
    printf(" |%3ld ", r->heads[i].skip_size);
  }
  printf("\n");
  
  int num = 0;
  for (rope_node *n = r->heads[0].node; n != NULL; n = n->nexts[0].node) {
    printf("%3d:", num++);
    for (int i = 0; i < n->height; i++) {
      printf(" |%3ld ", n->nexts[i].skip_size);
    }
    printf("        : \"");
    fwrite(n->str, n->num_bytes, 1, stdout);
    printf("\"\n");
  }
}

