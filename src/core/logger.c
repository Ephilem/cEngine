#include "logger.h"
#include "asserts.h"


// temporary implementation
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "platform/platform.h"


b8 initialize_logging() {
    LOG_INFO("Initializing logging");
    // TODO: create log file
    return TRUE;
}

void shutdown_logging() {
    // TODO: Cleaning up logging and write queued entries
}

void log_output(log_level level, const char* message, ...) {
    const char* level_strings[6] = {"[FATAL]", "[ERROR]", "[WARN]", "[INFO]", "[DEBUG]", "[TRACE]"};
    b8 is_error = level <= LOG_LEVEL_ERROR;

    const i32 message_length = 32000;
    char out_message[message_length];
    memset(out_message, 0, sizeof(out_message));

    __builtin_va_list arg_ptr; // pointer to the list of arguments
    va_start(arg_ptr, message); // initialize the list of arguments
    vsnprintf(out_message, message_length, message, arg_ptr); // write formatted output to the string
    va_end(arg_ptr); // end using the list of arguments

    char temp[message_length];
    sprintf(temp, "%s\t %s", level_strings[level], out_message);

    if (is_error) {
        platform_console_write_error(temp, level);
    } else {
        platform_console_write(temp, level);
    }
}

void report_assertion_failure(const char* expression, const char* message, const char* file, i32 line) {
    log_output(LOG_LEVEL_FATAL, "\n/!\\ Assertion failed: %s\n%s\n\nFile %s:%d", expression, message, file, line);
}
