#include "logger.h"
#include "asserts.h"


// temporary implementation
#include <stdarg.h>

#include "cmemory.h"
#include "cstring.h"
#include "platform/filesystem.h"
#include "platform/platform.h"

typedef struct logger_system_state {
    b8 initialized;

    file_handle log_file_handle;
} logger_system_state;

static logger_system_state* state_ptr; // copy to the logger state

b8 initialize_logging(u64* memory_requirement, void* state) {
    *memory_requirement = sizeof(logger_system_state);
    if (state == 0) {
        return false;
    }

    state_ptr = state;
    state_ptr->initialized = true;

    // init the log file
    if (!filesystem_open("console.log", FILE_MODE_WRITE, false, &state_ptr->log_file_handle)) {
        platform_console_write_error("ERROR: Failed to open log file", LOG_LEVEL_ERROR);
        return false;
    }



    return true;
}

void shutdown_logging() {
    // TODO: Cleaning up logging and write queued entries
    state_ptr = 0;
}

void append_to_log_file(const char* message) {
    if (state_ptr->log_file_handle.is_valid) {
        u64 length = string_length(message);
        u64 written = 0;
        if (!filesystem_write(&state_ptr->log_file_handle, length, message, &written)) {
            platform_console_write_error("ERROR: Failed to write to log file", LOG_LEVEL_ERROR);
        }
    }
}

void log_output(log_level level, const char* message, ...) {
    const char* level_strings[6] = {"[FATAL]", "[ERROR]", "[WARN]", "[INFO]", "[DEBUG]", "[TRACE]"};
    b8 is_error = level <= LOG_LEVEL_ERROR;

    char out_message[32000];
    czero_memory(out_message, sizeof(out_message));

    __builtin_va_list args_ptr; // pointer to the arguments of the function
    va_start(args_ptr, message);
    string_format_v(out_message, message, args_ptr);
    va_end(args_ptr);

    string_format(out_message, "%s %s\n", level_strings[level], out_message);

    if (is_error) {
        platform_console_write_error(out_message, level);
    } else {
        platform_console_write(out_message, level);
    }

    append_to_log_file(out_message);
}

void report_assertion_failure(const char* expression, const char* message, const char* file, i32 line) {
    log_output(LOG_LEVEL_FATAL, "\n/!\\ Assertion failed: %s\n%s\n\nFile %s:%d", expression, message, file, line);
}
