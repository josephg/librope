librope
=======

This is a little C library for heavyweight utf-8 strings (rope). Unlike regular C strings, ropes can do substring insertion and deletion in O(log n) time.

librope is implemented using skip lists, which have the same big-O time complexity as trees but don't require rebalancing.

librope is _fast_. It will happily perform ~1-5 million edit operations per second, depending on the size of your strings. Inserts and deletes in librope outperform straight C strings for any document longer than a few hundred bytes.

This library is still pretty new. Let me know if you find any issues!

Usage
-----

Just add `rope.c` and `rope.h` to your project.

```c
// Make a new empty rope
rope *r = rope_new();

// Put some content in it (at position 0)
rope_insert(r, 0, "Hi there!");

// Delete 6 characters at position 2
rope_delete(r, 2, 6);

// Get the whole string back out of the rope
uint8_t *str = rope_createcstr(rope, NULL);

// str now contains "Hi!"!
```

