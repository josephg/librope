.PHONY: all clean

CFLAGS=-O2 -Wall -I. -std=c99

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
CFLAGS := $(CFLAGS) -arch x86_64
endif

all: librope.a

clean:
	rm -f librope.a *.bc *.o tests

# You can add -emit-llvm here if you're using clang.
rope.o: rope.c rope.h
	$(CC) $(CFLAGS) $< -c -o $@

librope.a: rope.o
	ar rcs $@ $+

# Only need corefoundation to run the tests on mac
tests: test/tests.c test/benchmark.c test/slowstring.c librope.a
	$(CC) $(CFLAGS) $+ -o $@

