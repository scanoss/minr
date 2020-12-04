CC=gcc
CFLAGS=-O -Wall -g -lpthread -lcrypto -lz

all: minr mz

minr: src/*
	 $(CC) -o minr src/main.c $(CFLAGS)

mz: src/*
	 $(CC) -o mz src/mz_main.c $(CFLAGS)

install:
	cp mz /usr/bin
	cp minr /usr/bin

clean:
	rm -f minr mz *.o

distclean: clean

