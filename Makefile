# Main Makefile for DinoC C2 Server

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -I./src/include
LDFLAGS = -L./lib
LDLIBS = -lpthread -luuid -lssl -lcrypto -lmicrohttpd -ljansson -lpcap -lwebsockets -lz -lreadline -lm

# Directories
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin
LIB_DIR = lib

# Source files
SERVER_SRCS = $(wildcard $(SRC_DIR)/*.c) \
              $(wildcard $(SRC_DIR)/api/*.c) \
              $(wildcard $(SRC_DIR)/client/*.c) \
              $(wildcard $(SRC_DIR)/common/*.c) \
              $(wildcard $(SRC_DIR)/console/*.c) \
              $(wildcard $(SRC_DIR)/encryption/*.c) \
              $(wildcard $(SRC_DIR)/module/*.c) \
              $(wildcard $(SRC_DIR)/protocols/*.c) \
              $(wildcard $(SRC_DIR)/server/*.c) \
              $(wildcard $(SRC_DIR)/task/*.c) \
              $(SRC_DIR)/common/base64.c

# Object files
SERVER_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SERVER_SRCS))

# Targets
SERVER_TARGET = $(BIN_DIR)/server
BUILDER_TARGET = $(BIN_DIR)/builder

# Phony targets
.PHONY: all clean server builder tests

all: server builder

# Create directories
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/api
	mkdir -p $(BUILD_DIR)/client
	mkdir -p $(BUILD_DIR)/common
	mkdir -p $(BUILD_DIR)/console
	mkdir -p $(BUILD_DIR)/encryption
	mkdir -p $(BUILD_DIR)/module
	mkdir -p $(BUILD_DIR)/protocols
	mkdir -p $(BUILD_DIR)/server
	mkdir -p $(BUILD_DIR)/task
	mkdir -p $(BUILD_DIR)/builder

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(LIB_DIR):
	mkdir -p $(LIB_DIR)

# Server target
server: $(BUILD_DIR) $(BIN_DIR) $(SERVER_TARGET)

$(SERVER_TARGET): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Builder target
builder: $(BUILD_DIR) $(BIN_DIR)
	$(MAKE) -C $(SRC_DIR)/builder
	cp $(SRC_DIR)/builder/builder $(BUILDER_TARGET)

# Tests target
tests:
	$(MAKE) -C $(SRC_DIR)/tests
	$(MAKE) -C $(SRC_DIR)/tests/builder

# Clean target
clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(BIN_DIR)
	$(MAKE) -C $(SRC_DIR)/builder clean
	$(MAKE) -C $(SRC_DIR)/tests clean
