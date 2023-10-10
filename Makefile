CWD=$(shell pwd)
CC=gcc
# Enable all compiler warnings. 
CCFLAGS?=-g -Wall -I./inc -I./external/inc -D_LARGEFILE64_SOURCE -D_GNU_SOURCE

# Linker flags
LDFLAGS=-lz -lldb -lpthread -ldl

LDB_CURRENT_VERSION := $(shell ldb -v | sed 's/ldb-//' | head -c 3)
LDB_TARGET_VERSION := 3.2

VERSION_IS_LESS := $(shell echo $(LDB_CURRENT_VERSION) \< $(LDB_TARGET_VERSION) | bc)
ifeq ($(VERSION_IS_LESS),1)
	LDFLAGS += -lcrypto
endif

BUILD_DIR =build
SOURCES=$(wildcard src/*.c) $(wildcard src/**/*.c)  $(wildcard external/*.c) $(wildcard external/**/*.c)

SOURCES_MINR=$(filter-out src/mz_main.c, $(SOURCES))
OBJECTS_MIRN=$(SOURCES_MINR:.c=.o)

SOURCES_MZ=$(filter-out src/main.c, $(SOURCES))
OBJECTS_MZ=$(SOURCES_MZ:.c=.o)

TARGET_MINR=minr
TARGET_MZ=mz

VERSION=$(shell ./version.sh)

all: clean $(TARGET_MINR) $(TARGET_MZ)

$(TARGET_MINR): $(OBJECTS_MIRN)
	@echo "Current version: $(LDB_CURRENT_VERSION)"
	@echo "LDFLAGS: $(LDFLAGS)"
	$(CC) -o $@ $^ $(LDFLAGS)

$(TARGET_MZ): $(OBJECTS_MZ)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: minr

%.o: %.c
	$(CC) $(CCFLAGS) -o $@ -c $<

clean:
	rm -f src/*.o src/**/*.o external/src/*.o 

install:
	cp $(TARGET_MINR) /usr/bin
	cp $(TARGET_MZ) /usr/bin

prepare_deb_package: all ## Prepares the deb Package 
	@./package.sh deb $(VERSION)
	@echo deb package built

prepare_rpm_package: all ## Prepares the rpm Package 
	@./package.sh rpm $(VERSION)
	@echo rpm package built
