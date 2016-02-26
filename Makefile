CC ?= cc

CFLAGS += -Wall -Wextra -ansi -pedantic

all:	cfg_parse.c main.c cfg_parse.h
	$(CC) $(CFLAGS) -c cfg_parse.c
	$(CC) $(CFLAGS) -o test main.c cfg_parse.o

test:	all
	./test

clean:
	rm -f test *.o config_new.ini
