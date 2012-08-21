.PHONY: all clean

# Only need corefoundation to run the tests on mac
CFLAGS=-O2 -emit-llvm -Wall -framework CoreFoundation

all: librope.a

clean:
	rm librope.a *.o tests

rope.o: rope.c rope.h
	$(CC) $(CFLAGS) $< -c

tests.o: tests.c rope.h
	$(CC) $(CFLAGS) $< -c

librope.a: rope.o
	ar -r $@ $+

tests: librope.a tests.c
	$(CC) $(CFLAGS) $+ -o $@

