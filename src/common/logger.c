/**
 * @file logger.c
 * @brief Logging system implementation for C2 server
 */

#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

// Logger state
static FILE* log_file = NULL;
static log_level_t log_level = LOG_LEVEL_INFO;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool logger_initialized = false;

// Level names
static const char* level_names[] = {
    "TRACE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

// Level colors (ANSI escape codes)
static const char* level_colors[] = {
    "\x1b[90m", // TRACE: Bright Black
    "\x1b[36m", // DEBUG: Cyan
    "\x1b[32m", // INFO: Green
    "\x1b[33m", // WARN: Yellow
    "\x1b[31m", // ERROR: Red
    "\x1b[35m", // FATAL: Magenta
};

// Reset color
static const char* color_reset = "\x1b[0m";

/**
 * @brief Initialize logger
 */
status_t logger_init(const char* log_file_path, log_level_t level) {
    pthread_mutex_lock(&log_mutex);
    
    if (logger_initialized) {
        pthread_mutex_unlock(&log_mutex);
        return STATUS_ERROR_ALREADY_RUNNING;
    }
    
    // Set log level
    log_level = level;
    
    // Open log file if specified
    if (log_file_path != NULL) {
        log_file = fopen(log_file_path, "a");
        if (log_file == NULL) {
            pthread_mutex_unlock(&log_mutex);
            return STATUS_ERROR;
        }
    } else {
        log_file = stdout;
    }
    
    logger_initialized = true;
    
    // Log initialization message
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Logger initialized with level %s", level_names[level]);
    
    pthread_mutex_unlock(&log_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Shutdown logger
 */
status_t logger_shutdown(void) {
    pthread_mutex_lock(&log_mutex);
    
    if (!logger_initialized) {
        pthread_mutex_unlock(&log_mutex);
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    // Log shutdown message
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Logger shutting down");
    
    // Close log file if not stdout
    if (log_file != NULL && log_file != stdout) {
        fclose(log_file);
        log_file = NULL;
    }
    
    logger_initialized = false;
    
    pthread_mutex_unlock(&log_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Set log level
 */
status_t logger_set_level(log_level_t level) {
    pthread_mutex_lock(&log_mutex);
    
    if (!logger_initialized) {
        pthread_mutex_unlock(&log_mutex);
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    log_level = level;
    
    // Log level change message
    logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Log level changed to %s", level_names[level]);
    
    pthread_mutex_unlock(&log_mutex);
    
    return STATUS_SUCCESS;
}

/**
 * @brief Log message
 */
status_t logger_log(log_level_t level, const char* file, int line, const char* func, const char* format, ...) {
    if (level < log_level) {
        return STATUS_SUCCESS;
    }
    
    va_list args;
    va_start(args, format);
    status_t status = logger_logv(level, file, line, func, format, args);
    va_end(args);
    
    return status;
}

/**
 * @brief Log message with va_list
 */
status_t logger_logv(log_level_t level, const char* file, int line, const char* func, const char* format, va_list args) {
    if (level < log_level) {
        return STATUS_SUCCESS;
    }
    
    pthread_mutex_lock(&log_mutex);
    
    if (!logger_initialized) {
        pthread_mutex_unlock(&log_mutex);
        return STATUS_ERROR_NOT_RUNNING;
    }
    
    // Get current time
    time_t now = time(NULL);
    struct tm* timeinfo = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    // Get thread ID
    pthread_t thread_id = pthread_self();
    
    // Extract filename from path
    const char* filename = strrchr(file, '/');
    if (filename != NULL) {
        filename++;
    } else {
        filename = file;
    }
    
    // Print log header
    if (log_file == stdout) {
        // Use colors for stdout
        fprintf(log_file, "%s [%s] %s%-5s%s [%lu] %s:%d (%s): ",
                time_str, "DinoC", level_colors[level], level_names[level], color_reset,
                (unsigned long)thread_id, filename, line, func);
    } else {
        // No colors for file
        fprintf(log_file, "%s [%s] %-5s [%lu] %s:%d (%s): ",
                time_str, "DinoC", level_names[level],
                (unsigned long)thread_id, filename, line, func);
    }
    
    // Print log message
    vfprintf(log_file, format, args);
    fprintf(log_file, "\n");
    
    // Flush log file
    fflush(log_file);
    
    pthread_mutex_unlock(&log_mutex);
    
    return STATUS_SUCCESS;
}
