/**
 * @file logger.c
 * @brief Implementation of logging system
 */

#include "../include/common.h"
#include <stdarg.h>
#include <time.h>
#include <pthread.h>

// Log file handle
static FILE* log_file_handle = NULL;

// Current log level
static log_level_t current_log_level = LOG_LEVEL_INFO;

// Mutex for thread-safe logging
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Log level strings
static const char* log_level_strings[] = {
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR",
    "CRITICAL"
};

status_t log_init(const char* log_file, log_level_t level) {
    pthread_mutex_lock(&log_mutex);
    
    // Close existing log file if open
    if (log_file_handle != NULL && log_file_handle != stdout) {
        fclose(log_file_handle);
        log_file_handle = NULL;
    }
    
    // Set log level
    current_log_level = level;
    
    // Open log file or use stdout
    if (log_file != NULL) {
        log_file_handle = fopen(log_file, "a");
        if (log_file_handle == NULL) {
            log_file_handle = stdout;
            fprintf(stderr, "Failed to open log file '%s', using stdout\n", log_file);
            pthread_mutex_unlock(&log_mutex);
            return STATUS_ERROR_GENERIC;
        }
    } else {
        log_file_handle = stdout;
    }
    
    // Log initialization message
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_str[26];
    strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    
    fprintf(log_file_handle, "[%s] [INFO] Logging initialized (level: %s)\n", 
            time_str, log_level_strings[level]);
    fflush(log_file_handle);
    
    pthread_mutex_unlock(&log_mutex);
    return STATUS_SUCCESS;
}

void log_message(log_level_t level, const char* file, int line, const char* fmt, ...) {
    // Check if we should log this message
    if (level < current_log_level) {
        return;
    }
    
    pthread_mutex_lock(&log_mutex);
    
    // Ensure log file is open
    if (log_file_handle == NULL) {
        log_file_handle = stdout;
    }
    
    // Get current time
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_str[26];
    strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    
    // Print log header
    fprintf(log_file_handle, "[%s] [%s] [%s:%d] ", 
            time_str, log_level_strings[level], file, line);
    
    // Print formatted message
    va_list args;
    va_start(args, fmt);
    vfprintf(log_file_handle, fmt, args);
    va_end(args);
    
    // Add newline if not present
    if (fmt[strlen(fmt) - 1] != '\n') {
        fprintf(log_file_handle, "\n");
    }
    
    // Flush to ensure message is written
    fflush(log_file_handle);
    
    pthread_mutex_unlock(&log_mutex);
}

status_t log_shutdown(void) {
    pthread_mutex_lock(&log_mutex);
    
    // Log shutdown message
    if (log_file_handle != NULL) {
        time_t now = time(NULL);
        struct tm* tm_info = localtime(&now);
        char time_str[26];
        strftime(time_str, 26, "%Y-%m-%d %H:%M:%S", tm_info);
        
        fprintf(log_file_handle, "[%s] [INFO] Logging shutdown\n", time_str);
        
        // Close log file if not stdout
        if (log_file_handle != stdout) {
            fclose(log_file_handle);
        }
        
        log_file_handle = NULL;
    }
    
    pthread_mutex_unlock(&log_mutex);
    return STATUS_SUCCESS;
}
