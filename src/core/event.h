#pragma once

#include "define.h"


typedef struct event_context {
    // max size of event data is 128 bytes
    union {
        i64 i64[2];
        u64 u64[2];
        f64 f64[2];

        i32 i32[4];
        u32 u32[4];
        f32 f32[4];

        i16 i16[8];
        u16 u16[8];

        i8 i8[16];
        u8 u8[16];

        char c[16];
    } data;
} event_context;

// Return true if the event was handled and should not be propagated further
typedef b8 (*PFN_on_event)(u16 code, void* sender, void* listener_inst, event_context data);

b8 initialize_event(u64* memory_requirement, void* state);
void event_shutdown();

/**
 * Register to listen for when events are sent with the given code. Events with duplicate
 * listener/callback combos will not be registered again and will cause this to return false
 * @param code The event code to listen for
 * @param listener The listener instance that will be passed to the callback
 * @param on_event The callback to be called when the event is sent
 * @return true if the event was registered, false if the event was not registered
 */
b8 event_register(u16 code, void* listener, PFN_on_event on_event);

/**
 * Unregister a listener from an event are sent with the given code. If no matching
 * registration is found, this function returns false.
 * @param code The event code to unregister from
 * @param listener The listener instance that was passed to the callback
 * @param on_event The callback that was called when the event was sent
 * @return true if the event was unregistered, false if the event was not unregistered
 */
b8 event_unregister(u16 code, void* listener, PFN_on_event on_event);

b8 event_fire(u16 code, void* sender, event_context data);

typedef enum system_event_code {
    // Shutdown the application
    EVENT_CODE_APPLICATION_QUIT = 0x01,

    // Keyboard key pressed
    /* Context Usage :
     *  - u16 key_code = data.data.u16[0];
     */
    EVENT_CODE_KEY_PRESSED      = 0x02,

    // Keyboard key released
    /* Context Usage :
     *  - u16 key_code = data.data.u16[0];
     */
    EVENT_CODE_KEY_RELEASED     = 0x03,

    // Mouse button pressed
    /* Context Usage :
     *  - u16 button = data.data.u16[0];
     */
    EVENT_CODE_BUTTON_PRESSED   = 0x04,

    // Mouse button released
    /* Context Usage :
     *  - u16 button = data.data.u16[0];
     */
    EVENT_CODE_BUTTON_RELEASED  = 0x05,

    // Mouse moved
    /* Context Usage :
     *  - i16 x = data.data.i16[0];
     *  - i16 y = data.data.i16[1];
     */
    EVENT_CODE_MOUSE_MOVED      = 0x06,

    // Mouse wheel scrolled
    /* Context Usage :
     *  - i16 z_delta = data.data.i16[0];
     */
    EVENT_CODE_MOUSE_WHEEL      = 0x07,

    // Window resized
    /* Context Usage :
     *  - i16 width = data.data.i16[0];
     *  - i16 height = data.data.i16[1];
     */
    EVENT_CODE_WINDOW_RESIZE    = 0x08,


    MAX_EVENT_SYSTEM_CODE       = 0xFF,
} system_event_code;