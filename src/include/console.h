/**
 * @file console.h
 * @brief Console interface for C2 server
 */

#ifndef DINOC_CONSOLE_H
#define DINOC_CONSOLE_H

#include "common.h"
#include <stdint.h>
#include <stdbool.h>

// Console command function type
typedef status_t (*console_command_func_t)(int argc, char** argv);

// Console command structure
typedef struct {
    char* name;
    char* description;
    char* usage;
    console_command_func_t handler;
} console_command_t;

/**
 * @brief Initialize console
 * 
 * @return status_t Status code
 */
status_t console_init(void);

/**
 * @brief Start console
 * 
 * @return status_t Status code
 */
status_t console_start(void);

/**
 * @brief Stop console
 * 
 * @return status_t Status code
 */
status_t console_stop(void);

/**
 * @brief Shutdown console
 * 
 * @return status_t Status code
 */
status_t console_shutdown(void);

/**
 * @brief Register console command
 * 
 * @param name Command name
 * @param description Command description
 * @param usage Command usage
 * @param handler Command handler
 * @return status_t Status code
 */
status_t console_register_command(const char* name, const char* description, const char* usage, console_command_func_t handler);

/**
 * @brief Console thread function
 * 
 * @param arg Thread argument
 * @return void* Thread return value
 */
static void* console_thread(void* arg);

/**
 * @brief Parse command line
 * 
 * @param line Command line
 * @param argc Pointer to store argument count
 * @param argv Pointer to store argument array
 * @return status_t Status code
 */
static status_t console_parse_line(char* line, int* argc, char*** argv);

/**
 * @brief Execute command
 * 
 * @param argc Argument count
 * @param argv Argument array
 * @return status_t Status code
 */
static status_t console_execute_command(int argc, char** argv);

/**
 * @brief Free argument array
 * 
 * @param argc Argument count
 * @param argv Argument array
 */
static void console_free_args(int argc, char** argv);

#endif /* DINOC_CONSOLE_H */
