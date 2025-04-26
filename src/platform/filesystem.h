#pragma once

#include "define.h"

typedef struct file_handle {
    void* handle;
    b8 is_valid;
} file_handle;

typedef enum file_modes {
    FILE_MODE_READ = 0x1,
    FILE_MODE_WRITE = 0x2,
} file_modes;

/**
 * check if a file exists in the filesystem
 * @param path path to the file to check
 * @return true if the file exists, false otherwise
 */
b8 filesystem_exists(const char* path);

/**
 * Open a file in the filesystem
 * @param path path to the file to oen
 * @param mode mode to open the file in
 * @param binary true if the file should be opened in binary mode
 * @param out_handle pointer to the file handle to be filled
 * @return true if the file was opened successfully, false otherwise
 */
b8 filesystem_open(const char* path, file_modes mode, b8 binary, file_handle* out_handle);

void filesystem_close(file_handle* handle);

/**
 * Read up to a newline or EOF
 * @param handle A pointer to a file hadle structure.
 * @param max_length max length to be read from the line
 * @param line_buf A pointer to a character away populated by this method. Must already be -allocated-!
 * @param out_line_length A pointer to a u64 variable that will be set to the length of the line read
 * @return true if the line was read successfully, false otherwise
 */
b8 filesystem_read_line(file_handle* handle, u64 max_length, char** line_buf, u64* out_line_length);

/**
 * Writes text to the provided file, appending a new line at the end
 * @param handle file handle to write to
 * @param line_buffer buffer containing the text to write
 * @return true if the line was written successfully, false otherwise
 */
b8 filesystem_write_line(file_handle* handle, const char* text);

/**
 * Reads up to data_size bytes from the file into the provided buffer
 * -Allocate- *out_data, which must be freed!
 * @param handle file handle to read from
 * @param data_size size of the data to read
 * @param out_data buffer to store the read data
 * @param out_bytes_read pointer to store the number of bytes read
 */
b8 filesystem_read(file_handle* handle, u64 data_size, void* out_data, u64* out_bytes_read);

/**
 * Read all bytes from the file into out_bytes. The out_bytes_read will be set to the number of bytes read.
 * -Allocate- *out_bytes, which must be freed!
 * @param handle file handle to read from
 * @param out_bytes a pointer to a byte array which will be allocatedf and filled with the file data
 * @param out_bytes_read pointer to store the number of bytes read
 * @return true if the bytes were read successfully, false otherwise
 */
b8 filesystem_read_all_bytes(file_handle* handle, u8** out_bytes, u64* out_bytes_read);

b8 filesystem_write(file_handle* handle, u64 data_size, const void* data, u64* out_bytes_written);

b8 filesystem_get_executable_path(char* out_path);

b8 filesystem_get_executable_dir(char* out_path);