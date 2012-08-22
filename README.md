librope
=======

This is a little C library for heavyweight utf-8 strings (rope). Unlike regular C strings, ropes can do substring insertion and deletion in O(log n) time.

librope is implemented using skip lists, which have the same time complexity as trees (which are used in classical ropes).

librope is _fast_. It will happily perform ~1-5 million edit operations per second, depending on the size of your strings.

This library is still pretty new. It probably won't build using visual studio yet (pull requests welcome). Let me know if you find any issues!

Usage
-----

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

