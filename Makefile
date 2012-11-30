.PHONY: all clean

CFLAGS=-O2 -Wall -I.

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
CFLAGS := $(CFLAGS) -emit-llvm -arch x86_64
endif

all: librope.a

clean:
	rm -f librope.a *.o tests

rope.o: rope.c rope.h
	$(CC) $(CFLAGS) $< -c

librope.a: rope.o
	ar rcs $@ $+

# Only need corefoundation to run the tests on mac
tests: librope.a test/tests.c test/benchmark.c test/slowstring.c
	$(CC) $(CFLAGS) $+ -o $@

