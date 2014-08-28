CC=cc

all:	cfg_parse.c main.c cfg_parse.h
	$(CC) -Wall -Wextra -ansi -pedantic -O2 -pipe -fomit-frame-pointer -march=native -c cfg_parse.c
	$(CC) -Wall                         -O2 -pipe -fomit-frame-pointer -march=native -c main.c
	$(CC) -Wall -Wextra -ansi -pedantic -O2 -pipe -fomit-frame-pointer -march=native -o test *.o

test:	all
	./test

clean:
	rm -f test *.o config_new.ini
