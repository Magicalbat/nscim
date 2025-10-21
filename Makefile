
CC = clang
CFLAGS = -m64 -std=c99 -Isrc 
DEBUG_CFLAGS = -DDEBUG -g -O0 -fsanitize=address
RELEASE_CFLAGS = -DNDEBUG -O2

CFLAGS += -Wall -Wextra -pedantic -Wconversion
CFLAGS += -Wno-gnu-binary-literal -Wno-c23-extensions

config ?= debug
 
ifeq ($(config), debug)
	CFLAGS += $(DEBUG_CFLAGS)
else
	CFLAGS += $(RELEASE_CFLAGS)
endif


# OS-Specific Stuff
LFLAGS = 
MKDIR_BIN = 
RM_BIN = 
BIN_EXT = 

ifeq ($(OS), Windows_NT)
	LFLAGS += -lgdi32 -lkernel32 -luser32 -lBcrypt 
	MKDIR_BIN = if not exist bin\$(config) mkdir bin\$(config)
	RM_BIN = rd /s /q bin
	BIN_EXT = .exe
else
	LFLAGS += -lm 
	MKDIR_BIN = mkdir -p bin/$(config)
	RM_BIN = rm -r bin
endif

SRC_DIR = src
BIN = bin/$(config)/nscim

all: nscim 

nscim:
	@$(MKDIR_BIN)
	$(CC) $(SRC_DIR)/main.c $(CFLAGS) $(LFLAGS) -o $(BIN)$(BIN_EXT)

clean:
	$(RM_BIN)

.PHONY: all nscim clean

