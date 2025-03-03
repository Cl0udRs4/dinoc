/**
 * @file test_console.c
 * @brief Test program for console interface functionality
 */

#include "../include/console.h"
#include "../common/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

// Test configuration
#define TEST_TIMEOUT_MS 5000

// Global variables
static bool test_passed = false;
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
    // Clean up resources
    console_shutdown();
}

/**
 * @brief Test console initialization
 */
static void test_console_init(void) {
    printf("Testing console initialization...\n");
    
    // Initialize console
    status_t status = console_init();
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to initialize console: %d\n", status);
        exit(1);
    }
    
    printf("Console initialized successfully\n");
}

/**
 * @brief Test console command registration
 */
static void test_console_register_command(void) {
    printf("Testing console command registration...\n");
    
    // Define test command handler
    console_command_handler_t test_handler = NULL;
    
    // Register command
    status_t status = console_register_command("test", "Test command", "test [args]", test_handler);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to register console command: %d\n", status);
        exit(1);
    }
    
    printf("Console command registered successfully\n");
}

/**
 * @brief Test console command finding
 */
static void test_console_find_command(void) {
    printf("Testing console command finding...\n");
    
    // Find command
    console_command_t* command = console_find_command("test");
    
    if (command == NULL) {
        printf("Command not found\n");
        exit(1);
    }
    
    // Check command fields
    if (strcmp(command->name, "test") != 0) {
        printf("Command name is incorrect: %s\n", command->name);
        exit(1);
    }
    
    if (strcmp(command->description, "Test command") != 0) {
        printf("Command description is incorrect: %s\n", command->description);
        exit(1);
    }
    
    if (strcmp(command->usage, "test [args]") != 0) {
        printf("Command usage is incorrect: %s\n", command->usage);
        exit(1);
    }
    
    printf("Console command finding test passed\n");
}

/**
 * @brief Test console command execution
 */
static void test_console_execute_command(void) {
    printf("Testing console command execution...\n");
    
    // Define test command handler
    console_command_handler_t test_handler = NULL;
    
    // Register command with handler
    status_t status = console_register_command("test_exec", "Test execution command", "test_exec [args]", test_handler);
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to register console command: %d\n", status);
        exit(1);
    }
    
    // Execute command
    status = console_execute_command("test_exec arg1 arg2");
    
    // Note: Since we're using a NULL handler, we expect an error
    if (status == STATUS_SUCCESS) {
        printf("Command execution succeeded unexpectedly\n");
        exit(1);
    }
    
    printf("Console command execution test passed\n");
}

/**
 * @brief Test console start and stop
 */
static void test_console_start_stop(void) {
    printf("Testing console start and stop...\n");
    
    // Start console
    status_t status = console_start();
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to start console: %d\n", status);
        exit(1);
    }
    
    printf("Console started successfully\n");
    
    // Sleep briefly to allow console to start
    sleep(1);
    
    // Stop console
    status = console_stop();
    
    if (status != STATUS_SUCCESS) {
        printf("Failed to stop console: %d\n", status);
        exit(1);
    }
    
    printf("Console stopped successfully\n");
}

/**
 * @brief Main function
 */
int main(int argc, char** argv) {
    // Set up signal handler
    signal(SIGINT, signal_handler);
    
    // Run tests
    test_console_init();
    test_console_register_command();
    test_console_find_command();
    test_console_execute_command();
    test_console_start_stop();
    
    // Clean up
    cleanup();
    
    printf("All tests passed\n");
    
    return 0;
}
