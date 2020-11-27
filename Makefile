CC=gcc
CFLAGS=-O -Wall -g -lpthread -lcrypto -lz

all: minr unmz

minr: src/external/crc32c/crc32c.c src/external/wfp/winnowing.c src/*c
	 $(CC) -o minr src/main.c $(CFLAGS)

unmz: src/unmz.c
	 $(CC) -o unmz src/unmz.c $(CFLAGS)

install:
	cp unmz /usr/bin
	cp minr /usr/bin

clean:
	rm -f minr unmz *.o

distclean: clean

