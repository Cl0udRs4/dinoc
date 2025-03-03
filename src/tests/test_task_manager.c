/**
 * @file test_task_manager.c
 * @brief Test program for task management system
 */

#include "../include/task.h"
#include "../include/common.h"
#include "../include/client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

// Test configuration
#define TEST_TIMEOUT 2 // 2 seconds

// Global variables
static bool test_completed = false;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// Forward declarations
static void cleanup(void);

/**
 * @brief Signal handler
 */
static void signal_handler(int sig) {
    printf("Signal %d received, cleaning up...\n", sig);
    cleanup();
    exit(1);
}

/**
 * @brief Clean up resources
 */
static void cleanup(void) {
    task_manager_shutdown();
}

/**
 * @brief Test task creation and state transitions
 */
static void test_task_lifecycle(void) {
    printf("Testing task lifecycle...\n");
    
    // Create client ID
    uuid_t client_id;
    uuid_generate(&client_id);
    
    // Create task data
    const char* data_str = "echo \"hello world\"";
    uint8_t* data = (uint8_t*)strdup(data_str);
    size_t data_len = strlen(data_str);
    
    // Create task
    task_t* task = NULL;
    status_t status = task_create(&client_id, TASK_TYPE_SHELL, data, data_len, 0, &task);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to create task: %d\n", status);
        free(data);
        exit(1);
    }
    
    // Check task state
    if (task->state != TASK_STATE_CREATED) {
        printf("Unexpected task state: %d\n", task->state);
        free(data);
        exit(1);
    }
    
    // Update task state to SENT
    status = task_update_state(task, TASK_STATE_SENT);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to update task state: %d\n", status);
        free(data);
        exit(1);
    }
    
    // Check task state
    if (task->state != TASK_STATE_SENT) {
        printf("Unexpected task state: %d\n", task->state);
        free(data);
        exit(1);
    }
    
    // Update task state to RUNNING
    status = task_update_state(task, TASK_STATE_RUNNING);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to update task state: %d\n", status);
        free(data);
        exit(1);
    }
    
    // Check task state
    if (task->state != TASK_STATE_RUNNING) {
        printf("Unexpected task state: %d\n", task->state);
        free(data);
        exit(1);
    }
    
    // Set task result
    const char* result_str = "hello world";
    uint8_t* result = (uint8_t*)strdup(result_str);
    size_t result_len = strlen(result_str);
    
    status = task_set_result(task, result, result_len);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to set task result: %d\n", status);
        free(data);
        free(result);
        exit(1);
    }
    
    // Check task state
    if (task->state != TASK_STATE_COMPLETED) {
        printf("Unexpected task state: %d\n", task->state);
        free(data);
        free(result);
        exit(1);
    }
    
    // Find task by ID
    task_t* found_task = task_find(&task->id);
    
    if (found_task == NULL) {
        printf("Failed to find task\n");
        free(data);
        free(result);
        exit(1);
    }
    
    // Check task data
    if (found_task->data_len != data_len ||
        memcmp(found_task->data, data, data_len) != 0) {
        printf("Task data mismatch\n");
        free(data);
        free(result);
        exit(1);
    }
    
    // Check task result
    if (found_task->result_len != result_len ||
        memcmp(found_task->result, result, result_len) != 0) {
        printf("Task result mismatch\n");
        free(data);
        free(result);
        exit(1);
    }
    
    // Get tasks for client
    task_t** tasks = NULL;
    size_t task_count = 0;
    
    status = task_get_for_client(&client_id, &tasks, &task_count);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to get tasks for client: %d\n", status);
        free(data);
        free(result);
        exit(1);
    }
    
    // Check task count
    if (task_count != 1) {
        printf("Unexpected task count: %zu\n", task_count);
        free(data);
        free(result);
        free(tasks);
        exit(1);
    }
    
    // Check task
    if (tasks[0] != task) {
        printf("Task mismatch\n");
        free(data);
        free(result);
        free(tasks);
        exit(1);
    }
    
    // Clean up
    free(data);
    free(result);
    free(tasks);
    
    printf("Task lifecycle test completed successfully\n");
}

/**
 * @brief Test task timeout
 */
static void test_task_timeout(void) {
    printf("Testing task timeout...\n");
    
    // Create client ID
    uuid_t client_id;
    uuid_generate(&client_id);
    
    // Create task data
    const char* data_str = "sleep 10";
    uint8_t* data = (uint8_t*)strdup(data_str);
    size_t data_len = strlen(data_str);
    
    // Create task with timeout
    task_t* task = NULL;
    status_t status = task_create(&client_id, TASK_TYPE_SHELL, data, data_len, TEST_TIMEOUT, &task);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to create task: %d\n", status);
        free(data);
        exit(1);
    }
    
    // Update task state to SENT
    status = task_update_state(task, TASK_STATE_SENT);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to update task state: %d\n", status);
        free(data);
        exit(1);
    }
    
    // Wait for timeout
    printf("Waiting for task timeout (%d seconds)...\n", TEST_TIMEOUT);
    sleep(TEST_TIMEOUT + 1);
    
    // Find task by ID
    task_t* found_task = task_find(&task->id);
    
    if (found_task == NULL) {
        printf("Failed to find task\n");
        free(data);
        exit(1);
    }
    
    // Check task state
    if (found_task->state != TASK_STATE_TIMEOUT) {
        printf("Unexpected task state: %d\n", found_task->state);
        free(data);
        exit(1);
    }
    
    // Clean up
    free(data);
    
    printf("Task timeout test completed successfully\n");
}

/**
 * @brief Test task error
 */
static void test_task_error(void) {
    printf("Testing task error...\n");
    
    // Create client ID
    uuid_t client_id;
    uuid_generate(&client_id);
    
    // Create task data
    const char* data_str = "invalid command";
    uint8_t* data = (uint8_t*)strdup(data_str);
    size_t data_len = strlen(data_str);
    
    // Create task
    task_t* task = NULL;
    status_t status = task_create(&client_id, TASK_TYPE_SHELL, data, data_len, 0, &task);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to create task: %d\n", status);
        free(data);
        exit(1);
    }
    
    // Update task state to SENT
    status = task_update_state(task, TASK_STATE_SENT);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to update task state: %d\n", status);
        free(data);
        exit(1);
    }
    
    // Update task state to RUNNING
    status = task_update_state(task, TASK_STATE_RUNNING);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to update task state: %d\n", status);
        free(data);
        exit(1);
    }
    
    // Set task error
    const char* error_str = "Command not found";
    
    status = task_set_error(task, error_str);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to set task error: %d\n", status);
        free(data);
        exit(1);
    }
    
    // Check task state
    if (task->state != TASK_STATE_FAILED) {
        printf("Unexpected task state: %d\n", task->state);
        free(data);
        exit(1);
    }
    
    // Check task error
    if (task->error_message == NULL || strcmp(task->error_message, error_str) != 0) {
        printf("Task error mismatch\n");
        free(data);
        exit(1);
    }
    
    // Clean up
    free(data);
    
    printf("Task error test completed successfully\n");
}

/**
 * @brief Main function
 */
int main(int argc, char** argv) {
    // Set up signal handler
    signal(SIGINT, signal_handler);
    
    // Initialize task manager
    status_t status = task_manager_init();
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to initialize task manager: %d\n", status);
        return 1;
    }
    
    // Run tests
    test_task_lifecycle();
    test_task_timeout();
    test_task_error();
    
    // Clean up
    cleanup();
    
    printf("All tests completed successfully\n");
    
    return 0;
}
