BUILD_DIR := build
TEST_PROG := $(BUILD_DIR)/rose_mem_debugger_test

CC := gcc
CFLAGS := -Wall -Wextra -Werror -Wshadow -pedantic -std=c99 -O0 -ggdb3

SRC_DIRS := src
C_FILES := $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c))
O_FILES := $(C_FILES:%.c=$(BUILD_DIR)/%.o)

INC_DIRS := include
INC_FLAGS := $(INC_DIRS:%=-I%)
CFLAGS += $(INC_FLAGS)

all: $(TEST_PROG)

$(TEST_PROG): $(O_FILES)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: all clean

clean:
	rm -rf $(BUILD_DIR)
