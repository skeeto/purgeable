CC      = cc
CFLAGS  = -std=c99 -Wall -Wextra -g -O3
LDFLAGS =
LDLIBS  =

all: test hog

test: test.c purgeable.c purgeable.h
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ test.c purgeable.c $(LDLIBS)

hog: hog.c
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ hog.c $(LDLIBS)

clean:
	rm -f test hog
