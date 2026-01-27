.PHONY: clean
CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lm -lraylib

SRCS = $(wildcard *.c)
OBJS = $(addprefix $(OBJS_DIR)/, $(SRCS:.c=.o))

ASAN_FLAGS = -fsanitize=address -fno-omit-frame-pointer

ifeq ($(DEBUG), 1)
	CFLAGS += -g -O0
else
	CFLAGS += -O3
endif

ifeq ($(ASAN), 1)
	CFLAGS += $(ASAN_FLAGS)
	LDFLAGS += $(ASAN_FLAGS)
endif

OBJS_DIR = objects


all: main

main: ensure_dirs $(OBJS)
	$(CC) $(CFLAGS) -o main $(OBJS) $(LDFLAGS)

$(OBJS_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

ensure_dirs:
	mkdir -p $(OBJS_DIR)

clean:
	rm -rf main $(OBJS_DIR)
