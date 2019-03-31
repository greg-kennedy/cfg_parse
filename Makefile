CC ?= cc

#CFLAGS = -O0 -g
CFLAGS += -Wall -Wextra -ansi -pedantic

all:	test

test:	cfg_parse.c main.c cfg_parse.h
	$(CC) $(CFLAGS) -c cfg_parse.c
	$(CC) $(CFLAGS) -o test main.c cfg_parse.o

check:
	cppcheck --std=c89 --enable=all cfg_parse.h cfg_parse.c main.c

docs:
	doxygen

clean:
	rm -f test *.o config_new.ini
