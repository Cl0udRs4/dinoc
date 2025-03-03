# C2 System API Documentation

This document provides comprehensive documentation for the C2 system API, including HTTP API endpoints, protocol interfaces, task management system, and protocol fragmentation mechanism.

## Table of Contents

1. [HTTP API Endpoints](#http-api-endpoints)
   - [Task Management API](#task-management-api)
2. [Protocol Interfaces](#protocol-interfaces)
   - [Protocol Manager API](#protocol-manager-api)
   - [TCP Listener](#tcp-listener)
   - [UDP Listener](#udp-listener)
   - [WebSocket Listener](#websocket-listener)
   - [ICMP Listener](#icmp-listener)
   - [DNS Listener](#dns-listener)
3. [Task Management System](#task-management-system)
   - [Task States](#task-states)
   - [Task Types](#task-types)
   - [Task Management API](#task-management-api-1)
4. [Protocol Fragmentation Mechanism](#protocol-fragmentation-mechanism)
   - [Fragmentation API](#fragmentation-api)
   - [Fragment Header Format](#fragment-header-format)
5. [Test Results](#test-results)
   - [Protocol Fragmentation](#protocol-fragmentation)
   - [WebSocket Listener](#websocket-listener-1)
   - [ICMP Listener](#icmp-listener-1)
   - [DNS Listener](#dns-listener-1)
   - [Task Management System](#task-management-system-1)
   - [Encryption Detection](#encryption-detection)

## HTTP API Endpoints

The C2 system provides a RESTful HTTP API for managing tasks and clients. All API endpoints return JSON responses.

### Task Management API

#### GET /api/tasks

Retrieves a list of all tasks.

**Response:**
```json
{
  "tasks": [
    {
      "id": "550e8400-e29b-41d4-a716-446655440000",
      "client_id": "550e8400-e29b-41d4-a716-446655440001",
      "type": 0,
      "state": 1,
      "timeout": 3600,
      "created_time": 1646323200,
      "sent_time": 1646323210,
      "start_time": 1646323220,
      "end_time": 0
    }
  ]
}
```

**Status Codes:**
- 200: Success
- 500: Internal server error

#### POST /api/tasks

Creates a new task.

**Request:**
```json
{
  "client_id": "550e8400-e29b-41d4-a716-446655440001",
  "type": 0,
  "data": "base64_encoded_data",
  "timeout": 3600
}
```

**Response:**
```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "client_id": "550e8400-e29b-41d4-a716-446655440001",
  "type": 0,
  "state": 0,
  "timeout": 3600,
  "created_time": 1646323200
}
```

**Status Codes:**
- 201: Created
- 400: Bad request
- 404: Client not found
- 500: Internal server error

#### GET /api/tasks/{task_id}

Retrieves a specific task by ID.

**Response:**
```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "client_id": "550e8400-e29b-41d4-a716-446655440001",
  "type": 0,
  "state": 1,
  "timeout": 3600,
  "created_time": 1646323200,
  "sent_time": 1646323210,
  "start_time": 1646323220,
  "end_time": 0,
  "result": "base64_encoded_result",
  "error_message": null
}
```

**Status Codes:**
- 200: Success
- 404: Task not found
- 500: Internal server error

#### PUT /api/tasks/{task_id}/state

Updates the state of a task.

**Request:**
```json
{
  "state": 2
}
```

**Response:**
```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "state": 2,
  "start_time": 1646323220
}
```

**Status Codes:**
- 200: Success
- 400: Bad request
- 404: Task not found
- 500: Internal server error

#### POST /api/tasks/{task_id}/result

Sets the result of a task.

**Request:**
```json
{
  "result": "base64_encoded_result"
}
```

**Response:**
```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "state": 3,
  "end_time": 1646323300
}
```

**Status Codes:**
- 200: Success
- 400: Bad request
- 404: Task not found
- 500: Internal server error

#### GET /api/clients/{client_id}/tasks

Retrieves all tasks for a specific client.

**Response:**
```json
{
  "tasks": [
    {
      "id": "550e8400-e29b-41d4-a716-446655440000",
      "client_id": "550e8400-e29b-41d4-a716-446655440001",
      "type": 0,
      "state": 1,
      "timeout": 3600,
      "created_time": 1646323200,
      "sent_time": 1646323210,
      "start_time": 1646323220,
      "end_time": 0
    }
  ]
}
```

**Status Codes:**
- 200: Success
- 404: Client not found
- 500: Internal server error

## Protocol Interfaces

The C2 system supports multiple communication protocols through a unified protocol interface. Each protocol is implemented as a listener that can be created, started, stopped, and destroyed through the protocol manager.

### Protocol Manager API

#### protocol_manager_init

Initializes the protocol manager.

```c
status_t protocol_manager_init(void);
```

**Returns:**
- `STATUS_SUCCESS`: Protocol manager initialized successfully
- `STATUS_ERROR_ALREADY_EXISTS`: Protocol manager already initialized
- `STATUS_ERROR_MEMORY`: Memory allocation failed

#### protocol_manager_shutdown

Shuts down the protocol manager.

```c
status_t protocol_manager_shutdown(void);
```

**Returns:**
- `STATUS_SUCCESS`: Protocol manager shut down successfully
- `STATUS_ERROR_NOT_INITIALIZED`: Protocol manager not initialized

#### protocol_manager_create_listener

Creates a new protocol listener.

```c
status_t protocol_manager_create_listener(protocol_type_t type, const protocol_listener_config_t* config, protocol_listener_t** listener);
```

**Parameters:**
- `type`: Protocol type (TCP, UDP, WS, ICMP, DNS)
- `config`: Protocol listener configuration
- `listener`: Pointer to store created listener

**Returns:**
- `STATUS_SUCCESS`: Listener created successfully
- `STATUS_ERROR_INVALID_PARAMETER`: Invalid parameter
- `STATUS_ERROR_NOT_INITIALIZED`: Protocol manager not initialized
- `STATUS_ERROR_MEMORY`: Memory allocation failed
- `STATUS_ERROR_NOT_SUPPORTED`: Protocol type not supported

#### protocol_manager_destroy_listener

Destroys a protocol listener.

```c
status_t protocol_manager_destroy_listener(protocol_listener_t* listener);
```

**Parameters:**
- `listener`: Listener to destroy

**Returns:**
- `STATUS_SUCCESS`: Listener destroyed successfully
- `STATUS_ERROR_INVALID_PARAMETER`: Invalid parameter
- `STATUS_ERROR_NOT_INITIALIZED`: Protocol manager not initialized

#### protocol_manager_start_listener

Starts a protocol listener.

```c
status_t protocol_manager_start_listener(protocol_listener_t* listener);
```

**Parameters:**
- `listener`: Listener to start

**Returns:**
- `STATUS_SUCCESS`: Listener started successfully
- `STATUS_ERROR_INVALID_PARAMETER`: Invalid parameter
- `STATUS_ERROR_NOT_INITIALIZED`: Protocol manager not initialized
- `STATUS_ERROR_ALREADY_STARTED`: Listener already started

#### protocol_manager_stop_listener

Stops a protocol listener.

```c
status_t protocol_manager_stop_listener(protocol_listener_t* listener);
```

**Parameters:**
- `listener`: Listener to stop

**Returns:**
- `STATUS_SUCCESS`: Listener stopped successfully
- `STATUS_ERROR_INVALID_PARAMETER`: Invalid parameter
- `STATUS_ERROR_NOT_INITIALIZED`: Protocol manager not initialized
- `STATUS_ERROR_NOT_STARTED`: Listener not started

#### protocol_manager_send_message

Sends a message through a protocol listener.

```c
status_t protocol_manager_send_message(protocol_listener_t* listener, client_t* client, protocol_message_t* message);
```

**Parameters:**
- `listener`: Listener to send message through
- `client`: Client to send message to
- `message`: Message to send

**Returns:**
- `STATUS_SUCCESS`: Message sent successfully
- `STATUS_ERROR_INVALID_PARAMETER`: Invalid parameter
- `STATUS_ERROR_NOT_INITIALIZED`: Protocol manager not initialized
- `STATUS_ERROR_NOT_STARTED`: Listener not started
- `STATUS_ERROR_CONNECTION`: Connection error

#### protocol_manager_register_callbacks

Registers callbacks for a protocol listener.

```c
status_t protocol_manager_register_callbacks(protocol_listener_t* listener,
                                           void (*on_message_received)(protocol_listener_t*, client_t*, protocol_message_t*),
                                           void (*on_client_connected)(protocol_listener_t*, client_t*),
                                           void (*on_client_disconnected)(protocol_listener_t*, client_t*));
```

**Parameters:**
- `listener`: Listener to register callbacks for
- `on_message_received`: Callback for message received event
- `on_client_connected`: Callback for client connected event
- `on_client_disconnected`: Callback for client disconnected event

**Returns:**
- `STATUS_SUCCESS`: Callbacks registered successfully
- `STATUS_ERROR_INVALID_PARAMETER`: Invalid parameter
- `STATUS_ERROR_NOT_INITIALIZED`: Protocol manager not initialized

### TCP Listener

The TCP listener implements the protocol interface for TCP communication.

#### tcp_listener_create

Creates a new TCP listener.

```c
status_t tcp_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener);
```

**Parameters:**
- `config`: TCP listener configuration
  - `bind_address`: Address to bind to
  - `port`: Port to listen on
  - `timeout_ms`: Connection timeout in milliseconds
- `listener`: Pointer to store created listener

**Returns:**
- `STATUS_SUCCESS`: Listener created successfully
- `STATUS_ERROR_INVALID_PARAMETER`: Invalid parameter
- `STATUS_ERROR_MEMORY`: Memory allocation failed

### UDP Listener

The UDP listener implements the protocol interface for UDP communication.

#### udp_listener_create

Creates a new UDP listener.

```c
status_t udp_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener);
```

**Parameters:**
- `config`: UDP listener configuration
  - `bind_address`: Address to bind to
  - `port`: Port to listen on
  - `timeout_ms`: Connection timeout in milliseconds
- `listener`: Pointer to store created listener

**Returns:**
- `STATUS_SUCCESS`: Listener created successfully
- `STATUS_ERROR_INVALID_PARAMETER`: Invalid parameter
- `STATUS_ERROR_MEMORY`: Memory allocation failed

### WebSocket Listener

The WebSocket listener implements the protocol interface for WebSocket communication.

#### ws_listener_create

Creates a new WebSocket listener.

```c
status_t ws_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener);
```

**Parameters:**
- `config`: WebSocket listener configuration
  - `bind_address`: Address to bind to
  - `port`: Port to listen on
  - `timeout_ms`: Connection timeout in milliseconds
  - `ws_path`: WebSocket path
- `listener`: Pointer to store created listener

**Returns:**
- `STATUS_SUCCESS`: Listener created successfully
- `STATUS_ERROR_INVALID_PARAMETER`: Invalid parameter
- `STATUS_ERROR_MEMORY`: Memory allocation failed
- `STATUS_ERROR_LIBRARY`: libwebsockets error

**Example:**
```c
// Create WebSocket listener
protocol_listener_config_t config;
memset(&config, 0, sizeof(config));
config.bind_address = "0.0.0.0";
config.port = 8080;
config.timeout_ms = 30000;
config.ws_path = "/ws";

protocol_listener_t* listener;
status_t status = ws_listener_create(&config, &listener);
if (status != STATUS_SUCCESS) {
    // Handle error
}

// Register callbacks
status = protocol_manager_register_callbacks(listener,
                                          on_message_received,
                                          on_client_connected,
                                          on_client_disconnected);
if (status != STATUS_SUCCESS) {
    // Handle error
}

// Start listener
status = protocol_manager_start_listener(listener);
if (status != STATUS_SUCCESS) {
    // Handle error
}
```

### ICMP Listener

The ICMP listener implements the protocol interface for ICMP communication.

#### icmp_listener_create

Creates a new ICMP listener.

```c
status_t icmp_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener);
```

**Parameters:**
- `config`: ICMP listener configuration
  - `pcap_device`: Network device to capture ICMP packets on
  - `timeout_ms`: Connection timeout in milliseconds
- `listener`: Pointer to store created listener

**Returns:**
- `STATUS_SUCCESS`: Listener created successfully
- `STATUS_ERROR_INVALID_PARAMETER`: Invalid parameter
- `STATUS_ERROR_MEMORY`: Memory allocation failed
- `STATUS_ERROR_LIBRARY`: libpcap error
- `STATUS_ERROR_PERMISSION`: Permission denied (requires root privileges)

**Example:**
```c
// Create ICMP listener
protocol_listener_config_t config;
memset(&config, 0, sizeof(config));
config.pcap_device = "eth0";
config.timeout_ms = 30000;

protocol_listener_t* listener;
status_t status = icmp_listener_create(&config, &listener);
if (status != STATUS_SUCCESS) {
    // Handle error
}

// Register callbacks
status = protocol_manager_register_callbacks(listener,
                                          on_message_received,
                                          on_client_connected,
                                          on_client_disconnected);
if (status != STATUS_SUCCESS) {
    // Handle error
}

// Start listener
status = protocol_manager_start_listener(listener);
if (status != STATUS_SUCCESS) {
    // Handle error
}
```

### DNS Listener

The DNS listener implements the protocol interface for DNS communication.

#### dns_listener_create

Creates a new DNS listener.

```c
status_t dns_listener_create(const protocol_listener_config_t* config, protocol_listener_t** listener);
```

**Parameters:**
- `config`: DNS listener configuration
  - `bind_address`: Address to bind to
  - `port`: Port to listen on (usually 53)
  - `timeout_ms`: Connection timeout in milliseconds
  - `domain`: Domain to listen for
- `listener`: Pointer to store created listener

**Returns:**
- `STATUS_SUCCESS`: Listener created successfully
- `STATUS_ERROR_INVALID_PARAMETER`: Invalid parameter
- `STATUS_ERROR_MEMORY`: Memory allocation failed
- `STATUS_ERROR_LIBRARY`: c-ares error
- `STATUS_ERROR_PERMISSION`: Permission denied (requires root privileges for port 53)

**Example:**
```c
// Create DNS listener
protocol_listener_config_t config;
memset(&config, 0, sizeof(config));
config.bind_address = "0.0.0.0";
config.port = 53;
config.timeout_ms = 30000;
config.domain = "example.com";

protocol_listener_t* listener;
status_t status = dns_listener_create(&config, &listener);
if (status != STATUS_SUCCESS) {
    // Handle error
}

// Register callbacks
status = protocol_manager_register_callbacks(listener,
                                          on_message_received,
                                          on_client_connected,
                                          on_client_disconnected);
if (status != STATUS_SUCCESS) {
    // Handle error
}

// Start listener
status = protocol_manager_start_listener(listener);
if (status != STATUS_SUCCESS) {
    // Handle error
}
```

## Task Management System

The task management system provides a way to create, track, and manage tasks assigned to clients.

### Task States

Tasks can be in one of the following states:

- `TASK_STATE_CREATED (0)`: Task has been created but not sent
- `TASK_STATE_SENT (1)`: Task has been sent to client
- `TASK_STATE_RUNNING (2)`: Task is running on client
- `TASK_STATE_COMPLETED (3)`: Task has completed successfully
- `TASK_STATE_FAILED (4)`: Task has failed
- `TASK_STATE_TIMEOUT (5)`: Task has timed out

### Task Types

Tasks can be of the following types:

- `TASK_TYPE_SHELL (0)`: Shell command execution
- `TASK_TYPE_DOWNLOAD (1)`: Download file from client
- `TASK_TYPE_UPLOAD (2)`: Upload file to client
- `TASK_TYPE_MODULE (3)`: Module operation (load, unload, etc.)
- `TASK_TYPE_CONFIG (4)`: Configuration change
- `TASK_TYPE_CUSTOM (5)`: Custom task type

### Task Management API

#### task_manager_init

Initializes the task manager.

```c
status_t task_manager_init(void);
```

**Returns:**
- `STATUS_SUCCESS`: Task manager initialized successfully
- `STATUS_ERROR_ALREADY_EXISTS`: Task manager already initialized
- `STATUS_ERROR_MEMORY`: Memory allocation failed

#### task_manager_shutdown

Shuts down the task manager.

```c
status_t task_manager_shutdown(void);
```

**Returns:**
- `STATUS_SUCCESS`: Task manager shut down successfully
- `STATUS_ERROR_NOT_INITIALIZED`: Task manager not initialized

#### task_create

Creates a new task.

```c
status_t task_create(const uuid_t* client_id, task_type_t type,
                   const uint8_t* data, size_t data_len,
                   uint32_t timeout, task_t** task);
```

**Parameters:**
- `client_id`: Client ID
- `type`: Task type
- `data`: Task data
- `data_len`: Task data length
- `timeout`: Timeout in seconds (0 = no timeout)
- `task`: Pointer to store created task

**Returns:**
- `STATUS_SUCCESS`: Task created successfully
- `STATUS_ERROR_INVALID_PARAMETER`: Invalid parameter
- `STATUS_ERROR_NOT_INITIALIZED`: Task manager not initialized
- `STATUS_ERROR_MEMORY`: Memory allocation failed

**Example:**
```c
// Create a shell command task
uuid_t client_id;
uuid_from_string("550e8400-e29b-41d4-a716-446655440001", &client_id);

const char* command = "ls -la";
task_t* task;
status_t status = task_create(&client_id, TASK_TYPE_SHELL,
                            (const uint8_t*)command, strlen(command),
                            3600, &task);
if (status != STATUS_SUCCESS) {
    // Handle error
}
```

#### task_update_state

Updates the state of a task.

```c
status_t task_update_state(task_t* task, task_state_t state);
```

**Parameters:**
- `task`: Task to update
- `state`: New state

**Returns:**
- `STATUS_SUCCESS`: Task state updated successfully
- `STATUS_ERROR_INVALID_PARAMETER`: Invalid parameter
- `STATUS_ERROR_NOT_INITIALIZED`: Task manager not initialized
- `STATUS_ERROR_INVALID_STATE`: Invalid state transition

#### task_set_result

Sets the result of a task.

```c
status_t task_set_result(task_t* task, const uint8_t* result, size_t result_len);
```

**Parameters:**
- `task`: Task to update
- `result`: Result data
- `result_len`: Result data length

**Returns:**
- `STATUS_SUCCESS`: Task result set successfully
- `STATUS_ERROR_INVALID_PARAMETER`: Invalid parameter
- `STATUS_ERROR_NOT_INITIALIZED`: Task manager not initialized
- `STATUS_ERROR_MEMORY`: Memory allocation failed

#### task_set_error

Sets the error message of a task.

```c
status_t task_set_error(task_t* task, const char* error_message);
```

**Parameters:**
- `task`: Task to update
- `error_message`: Error message

**Returns:**
- `STATUS_SUCCESS`: Task error set successfully
- `STATUS_ERROR_INVALID_PARAMETER`: Invalid parameter
- `STATUS_ERROR_NOT_INITIALIZED`: Task manager not initialized
- `STATUS_ERROR_MEMORY`: Memory allocation failed

#### task_is_timed_out

Checks if a task has timed out.

```c
bool task_is_timed_out(const task_t* task);
```

**Parameters:**
- `task`: Task to check

**Returns:**
- `true`: Task has timed out
- `false`: Task has not timed out

#### task_destroy

Destroys a task.

```c
status_t task_destroy(task_t* task);
```

**Parameters:**
- `task`: Task to destroy

**Returns:**
- `STATUS_SUCCESS`: Task destroyed successfully
- `STATUS_ERROR_INVALID_PARAMETER`: Invalid parameter

#### task_find

Finds a task by ID.

```c
task_t* task_find(const uuid_t* id);
```

**Parameters:**
- `id`: Task ID

**Returns:**
- Pointer to task if found, NULL otherwise

#### task_get_for_client

Gets all tasks for a client.

```c
status_t task_get_for_client(const uuid_t* client_id, task_t*** tasks, size_t* count);
```

**Parameters:**
- `client_id`: Client ID
- `tasks`: Pointer to store tasks array
- `count`: Pointer to store number of tasks

**Returns:**
- `STATUS_SUCCESS`: Tasks retrieved successfully
- `STATUS_ERROR_INVALID_PARAMETER`: Invalid parameter
- `STATUS_ERROR_NOT_INITIALIZED`: Task manager not initialized
- `STATUS_ERROR_MEMORY`: Memory allocation failed
- `STATUS_ERROR_NOT_FOUND`: Client not found

## Protocol Fragmentation Mechanism

The protocol fragmentation mechanism allows large messages to be split into smaller fragments for protocols with size limitations, such as ICMP and DNS.

### Fragmentation API

#### fragmentation_init

Initializes the fragmentation system.

```c
status_t fragmentation_init(void);
```

**Returns:**
- `STATUS_SUCCESS`: Fragmentation system initialized successfully
- `STATUS_ERROR_ALREADY_EXISTS`: Fragmentation system already initialized
- `STATUS_ERROR_MEMORY`: Memory allocation failed

#### fragmentation_shutdown

Shuts down the fragmentation system.

```c
status_t fragmentation_shutdown(void);
```

**Returns:**
- `STATUS_SUCCESS`: Fragmentation system shut down successfully
- `STATUS_ERROR_NOT_INITIALIZED`: Fragmentation system not initialized

#### fragmentation_send_message

Fragments a message and sends it through a protocol listener.

```c
status_t fragmentation_send_message(protocol_listener_t* listener, client_t* client,
                                  const uint8_t* data, size_t data_len,
                                  size_t max_fragment_size);
```

**Parameters:**
- `listener`: Listener to send fragments through
- `client`: Client to send fragments to
- `data`: Message data
- `data_len`: Message data length
- `max_fragment_size`: Maximum fragment size

**Returns:**
- `STATUS_SUCCESS`: Message fragmented and sent successfully
- `STATUS_ERROR_INVALID_PARAMETER`: Invalid parameter
- `STATUS_ERROR_NOT_INITIALIZED`: Fragmentation system not initialized
- `STATUS_ERROR_MEMORY`: Memory allocation failed

**Example:**
```c
// Send a large message through ICMP
const char* message = "This is a large message that needs to be fragmented...";
status_t status = fragmentation_send_message(icmp_listener, client,
                                          (const uint8_t*)message, strlen(message),
                                          64);
if (status != STATUS_SUCCESS) {
    // Handle error
}
```

#### fragmentation_process_fragment

Processes a received fragment and reassembles the original message if all fragments are received.

```c
status_t fragmentation_process_fragment(protocol_listener_t* listener, client_t* client,
                                      const uint8_t* fragment, size_t fragment_len,
                                      void (*on_message_reassembled)(protocol_listener_t*, client_t*, protocol_message_t*));
```

**Parameters:**
- `listener`: Listener that received the fragment
- `client`: Client that sent the fragment
- `fragment`: Fragment data
- `fragment_len`: Fragment data length
- `on_message_reassembled`: Callback for when message is reassembled

**Returns:**
- `STATUS_SUCCESS`: Fragment processed successfully
- `STATUS_ERROR_INVALID_PARAMETER`: Invalid parameter
- `STATUS_ERROR_NOT_INITIALIZED`: Fragmentation system not initialized
- `STATUS_ERROR_MEMORY`: Memory allocation failed
- `STATUS_ERROR_INVALID_FORMAT`: Invalid fragment format

#### fragmentation_create_header

Creates a fragment header.

```c
status_t fragmentation_create_header(uint16_t fragment_id, uint8_t fragment_index,
                                   uint8_t total_fragments, uint8_t flags,
                                   fragment_header_t* header);
```

**Parameters:**
- `fragment_id`: Fragment ID
- `fragment_index`: Fragment index
- `total_fragments`: Total number of fragments
- `flags`: Fragment flags
- `header`: Pointer to store header

**Returns:**
- `STATUS_SUCCESS`: Header created successfully
- `STATUS_ERROR_INVALID_PARAMETER`: Invalid parameter

### Fragment Header Format

The fragment header is a 5-byte structure with the following fields:

- `fragment_id` (2 bytes): Unique identifier for the fragmented message
- `fragment_index` (1 byte): Index of this fragment (0-based)
- `total_fragments` (1 byte): Total number of fragments in the message
- `flags` (1 byte): Fragment flags
  - Bit 0: Last fragment
  - Bit 1: First fragment
  - Bits 2-7: Reserved

**Example:**
```
+----------------+----------------+----------------+----------------+----------------+
| fragment_id[0] | fragment_id[1] | fragment_index | total_fragments|     flags     |
+----------------+----------------+----------------+----------------+----------------+
```

This header is prepended to each fragment before sending and is used to reassemble the original message on the receiving end.

## Test Results

This section documents the results of testing all protocol implementations in the C2 system.

### Protocol Fragmentation

**Test Status: Complete ✅**

**Test Results:**
- Successfully tested fragmentation and reassembly of messages
- Successfully tested fragment timeout handling
- All tests passed without errors

**Test Output:**
```
Testing fragmentation and reassembly...
Fragmenting message into 6 fragments
Message reassembled: This is a test message that will be fragmented into multiple pieces to test the protocol fragmentation system. It needs to be long enough to be split into multiple fragments.
Fragmentation and reassembly test completed successfully
Testing fragment timeout...
Fragmenting message into 6 fragments, but only sending half
Waiting for fragment timeout...
Fragment timeout test completed successfully
All tests completed successfully
```

**Features Verified:**
- Message fragmentation into multiple pieces
- Out-of-order fragment reassembly
- Fragment timeout handling
- Thread safety of the fragmentation system

### WebSocket Listener

**Test Status: Incomplete**

**Issues Encountered:**
- Compilation errors in `test_ws_listener.c`:
  - Missing `errno.h` and `arpa/inet.h` includes - Fixed
  - Unused parameter warnings - Fixed by adding `(void)param` statements
- Linking errors:
  - Undefined references to `tcp_listener_create`, `udp_listener_create`, `icmp_listener_create`, and `dns_listener_create`
  - These functions are declared in `protocol.h` but their implementations may not be linked properly

**Resolution Steps:**
1. Fixed compilation errors by adding necessary includes and addressing unused parameter warnings
2. Need to ensure all protocol listener implementation files are properly compiled and linked

### ICMP Listener

**Test Status: Incomplete**

**Issues Encountered:**
- Similar linking issues expected as with WebSocket listener
- ICMP protocol requires root privileges for raw socket access

**Resolution Steps:**
1. Need to ensure ICMP listener implementation is properly compiled and linked
2. May need to run tests with sudo for raw socket access

### DNS Listener

**Test Status: Incomplete**

**Issues Encountered:**
- Similar linking issues expected as with WebSocket listener
- DNS protocol may require root privileges for binding to port 53

**Resolution Steps:**
1. Need to ensure DNS listener implementation is properly compiled and linked
2. May need to run tests with sudo for binding to privileged ports

### Task Management System

**Test Status: Incomplete**

**Issues Encountered:**
- Compilation warnings in `task_manager.c`:
  - Implicit declaration of function `uuid_compare`
  - Unused parameter warnings
- Linking errors:
  - Undefined reference to `uuid_compare`

**Resolution Steps:**
1. Need to ensure `uuid_compare` function is properly implemented in `uuid.c` and linked
2. Fix unused parameter warnings

### Encryption Detection

**Test Status: Incomplete**

**Issues Encountered:**
- Test file for encryption detection not found in the test directory
- Need to create or locate the encryption detection test file

**Resolution Steps:**
1. Create or locate the encryption detection test file
2. Implement tests to verify that encryption detection works correctly for all protocols
