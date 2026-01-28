.PHONY: clean
CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lm -lraylib

SRCS_DIR = src
HEADERS_DIR = headers
OBJS_DIR = objects

SRCS = $(wildcard $(SRCS_DIR)/*.c)
OBJS = $(addprefix $(OBJS_DIR)/, $(notdir $(SRCS:.c=.o)))

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


all: spg_sim

spg_sim: ensure_dirs $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@

$(OBJS_DIR)/%.o: $(SRCS_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -I $(HEADERS_DIR)

ensure_dirs:
	mkdir -p $(OBJS_DIR)

clean:
	rm -rf spg_sim $(OBJS_DIR)
