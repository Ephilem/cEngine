#include "core/input.h"

#include "core/event.h"
#include "core/cmemory.h"
#include "core/logger.h"

typedef struct keyboard_state {
    b8 keys[256];
} keyboard_state;

typedef struct mouse_state {
    i16 x;
    i16 y;
    u8 buttons[BUTTON_MAX_BUTTONS];
} mouse_state;

typedef struct input_state {
    keyboard_state keyboard_current;
    keyboard_state keyboard_previous;

    mouse_state mouse_current;
    mouse_state mouse_previous;
} input_state;

static b8 initialized = FALSE;
static input_state state = {};

void initialize_input() {
    if (initialized) {
        LOG_WARN("Input already initialized. Do not call initialize_input more than once.");
        return;
    }

    czero_memory(&state, sizeof(input_state));
    initialized = TRUE;
    LOG_INFO("Input initialized");
}

void shutdown_input() {
    if (!initialized) {
        LOG_WARN("Input not initialized. Do not call shutdown_input before calling initialize_input.");
        return;
    }

    initialized = FALSE;
}

void update_input(f64 delta_time) {
    if (!initialized) {
        LOG_WARN("Input not initialized. Do not call update_input before calling initialize_input.");
        return;
    }

    // copy the current state to the previous state
    ccopy_memory(&state.keyboard_previous, &state.keyboard_current, sizeof(keyboard_state));
    ccopy_memory(&state.mouse_previous, &state.mouse_current, sizeof(mouse_state));
}

void input_process_key(keys key, b8 pressed) {
    if (!initialized) {
        LOG_WARN("Input not initialized. Do not call input_process_key before calling initialize_input.");
        return;
    }

    // only handle if the key state actually changed
    if (state.keyboard_current.keys[key] != pressed) {
        state.keyboard_current.keys[key] = pressed;

        event_context context;
        context.data.u16[0] = key;
        event_fire(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, 0, context);
    }
}

void input_process_button(buttons button, b8 pressed) {
    if (!initialized) {
        LOG_WARN("Input not initialized. Do not call input_process_button before calling initialize_input.");
        return;
    }

    // only handle if the button state actually changed
    if (state.mouse_current.buttons[button] != pressed) {
        state.mouse_current.buttons[button] = pressed;

        event_context context;
        context.data.u16[0] = button;
        event_fire(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, 0, context);
    }
}

void input_process_mouse_move(i16 x, i16 y) {
    if (!initialized) {
        LOG_WARN("Input not initialized. Do not call input_process_mouse_move before calling initialize_input.");
        return;
    }

    if (state.mouse_current.x != x || state.mouse_current.y != y) {
        state.mouse_current.x = x;
        state.mouse_current.y = y;

        event_context context;
        context.data.u16[0] = x;
        context.data.u16[1] = y;
        event_fire(EVENT_CODE_MOUSE_MOVED, 0, context);
    }
}

void input_process_mouse_wheel(i8 z_delta) {
    if (!initialized) {
        LOG_WARN("Input not initialized. Do not call input_process_mouse_wheel before calling initialize_input.");
        return;
    }

    event_context context;
    context.data.u8[0] = z_delta;
    event_fire(EVENT_CODE_MOUSE_WHEEL, 0, context);
}

b8 input_is_key_down(keys key) {
    if (!initialized) {
        LOG_WARN("Input not initialized. Do not call input_is_key_down before calling initialize_input.");
        return FALSE;
    }

    return state.keyboard_current.keys[key] == TRUE;
}

b8 input_is_key_up(keys key) {
    if (!initialized) {
        LOG_WARN("Input not initialized. Do not call input_is_key_up before calling initialize_input.");
        return TRUE;
    }

    return state.keyboard_current.keys[key] == FALSE;
}

b8 input_was_key_down(keys key) {
    if (!initialized) {
        LOG_WARN("Input not initialized. Do not call input_was_key_down before calling initialize_input.");
        return FALSE;
    }

    return state.keyboard_previous.keys[key] == TRUE;
}

b8 input_was_key_up(keys key) {
    if (!initialized) {
        LOG_WARN("Input not initialized. Do not call input_was_key_up before calling initialize_input.");
        return TRUE;
    }

    return state.keyboard_previous.keys[key] == FALSE;
}

b8 input_is_button_down(buttons button) {
    if (!initialized) {
        LOG_WARN("Input not initialized. Do not call input_is_button_down before calling initialize_input.");
        return FALSE;
    }

    return state.mouse_current.buttons[button] == TRUE;
}

b8 input_is_button_up(buttons button) {
    if (!initialized) {
        LOG_WARN("Input not initialized. Do not call input_is_button_up before calling initialize_input.");
        return TRUE;
    }

    return state.mouse_current.buttons[button] == FALSE;
}


void input_get_mouse_pos(i32* x, i32* y) {
    if (!initialized) {
        LOG_WARN("Input not initialized. Do not call input_get_mouse_pos before calling initialize_input.");
        *x = 0;
        *y = 0;
        return;
    }

    *x = state.mouse_current.x;
    *y = state.mouse_current.y;
}

void input_get_previous_mouse_pos(i32* x, i32* y) {
    if (!initialized) {
        LOG_WARN("Input not initialized. Do not call input_get_previous_mouse_pos before calling initialize_input.");
        *x = 0;
        *y = 0;
        return;
    }

    *x = state.mouse_previous.x;
    *y = state.mouse_previous.y;
}






