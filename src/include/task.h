/**
 * @file task.h
 * @brief Task management interface
 */

#ifndef DINOC_TASK_H
#define DINOC_TASK_H

#include "common.h"
#include "client.h"

/**
 * @brief Task type enumeration
 */
typedef enum {
    TASK_TYPE_SHELL_COMMAND = 1,
    TASK_TYPE_LOAD_MODULE = 2,
    TASK_TYPE_UNLOAD_MODULE = 3,
    TASK_TYPE_CHANGE_PROTOCOL = 4,
    TASK_TYPE_CHANGE_HEARTBEAT = 5,
    TASK_TYPE_CUSTOM = 100
} task_type_t;

/**
 * @brief Task structure
 */
typedef struct {
    uuid_t id;                  // Task ID
    uuid_t client_id;           // Target client ID
    task_type_t type;           // Task type
    task_state_t state;         // Task state
    uint32_t timeout;           // Task timeout in seconds (0 for no timeout)
    time_t created_time;        // Task creation time
    time_t sent_time;           // Task sent time
    time_t completed_time;      // Task completion time
    uint8_t* data;              // Task data
    size_t data_len;            // Data length
    uint8_t* result;            // Task result
    size_t result_len;          // Result length
    char* error_message;        // Error message (if failed)
} task_t;

/**
 * @brief Create a new task
 * 
 * @param client_id Target client ID
 * @param type Task type
 * @param data Task data
 * @param data_len Data length
 * @param timeout Task timeout in seconds (0 for no timeout)
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
 * @param result_len Result length
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
 * @return bool True if timed out, false otherwise
 */
bool task_is_timed_out(const task_t* task);

/**
 * @brief Destroy a task
 * 
 * @param task Task to destroy
 * @return status_t Status code
 */
status_t task_destroy(task_t* task);

#endif /* DINOC_TASK_H */
