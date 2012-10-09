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

Wide Character string compatibility
-----------------------------------

Beware! String insertion / deletion positions in Javascript, Objective-C (NSString), Java, C# and others are *incompatible with UTF-8 strings*!

Thats because these languages store strings as arrays of wchars (two byte characters). Some characters in the unicode character set require more than two bytes. These languages correctly encode such characters using multiple wchars (UTF-16). This works most of the time. However, insertion and deletion positions in these strings refer to offsets in the array. That means unicode characters which take up 4 bytes in UTF-16 count as two characters for the purpose of deletion ranges, insertion positions and string length.

Even though these characters are exceptionally rare, I don't want my editor to go all funky if you start getting creative. Therefore about a quarter of librope's code is dedicated to simply fixing this mismatch. However, the bookkeeping isn't free - librope performance drops by 35% with wchar conversion bookkeeping enabled.

For more information, read my [blog post about it](http://josephg.com/post/31707645955/string-length-lies).

Long story short, if you need to interoperate with strings from any of these dodgy languages, here's what you do:

- Compile with `-DROPE_WCHAR=1`. This macro enables the expensive wchar bookkeeping.
- Use the alternate insert & delete functions `rope_insert_at_wchar(...)` and `rope_del_at_wchar`. Take a look at the header file for details. As a byproduct, these functions convert your wchar offsets into utf-8 character offsets and return the results.

#### Beware:

- When using `rope_insert_at_wchar` you still need to convert the string you're inserting into UTF-8 before you pass it into librope.
- The API lets you try to delete or insert halfway through a large character. I don't know what happens if you do that, but I wouldn't recommend finding out.
- librope is 100% faithful when it comes to the characters you're inserting. If your string has byte order marks, you might want to remove them before passing the string into librope.

