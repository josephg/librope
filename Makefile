.PHONY: all clean

CFLAGS=-O2 -emit-llvm -Wall

all: librope.a

clean:
	rm librope.a *.o tests

rope.o: rope.c rope.h
	$(CC) $(CFLAGS) $< -c

tests.o: tests.c rope.h
	$(CC) $(CFLAGS) $< -c

librope.a: rope.o
	ar -r $@ $+

# Only need corefoundation to run the tests on mac
tests: librope.a tests.c benchmark.c
	$(CC) $(CFLAGS) -framework CoreFoundation $+ -o $@

