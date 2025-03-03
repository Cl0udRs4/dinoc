/**
 * @file task_manager.c
 * @brief Implementation of task management system
 */

#include "../include/task.h"
#include "../include/client.h"
#include "../common/uuid.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// Task manager structure
typedef struct {
    task_t** tasks;                 // Array of tasks
    size_t task_count;              // Number of tasks
    size_t task_capacity;           // Capacity of tasks array
    pthread_mutex_t mutex;          // Mutex for thread safety
    pthread_t timeout_thread;       // Thread for checking timeouts
    bool running;                   // Running flag
} task_manager_t;

// Global task manager
static task_manager_t* global_manager = NULL;

// Forward declaration for the timeout thread function
static void* task_timeout_thread(void* arg);

/**
 * @brief Initialize task manager
 */
status_t task_manager_init(void) {
    if (global_manager != NULL) {
        return STATUS_ERROR_ALREADY_EXISTS;
    }
    
    global_manager = (task_manager_t*)malloc(sizeof(task_manager_t));
    if (global_manager == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    global_manager->task_capacity = 16;
    global_manager->task_count = 0;
    global_manager->tasks = (task_t**)malloc(global_manager->task_capacity * sizeof(task_t*));
    
    if (global_manager->tasks == NULL) {
        free(global_manager);
        global_manager = NULL;
        return STATUS_ERROR_MEMORY;
    }
    
    pthread_mutex_init(&global_manager->mutex, NULL);
    global_manager->running = true;
    
    // Create timeout thread
    if (pthread_create(&global_manager->timeout_thread, NULL, task_timeout_thread, NULL) != 0) {
        pthread_mutex_destroy(&global_manager->mutex);
        free(global_manager->tasks);
        free(global_manager);
        global_manager = NULL;
        return STATUS_ERROR_GENERIC;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Shutdown task manager
 */
status_t task_manager_shutdown(void) {
    if (global_manager == NULL) {
        return STATUS_ERROR_NOT_FOUND;
    }
    
    global_manager->running = false;
    pthread_join(global_manager->timeout_thread, NULL);
    
    pthread_mutex_lock(&global_manager->mutex);
    
    // Free all tasks
    for (size_t i = 0; i < global_manager->task_count; i++) {
        task_destroy(global_manager->tasks[i]);
    }
    
    free(global_manager->tasks);
    pthread_mutex_unlock(&global_manager->mutex);
    pthread_mutex_destroy(&global_manager->mutex);
    
    free(global_manager);
    global_manager = NULL;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Create a new task
 */
status_t task_create(const uuid_t* client_id, task_type_t type,
                   const uint8_t* data, size_t data_len,
                   uint32_t timeout, task_t** task) {
    if (global_manager == NULL) {
        return STATUS_ERROR_NOT_FOUND;
    }
    
    if (client_id == NULL || task == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Create task
    task_t* new_task = (task_t*)malloc(sizeof(task_t));
    if (new_task == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Initialize task
    memset(new_task, 0, sizeof(task_t));
    uuid_generate_compat(&new_task->id);
    memcpy(&new_task->client_id, client_id, sizeof(uuid_t));
    new_task->type = type;
    new_task->state = TASK_STATE_CREATED;
    new_task->timeout = timeout;
    new_task->created_time = time(NULL);
    
    // Copy data if provided
    if (data != NULL && data_len > 0) {
        new_task->data = (uint8_t*)malloc(data_len);
        if (new_task->data == NULL) {
            free(new_task);
            return STATUS_ERROR_MEMORY;
        }
        
        memcpy(new_task->data, data, data_len);
        new_task->data_len = data_len;
    }
    
    // Add to task manager
    pthread_mutex_lock(&global_manager->mutex);
    
    // Resize array if needed
    if (global_manager->task_count >= global_manager->task_capacity) {
        size_t new_capacity = global_manager->task_capacity * 2;
        task_t** new_tasks = (task_t**)realloc(global_manager->tasks, new_capacity * sizeof(task_t*));
        
        if (new_tasks == NULL) {
            pthread_mutex_unlock(&global_manager->mutex);
            if (new_task->data != NULL) {
                free(new_task->data);
            }
            free(new_task);
            return STATUS_ERROR_MEMORY;
        }
        
        global_manager->tasks = new_tasks;
        global_manager->task_capacity = new_capacity;
    }
    
    global_manager->tasks[global_manager->task_count++] = new_task;
    pthread_mutex_unlock(&global_manager->mutex);
    
    *task = new_task;
    return STATUS_SUCCESS;
}

/**
 * @brief Update task state
 */
status_t task_update_state(task_t* task, task_state_t state) {
    if (task == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    task->state = state;
    
    // Update timestamps based on state
    time_t now = time(NULL);
    
    switch (state) {
        case TASK_STATE_SENT:
            task->sent_time = now;
            break;
            
        case TASK_STATE_RUNNING:
            task->start_time = now;
            break;
            
        case TASK_STATE_COMPLETED:
        case TASK_STATE_FAILED:
        case TASK_STATE_TIMEOUT:
            task->end_time = now;
            break;
            
        default:
            break;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Set task result
 */
status_t task_set_result(task_t* task, const uint8_t* result, size_t result_len) {
    if (task == NULL || (result == NULL && result_len > 0)) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Free previous result if any
    if (task->result != NULL) {
        free(task->result);
        task->result = NULL;
        task->result_len = 0;
    }
    
    // Copy new result if provided
    if (result != NULL && result_len > 0) {
        task->result = (uint8_t*)malloc(result_len);
        if (task->result == NULL) {
            return STATUS_ERROR_MEMORY;
        }
        
        memcpy(task->result, result, result_len);
        task->result_len = result_len;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Set task error
 */
status_t task_set_error(task_t* task, const char* error_message) {
    if (task == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Free previous error if any
    if (task->error_message != NULL) {
        free(task->error_message);
        task->error_message = NULL;
    }
    
    // Copy new error if provided
    if (error_message != NULL) {
        task->error_message = strdup(error_message);
        if (task->error_message == NULL) {
            return STATUS_ERROR_MEMORY;
        }
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Check if task has timed out
 */
bool task_is_timed_out(const task_t* task) {
    if (task == NULL || task->timeout == 0) {
        return false;
    }
    
    time_t now = time(NULL);
    time_t start_time = task->sent_time > 0 ? task->sent_time : task->created_time;
    
    return (now - start_time) > task->timeout;
}

/**
 * @brief Destroy a task
 */
status_t task_destroy(task_t* task) {
    if (task == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    if (task->data != NULL) {
        free(task->data);
    }
    
    if (task->result != NULL) {
        free(task->result);
    }
    
    if (task->error_message != NULL) {
        free(task->error_message);
    }
    
    free(task);
    return STATUS_SUCCESS;
}

/**
 * @brief Find a task by ID
 */
task_t* task_find(const uuid_t* id) {
    if (global_manager == NULL || id == NULL) {
        return NULL;
    }
    
    task_t* found_task = NULL;
    
    pthread_mutex_lock(&global_manager->mutex);
    
    for (size_t i = 0; i < global_manager->task_count; i++) {
        if (uuid_compare_wrapper(*id, global_manager->tasks[i]->id) == 0) {
            found_task = global_manager->tasks[i];
            break;
        }
    }
    
    pthread_mutex_unlock(&global_manager->mutex);
    
    return found_task;
}

/**
 * @brief Get tasks for a client
 */
status_t task_get_for_client(const uuid_t* client_id, task_t*** tasks, size_t* count) {
    if (global_manager == NULL || client_id == NULL || tasks == NULL || count == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Count matching tasks
    size_t matching_count = 0;
    
    pthread_mutex_lock(&global_manager->mutex);
    
    for (size_t i = 0; i < global_manager->task_count; i++) {
        if (uuid_compare_wrapper(*client_id, global_manager->tasks[i]->client_id) == 0) {
            matching_count++;
        }
    }
    
    if (matching_count == 0) {
        pthread_mutex_unlock(&global_manager->mutex);
        *tasks = NULL;
        *count = 0;
        return STATUS_SUCCESS;
    }
    
    // Allocate array for matching tasks
    task_t** matching_tasks = (task_t**)malloc(matching_count * sizeof(task_t*));
    if (matching_tasks == NULL) {
        pthread_mutex_unlock(&global_manager->mutex);
        return STATUS_ERROR_MEMORY;
    }
    
    // Fill array with matching tasks
    size_t index = 0;
    for (size_t i = 0; i < global_manager->task_count; i++) {
        if (uuid_compare_wrapper(*client_id, global_manager->tasks[i]->client_id) == 0) {
            matching_tasks[index++] = global_manager->tasks[i];
        }
    }
    
    pthread_mutex_unlock(&global_manager->mutex);
    
    *tasks = matching_tasks;
    *count = matching_count;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Task timeout thread
 */
static void* task_timeout_thread(void* arg) {
    while (global_manager != NULL && global_manager->running) {
        pthread_mutex_lock(&global_manager->mutex);
        
        // Check for timed out tasks
        for (size_t i = 0; i < global_manager->task_count; i++) {
            task_t* task = global_manager->tasks[i];
            
            if (task->state != TASK_STATE_COMPLETED && 
                task->state != TASK_STATE_FAILED &&
                task->state != TASK_STATE_TIMEOUT &&
                task_is_timed_out(task)) {
                
                task_update_state(task, TASK_STATE_TIMEOUT);
                task_set_error(task, "Task timed out");
            }
        }
        
        pthread_mutex_unlock(&global_manager->mutex);
        
        // Sleep for 1 second
        sleep(1);
    }
    
    return NULL;
}
