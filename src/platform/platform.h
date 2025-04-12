#pragma once

#include "define.h"

// La structure platform_state est utilisée pour stocker l'état de la fenêtre active
typedef struct platform_state {
    void* internal_state;
    
    // Ces informations sont nécessaires lors de l'initialisation
    const char* window_title;
    i32 x;
    i32 y;
    i32 width;
    i32 height;
} platform_state;

b8 initialize_platform(u64* memory_requirement, void* state);
void shutdown_platform();

b8 create_window(platform_state* state);

b8 platform_pump_messages(platform_state* state);

// Platform specific functions
void* platform_allocate(u64 size, b8 aligned);
void platform_free(void* block, b8 aligned);
void* platform_zero_memory(void* block, u64 size);
void* platform_copy_memory(void* dest, const void* source, u64 size);
void* platform_set_memory(void* dest, i32 value, u64 size);

void platform_console_write(const char* message, u8 color);
void platform_console_write_error(const char* message, u8 color);

f64 platform_get_absolute_time();

void platform_sleep_ms(u64 ms);