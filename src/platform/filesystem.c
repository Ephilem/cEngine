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

b8 filesystem_exists(const char* path) {
#ifdef _MSC_VER
    struct _stat st;
    return _stat(path, &st);
#else
    struct stat st;
    return stat(path, &st) == 0;
#endif
}

b8 filesystem_open(const char* path, file_modes mode, b8 binary, file_handle* out_handle) {
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

void filesystem_close(file_handle* handle) {
    if (handle->handle) {
        fclose((FILE *) handle->handle);
        handle->handle = 0;
        handle->is_valid = false;
    }
}

b8 filesystem_size(file_handle* handle, u64* out_size) {
    if (handle->handle) {
        fseek((FILE *) handle->handle, 0, SEEK_END);
        *out_size = ftell((FILE *) handle->handle);
        rewind((FILE *) handle->handle);
        return true;
    }
    return false;
}

b8 filesystem_read_line(file_handle* handle, u64 max_length, char** line_buf, u64* out_line_length) {
    if (handle->handle && line_buf && out_line_length && max_length > 0) {
        char* buf = *line_buf;
        if (fgets(buf, max_length, (FILE *) handle->handle) != 0) {
            *out_line_length = strlen(buf);
            return true;
        }
    }
    return false;
}

b8 filesystem_write_line(file_handle* handle, const char* text) {
    if (handle->handle) {
        i32 result = fputs(text, (FILE *) handle->handle);
        if (result != EOF) {
            result = fputc('\n', (FILE *) handle->handle);
        }

        // flush the stream so its written to the file immediately
        fflush((FILE *) handle->handle);
        return result != EOF;
    }
    return false;
}

b8 filesystem_read(file_handle* handle, u64 data_size, void* out_data, u64* out_bytes_read) {
    if (handle->handle && out_data) {
        *out_bytes_read = fread(out_data, 1, data_size, (FILE *) handle->handle);
        if (*out_bytes_read != data_size) {
            return false;
        }
        return true;
    }
    return false;
}

b8 filesystem_read_all_bytes(file_handle* handle, u8* out_bytes, u64* out_bytes_read) {
    if (handle->handle && out_bytes && out_bytes_read) {
        // seek to the end of the file to get the size
        u64 size = 0;
        if (!filesystem_size(handle, &size)) {
            return false;
        }

        *out_bytes_read = fread(out_bytes, 1, size, (FILE *) handle->handle);
        return *out_bytes_read == size;
    }
    return false;
}

b8 filesystem_read_all_text(file_handle* handle, char** out_text, u64* out_bytes_read) {
    if (handle->handle && out_text && out_bytes_read) {
        // seek to the end of the file to get the size
        u64 size = 0;
        if (!filesystem_size(handle, &size)) {
            return false;
        }

        *out_text = (char*)malloc(size + 1);
        if (!*out_text) {
            return false;
        }

        *out_bytes_read = fread(*out_text, 1, size, (FILE *) handle->handle);
        (*out_text)[*out_bytes_read] = '\0';
        return *out_bytes_read == size;
    }
    return false;
}

b8 filesystem_write(file_handle* handle, u64 data_size, const void* data, u64* out_bytes_written) {
    if (handle->handle) {
        *out_bytes_written = fwrite(data, 1, data_size, (FILE *) handle->handle);
        if (*out_bytes_written != data_size) {
            return false;
        }
        fflush((FILE *) handle->handle);
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
