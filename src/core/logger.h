#pragma once

#include "define.h"

#define LOG_LEVEL LOG_LEVEL_DEBUG

typedef enum log_level {
    LOG_LEVEL_FATAL = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4,
    LOG_LEVEL_TRACE = 5
} log_level;

/**
 * Initialize the logging system. call twice; once with state = 0 to get the required memory and
 * then a second time passing allocated memory to state.
 *
 * @param memory_requirement a pointer to the size of memory required for the logging system
 * @param state a pointer to the memory allocated for the logging system
 * @return true if the logging system was initialized successfully, false otherwise
 */
b8 initialize_logging(u64* memory_requirement, void* state);
void shutdown_logging();

void log_output(log_level, const char* message, ...);

#define LOG_TRACE(message, ...) log_output(LOG_LEVEL_TRACE, message, ##__VA_ARGS__)
#define LOG_DEBUG(message, ...) log_output(LOG_LEVEL_DEBUG, message, ##__VA_ARGS__)
#define LOG_INFO(message, ...) log_output(LOG_LEVEL_INFO, message, ##__VA_ARGS__)
#define LOG_WARN(message, ...) log_output(LOG_LEVEL_WARN, message, ##__VA_ARGS__)
#define LOG_ERROR(message, ...) log_output(LOG_LEVEL_ERROR, message, ##__VA_ARGS__)
#define LOG_FATAL(message, ...) log_output(LOG_LEVEL_FATAL, message, ##__VA_ARGS__)
