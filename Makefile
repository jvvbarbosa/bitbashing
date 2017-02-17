CC=gcc
CFLAGS=-g -Wall -pedantic


all: bitbash

bitbash: bitbash.c
	$(CC) -o bitbash bitbash.c $(CFLAGS)


clean:
	rm bitbash

.PHONY: clean
