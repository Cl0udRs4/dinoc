# Makefile for test programs

CC = gcc
CFLAGS = -Wall -Wextra -g -I../include
LDFLAGS = -lpthread -lssl -lcrypto -lmicrohttpd -ljansson

# Source directories
SRC_DIR = ..
COMMON_DIR = $(SRC_DIR)/common
ENCRYPTION_DIR = $(SRC_DIR)/encryption
PROTOCOLS_DIR = $(SRC_DIR)/protocols
API_DIR = $(SRC_DIR)/api

# Test programs
TEST_PROTOCOL_HEADER = test_protocol_header
TEST_TCP_LISTENER = test_tcp_listener

# Source files
COMMON_SRC = $(COMMON_DIR)/logger.c $(COMMON_DIR)/uuid.c $(COMMON_DIR)/utils.c
ENCRYPTION_SRC = $(ENCRYPTION_DIR)/encryption.c $(ENCRYPTION_DIR)/aes.c $(ENCRYPTION_DIR)/chacha20.c
PROTOCOLS_SRC = $(PROTOCOLS_DIR)/protocol_header.c $(PROTOCOLS_DIR)/protocol_handler.c $(PROTOCOLS_DIR)/protocol_manager.c \
                $(PROTOCOLS_DIR)/tcp_listener.c $(PROTOCOLS_DIR)/udp_listener.c

# Object files
COMMON_OBJ = $(COMMON_SRC:.c=.o)
ENCRYPTION_OBJ = $(ENCRYPTION_SRC:.c=.o)
PROTOCOLS_OBJ = $(PROTOCOLS_SRC:.c=.o)

# Default target
all: $(TEST_PROTOCOL_HEADER) $(TEST_TCP_LISTENER)

# Test protocol header
$(TEST_PROTOCOL_HEADER): $(TEST_PROTOCOL_HEADER).o $(COMMON_OBJ) $(PROTOCOLS_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

# Test TCP listener
$(TEST_TCP_LISTENER): $(TEST_TCP_LISTENER).o $(COMMON_OBJ) $(ENCRYPTION_OBJ) $(PROTOCOLS_OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean
clean:
	rm -f *.o $(TEST_PROTOCOL_HEADER) $(TEST_TCP_LISTENER)
	rm -f $(COMMON_OBJ) $(ENCRYPTION_OBJ) $(PROTOCOLS_OBJ)

.PHONY: all clean
