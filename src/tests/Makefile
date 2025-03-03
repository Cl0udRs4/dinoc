# Makefile for C2 system tests

CC = gcc
CFLAGS = -Wall -Wextra -I../include -g
LDFLAGS = -lpthread -lcrypto -lssl -lm -lz

# Common objects
COMMON_OBJS = ../common/logger.o ../common/uuid.o ../common/utils.o ../common/config.o ../client/client.o

# Protocol objects
PROTOCOL_OBJS = ../protocols/protocol_header.o ../protocols/protocol_handler.o ../protocols/protocol_manager.o ../protocols/protocol_stubs.o

# Encryption objects
ENCRYPTION_OBJS = ../encryption/encryption.o ../encryption/aes.o ../encryption/chacha20.o

# Protocol listener objects
TCP_LISTENER_OBJ = ../protocols/tcp_listener.o
UDP_LISTENER_OBJ = ../protocols/udp_listener.o
WS_LISTENER_OBJ = ../protocols/ws_listener.o
ICMP_LISTENER_OBJ = ../protocols/icmp_listener.o
DNS_LISTENER_OBJ = ../protocols/dns_listener.o

# Protocol fragmentation objects
FRAGMENTATION_OBJ = ../protocols/protocol_fragmentation.o

# Protocol switch objects
PROTOCOL_SWITCH_OBJ = ../protocols/protocol_switch.o

# Task manager objects
TASK_MANAGER_OBJ = ../task/task_manager.o

# Module manager objects
MODULE_MANAGER_OBJ = ../module/module_manager.o
SHELL_MODULE_OBJ = ../module/shell_module.o

# Console objects
CONSOLE_OBJ = ../console/console.o

# Server objects
SERVER_OBJ = ../server/server.o

# API objects
API_OBJS = ../api/http_server.o ../api/task_api.o

# All test targets
TARGETS = test_protocol_header test_tcp_listener test_encryption_detection \
          test_client test_ws_listener test_icmp_listener test_dns_listener \
          test_task_manager test_protocol_fragmentation test_encryption_simple \
          test_client_manager test_protocol_switch test_module_management \
          test_console test_heartbeat test_client_registration

.PHONY: all clean

all: $(TARGETS)

# Protocol header test
test_protocol_header: test_protocol_header.c $(PROTOCOL_OBJS) $(COMMON_OBJS) $(ENCRYPTION_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# TCP listener test
test_tcp_listener: test_tcp_listener.c $(TCP_LISTENER_OBJ) $(PROTOCOL_OBJS) $(COMMON_OBJS) $(ENCRYPTION_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Encryption detection test
test_encryption_detection: test_encryption_detection.c $(PROTOCOL_OBJS) $(COMMON_OBJS) $(ENCRYPTION_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Encryption simple test
test_encryption_simple: test_encryption_simple.c $(ENCRYPTION_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Client test
test_client: test_client.c $(COMMON_OBJS) $(ENCRYPTION_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# WebSocket listener test
test_ws_listener: test_ws_listener.c $(WS_LISTENER_OBJ) $(PROTOCOL_OBJS) $(COMMON_OBJS) $(ENCRYPTION_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lwebsockets

# ICMP listener test
test_icmp_listener: test_icmp_listener.c $(ICMP_LISTENER_OBJ) $(PROTOCOL_OBJS) $(COMMON_OBJS) $(ENCRYPTION_OBJS) $(FRAGMENTATION_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lpcap

# DNS listener test
test_dns_listener: test_dns_listener.c $(DNS_LISTENER_OBJ) $(PROTOCOL_OBJS) $(COMMON_OBJS) $(ENCRYPTION_OBJS) $(FRAGMENTATION_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lares -lresolv

# Task manager test
test_task_manager: test_task_manager.c $(TASK_MANAGER_OBJ) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Protocol fragmentation test
test_protocol_fragmentation: test_protocol_fragmentation.c $(FRAGMENTATION_OBJ) $(PROTOCOL_OBJS) $(COMMON_OBJS) $(TCP_LISTENER_OBJ) $(UDP_LISTENER_OBJ) $(WS_LISTENER_OBJ) $(ICMP_LISTENER_OBJ) $(DNS_LISTENER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -lwebsockets -lpcap

# Client manager test
test_client_manager: test_client_manager.c $(COMMON_OBJS) $(PROTOCOL_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Protocol switch test
test_protocol_switch: test_protocol_switch.c $(PROTOCOL_SWITCH_OBJ) $(PROTOCOL_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Module management test
test_module_management: test_module_management.c $(MODULE_MANAGER_OBJ) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -ldl

# Console test
test_console: test_console.c $(CONSOLE_OBJ) $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Heartbeat test
test_heartbeat: test_heartbeat.c $(COMMON_OBJS) $(PROTOCOL_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Client registration test
test_client_registration: test_client_registration.c $(COMMON_OBJS) $(PROTOCOL_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Client simulator
client_simulator: client_simulator.c $(COMMON_OBJS) $(ENCRYPTION_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Clean up
clean:
	rm -f $(TARGETS) client_simulator *.o

# Run tests
test: all
	./test_protocol_header
	./test_tcp_listener
	./test_encryption_detection
	./test_encryption_simple
	./test_client
	./test_ws_listener
	./test_icmp_listener
	./test_dns_listener
	./test_task_manager
	./test_protocol_fragmentation
	./test_client_manager
	./test_protocol_switch
	./test_module_management
	./test_console
	./test_heartbeat
	./test_client_registration
	./test_task_api.sh
