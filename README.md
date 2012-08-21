librope
=======

This is a little C library for heavyweight utf-8 strings (rope). Unlike regular C strings, ropes can do substring insertion and deletion in O(log n) time.

librope is implemented using skip lists.

This library is still pretty new. It probably won't build using visual studio yet (patches welcome btw) and there's probably about 1-2 bugs remaining in the code.

Let me know if you find any issues!

Usage
-----

```c
// Make a new empty rope
rope *r = rope_new();

// Put some content in it (at position 0)
rope_insert(r, 0, "Hi there!");

// Delete some characters
rope_delete(r, 2, 6);

// Get the whole string back out of the rope
uint8_t *str = rope_createcstr(rope, NULL);

// str now contains "Hi!"!
```

