CWD=$(shell pwd)
CC=gcc
# Enable all compiler warnings. 
CCFLAGS=-g -Wall -std=gnu99 -I./inc -I./external/inc  -D_LARGEFILE64_SOURCE
# Linker flags
LDFLAGS=-lz -L. -lldb -lpthread -lcrypto

BUILD_DIR =build
SOURCES=$(wildcard src/*.c) $(wildcard src/**/*.c)  $(wildcard external/*.c) $(wildcard external/**/*.c)

SOURCES_MINR=$(filter-out src/mz_main.c, $(SOURCES))
OBJECTS_MIRN=$(SOURCES_MINR:.c=.o)

SOURCES_MZ=$(filter-out src/main.c, $(SOURCES))
OBJECTS_MZ=$(SOURCES_MZ:.c=.o)

TARGET_MINR=minr
TARGET_MZ=mz


all: clean $(TARGET_MINR) $(TARGET_MZ) clean_build

$(TARGET_MINR): $(OBJECTS_MIRN)
	$(CC) -g -o $@ $^ $(LDFLAGS)

$(TARGET_MZ): $(OBJECTS_MZ)
	$(CC) -g -o $@ $^ $(LDFLAGS)

.PHONY: minr

%.o: %.c
	$(CC) $(CCFLAGS) -o $@ -c $<

clean_build:
	rm -f src/*.o src/**/*.o 

clean:
	 rm -f src/*.o src/**/*.o  $(TARGET_MINR)
	 rm -f src/*.o src/**/*.o  $(TARGET_MZ)

install:
	cp $(TARGET_MINR) /usr/bin
	cp $(TARGET_MZ) /usr/bin

update-docs:
	openapi-spec-gen . > scanoss-api.yaml
  
	
