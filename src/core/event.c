#include "core/event.h"

#include "core/cmemory.h"
#include "core/logger.h"
#include "containers/darray.h"

typedef struct registered_event {
    void *listener;
    PFN_on_event callback;
} registered_event;

typedef struct event_code_entry {
    registered_event *events;
} event_code_entry;

#define MAX_MESSAGE_CODES 16384

typedef struct event_system_state {
    event_code_entry registered[MAX_MESSAGE_CODES];
} event_system_state;

/**
* Event system internal state
*/

static event_system_state *state_ptr;


b8 initialize_event(u64 *memory_requirement, void *state) {
    *memory_requirement = sizeof(event_system_state);
    if (!state) {
        return false;
    }

    if (state_ptr) {
        LOG_WARN("Event system already initialized. Do not call initialize_event more than once.");
        return false;
    }

    state_ptr = state;
    czero_memory(state_ptr, sizeof(event_system_state));

    return true;
}

void event_shutdown() {
    // free the events arrays. And objects poitned to should be destroyed on their own
    for (u16 i = 0; i < MAX_MESSAGE_CODES; ++i) {
        if (state_ptr->registered[i].events != 0) {
            darray_destroy(state_ptr->registered[i].events);
            state_ptr->registered[i].events = 0;
        }
    }
}

b8 event_register(u16 code, void *listener, PFN_on_event on_event) {
    if (!state_ptr) {
        return false;
    }

    // create the array if it doesn't exist
    if (state_ptr->registered[code].events == 0) {
        state_ptr->registered[code].events = darray_create(registered_event);
    }

    // check if the listener is already registered
    u64 registered_count = darray_length(state_ptr->registered[code].events);
    for (u64 i = 0; i < registered_count; ++i) {
        registered_event *event = &state_ptr->registered[code].events[i];
        if (event->listener == listener && event->callback == on_event) {
            LOG_WARN("Event listener already registered");
            return false;
        }
    }

    // register
    registered_event event = {listener, on_event};
    darray_push(state_ptr->registered[code].events, event);

    LOG_TRACE("Event listener registered for code %d (callback: %p)", code, on_event);

    return true;
}

b8 event_unregister(u16 code, void *listener, PFN_on_event on_event) {
    if (!state_ptr) {
        return false;
    }

    // if nothign is registered of the code, return false
    if (state_ptr->registered[code].events == 0) {
        LOG_WARN("No events registered for code %d", code);
        return false;
    }

    // check if the listener is already registered
    u64 registered_count = darray_length(state_ptr->registered[code].events);
    for (u64 i = 0; i < registered_count; ++i) {
        registered_event event = state_ptr->registered[code].events[i];
        if (event.listener == listener && event.callback == on_event) {
            registered_event popped;
            darray_pop_at(state_ptr->registered[code].events, i, &popped);
            return true;
        }
    }

    // not found
    return false;
}

b8 event_fire(u16 code, void *sender, event_context context) {
    if (!state_ptr) {
        return false;
    }

    // if nothing is registered of the code, return false
    if (state_ptr->registered[code].events == 0) {
        return false;
    }

    // fire the events
    u64 registered_count = darray_length(state_ptr->registered[code].events);
    for (u64 i = 0; i < registered_count; ++i) {
        registered_event event = state_ptr->registered[code].events[i];
        if (event.callback(code, sender, event.listener, context)) {
            return true;
        }
    }

    return false;
}








