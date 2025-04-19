#include "filesystem.h"

#include "core/logger.h"
#include "core/cmemory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>

#include "core/cstring.h"

b8 filesystem_exists(const char *path) {
    struct stat st;
    return stat(path, &st) == 0;
}

b8 filesystem_open(const char *path, file_modes mode, b8 binary, file_handle *out_handle) {
    out_handle->is_valid = false;
    out_handle->handle = 0;
    const char* mode_str;

    if ((mode & FILE_MODE_READ) != 0 && (mode & FILE_MODE_WRITE) != 0) {
        mode_str = binary ? "w+b" : "w+";
    } else if ((mode & FILE_MODE_READ) != 0 && (mode & FILE_MODE_WRITE) == 0) {
        mode_str = binary ? "rb" : "r";
    } else if ((mode & FILE_MODE_READ) == 0 && (mode & FILE_MODE_WRITE) != 0) {
        mode_str = binary ? "wb" : "w";
    } else {
        LOG_ERROR("Invalid file mode");
        return false;
    }

    FILE* file = fopen(path, mode_str);
    if (!file) {
        LOG_ERROR("Failed to open file %s in mode %s", path, mode_str);
        return false;
    }

    out_handle->handle = file;
    out_handle->is_valid = true;
    return true;
}

void filesystem_close(file_handle *handle) {
    if (handle->handle) {
        fclose((FILE*) handle->handle);
        handle->handle = 0;
        handle->is_valid = false;
    }
}

b8 filesystem_read_line(file_handle *handle, char **line_buffer) {
    if (handle->is_valid) {
        // we assume a max of 32k char per line
        char buffer[32000];
        if (fgets(buffer, 32000, (FILE*)handle->handle) != 0) {
            u64 length = strlen(buffer);
            *line_buffer = callocate((sizeof(char) * length) + 1, MEMORY_TAG_STRING);
            strcpy(*line_buffer, buffer);
            return true;
        }
    }
    return false;
}

b8 filesystem_write_line(file_handle *handle, const char *text) {
    if (handle->handle) {
        i32 result = fputs(text, (FILE*)handle->handle);
        if (result != EOF) {
            result = fputc('\n', (FILE*)handle->handle);
        }

        // flush the stream so its written to the file immediately
        fflush((FILE*)handle->handle);
        return result != EOF;
    }
    return false;
}

b8 filesystem_read(file_handle *handle, u64 data_size, void *out_data, u64 *out_bytes_read) {
    if (handle->handle && out_data) {
        *out_bytes_read = fread(out_data, 1, data_size, (FILE*)handle->handle);
        if (*out_bytes_read != data_size) {
            return false;
        }
        return true;
    }
    return false;
}

b8 filesystem_read_all_bytes(file_handle *handle, u8 **out_bytes, u64 *out_bytes_read) {
    if (handle->handle) {
        // seek to the end of the file to get the size
        fseek((FILE*)handle->handle, 0, SEEK_END);
        u64 size = ftell((FILE*)handle->handle);
        rewind((FILE*)handle->handle); // go back to the beginning of the file

        *out_bytes = callocate(sizeof(u8) * size, MEMORY_TAG_STRING);
        *out_bytes_read = fread(*out_bytes, 1, size, (FILE*)handle->handle);
        if (*out_bytes_read != size) {
            return false;
        }
        return true;
    }
    return false;
}

b8 filesystem_write(file_handle *handle, u64 data_size, const void *data, u64 *out_bytes_written) {
    if (handle->handle) {
        *out_bytes_written = fwrite(data, 1, data_size, (FILE*)handle->handle);
        if (*out_bytes_written != data_size) {
            return false;
        }
        fflush((FILE*)handle->handle);
        return true;
    }
    return false;
}

b8 filesystem_get_executable_path(char* buffer) {
    u64 len = readlink("/proc/self/exe", buffer, 1024 - 1);
    if (len != -1) {
        buffer[len] = '\0';
        return true;
    }
    return false;
}

b8 filesystem_get_executable_dir(char* buffer) {
    if (filesystem_get_executable_path(buffer)) {
        dirname(buffer);
        return true;
    }
    return false;
}
