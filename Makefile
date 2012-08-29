.PHONY: all clean

CFLAGS=-O2 -emit-llvm -Wall -arch x86_64 -I.

# Debug mode.
#CFLAGS=-g -Wall -arch x86_64 -I.

all: librope.a

clean:
	rm -f librope.a *.o tests

rope.o: rope.c rope.h
	$(CC) $(CFLAGS) $< -c

librope.a: rope.o
	ar -r $@ $+

# Only need corefoundation to run the tests on mac
tests: librope.a test/tests.c test/benchmark.c test/slowstring.c
	$(CC) $(CFLAGS) $+ -o $@

