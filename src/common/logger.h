/**
 * @file logger.h
 * @brief Logging system for C2 server
 */

#ifndef DINOC_LOGGER_H
#define DINOC_LOGGER_H

#include "../include/common.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

/**
 * @brief Log level enumeration
 */
typedef enum {
    LOG_LEVEL_TRACE = 0,
    LOG_LEVEL_DEBUG = 1,
    LOG_LEVEL_INFO = 2,
    LOG_LEVEL_WARN = 3,
    LOG_LEVEL_ERROR = 4,
    LOG_LEVEL_FATAL = 5
} log_level_t;

/**
 * @brief Initialize logger
 * 
 * @param log_file Path to log file (NULL for stdout)
 * @param level Minimum log level
 * @return status_t Status code
 */
status_t logger_init(const char* log_file, log_level_t level);

/**
 * @brief Shutdown logger
 * 
 * @return status_t Status code
 */
status_t logger_shutdown(void);

/**
 * @brief Set log level
 * 
 * @param level New log level
 * @return status_t Status code
 */
status_t logger_set_level(log_level_t level);

/**
 * @brief Log message
 * 
 * @param level Log level
 * @param file Source file
 * @param line Source line
 * @param func Source function
 * @param format Format string
 * @param ... Format arguments
 * @return status_t Status code
 */
status_t logger_log(log_level_t level, const char* file, int line, const char* func, const char* format, ...);

/**
 * @brief Log message with va_list
 * 
 * @param level Log level
 * @param file Source file
 * @param line Source line
 * @param func Source function
 * @param format Format string
 * @param args Format arguments
 * @return status_t Status code
 */
status_t logger_logv(log_level_t level, const char* file, int line, const char* func, const char* format, va_list args);

// Convenience macros
#define LOG_TRACE(format, ...) logger_log(LOG_LEVEL_TRACE, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define LOG_DEBUG(format, ...) logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)  logger_log(LOG_LEVEL_INFO,  __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define LOG_WARN(format, ...)  logger_log(LOG_LEVEL_WARN,  __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define LOG_FATAL(format, ...) logger_log(LOG_LEVEL_FATAL, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#endif /* DINOC_LOGGER_H */
