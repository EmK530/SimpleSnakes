# -------------------------
# Makefile for Global Snake
# I am not good at Makefile so this is very AI-generated :)
# -------------------------

# Default target
.DEFAULT_GOAL := all

# Compiler
CC := gcc

# Directories
SRC_DIR := src
LIB_DIR := libraries
INC_DIR := include
BUILD_DIR := build
BUILD_OUT_LINUX := build/out/linux
BUILD_OUT_WIN64 := build/out/win64

# SDL2 flags (Linux / WSL)
SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LDFLAGS := $(shell sdl2-config --libs)

# Additional SDL2 libraries
EXTRA_LIBS := -lSDL2_ttf -lSDL2_mixer -lm

# Output binary
TARGET := snake

# -----------------------------------------------------
# Source file discovery (recursive)
# -----------------------------------------------------
SRCS := \
	main.c \
	$(shell find $(SRC_DIR) -name '*.c') \
	$(shell find $(LIB_DIR) -name '*.c')

OBJS := $(patsubst %.c, $(BUILD_DIR)/linux/%.o, $(SRCS))

# -----------------------------------------------------
# Include directories
# -----------------------------------------------------
INCLUDES := -I$(INC_DIR) -I$(LIB_DIR)
CFLAGS_BASE := -Wall -Wextra $(INCLUDES) -O2 $(SDL_CFLAGS) # -DDEV_BUILD
LDFLAGS := $(SDL_LDFLAGS) $(EXTRA_LIBS)

SANITIZE_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer
DEBUG_FLAGS=-g -O0 -Wno-unused-function -Wno-format-truncation

ifeq ($(MODE),debug)
    CFLAGS=$(SANITIZE_FLAGS) $(CFLAGS_BASE) $(DEBUG_FLAGS)
    LDFLAGS= $(SDL_LDFLAGS) $(EXTRA_LIBS) $(SANITIZE_FLAGS)
else
    CFLAGS=$(CFLAGS_BASE)
    LDFLAGS= $(SDL_LDFLAGS) $(EXTRA_LIBS)
endif

# -----------------------------------------------------
# Linux build
# -----------------------------------------------------
all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_OUT_LINUX)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/linux/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# -----------------------------------------------------
# Windows cross-compile (explicit call: make windows)
# -----------------------------------------------------
WIN_CC := x86_64-w64-mingw32-gcc
WIN_TARGET := snake.exe
WIN_LIBS_DIR := win64_deps/libs
WIN_INCLUDE_DIR := win64_deps/include
WIN_CFLAGS := -I$(INC_DIR) -I$(LIB_DIR) -I$(LIB_DIR)/jansson -I$(WIN_INCLUDE_DIR) -Dmain=SDL_main
WIN_LDFLAGS := -L$(WIN_LIBS_DIR) \
    -lmingw32 -lSDL2main -lSDL2 -lpthread \
    -lSDL2_ttf -lSDL2_mixer \
    -lwinmm -lgdi32 -luser32 -lkernel32 -lole32 -lsetupapi \
	-lws2_32 -limm32 -lole32 -loleaut32 -lversion -lrpcrt4

WIN_CFLAGS += -O2
WIN_LDFLAGS += -s

# Windows object files (built by MinGW)
WIN_OBJS := $(patsubst %.c, $(BUILD_DIR)/win64/%.o, $(SRCS))

# Compile Windows .o files
$(BUILD_DIR)/win64/%.o: %.c
	@mkdir -p $(dir $@)
	$(WIN_CC) $(WIN_CFLAGS) -c $< -o $@

# Windows build target
windows: $(WIN_OBJS)
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_OUT_WIN64)
	$(WIN_CC) $(WIN_OBJS) -o $(BUILD_OUT_WIN64)/$(WIN_TARGET) $(WIN_LDFLAGS)
	@echo "Copying DLLs..."
	@cp -u win64_deps/dlls/*.dll $(dir $(BUILD_OUT_WIN64)/*.dll)
	@echo "Copying assets..."
	@mkdir -p $(dir $(BUILD_OUT_WIN64)/)
	@cp -r assets $(dir $(BUILD_OUT_WIN64)/)

# -----------------------------------------------------
# Helpers
# -----------------------------------------------------
run: all
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

debug: clean
	$(MAKE) MODE=debug

.PHONY: all clean run debug