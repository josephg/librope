.PHONY: all clean

# Only need corefoundation to run the tests on mac
CFLAGS=-O2 -Wall -framework CoreFoundation

all: librope.a

clean:
	rm librope.a *.o

rope.o: rope.c rope.h
	$(CC) $(CFLAGS) $< -c

tests.o: tests.c rope.h
	$(CC) $(CFLAGS) $< -c

librope.a: rope.o
	ar -r $@ $+

test: librope.a tests.c
	$(CC) $(CFLAGS) $+ -o $@
	./$@

