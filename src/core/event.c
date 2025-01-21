#include "core/event.h"

#include "core/cmemory.h"
#include "core/logger.h"
#include "containers/darray.h"

typedef struct registered_event {
	void* listener;
	PFN_on_event callback;
} registered_event;

typedef struct event_code_entry {
	registered_event* events;
} event_code_entry;

#define MAX_MESSAGE_CODES 16384

typedef struct event_system_state {
    event_code_entry registered[MAX_MESSAGE_CODES];
} event_system_state;

/**
* Event system internal state
*/

static b8 is_initialized = FALSE;
static event_system_state state;


b8 initialize_event() {
  if (is_initialized == TRUE) {
	return FALSE;
  }

  is_initialized = FALSE;
  czero_memory(&state, sizeof(event_system_state));

  is_initialized = TRUE;

  return TRUE;
}

void event_shutdown() {
	// free the events arrays. And objects poitned to should be destroyed on their own
	for (u16 i = 0; i < MAX_MESSAGE_CODES; ++i) {
		if (state.registered[i].events != 0) {
			darray_destroy(state.registered[i].events);
            state.registered[i].events = 0;
		}
	}
}

b8 event_register(u16 code, void* listener, PFN_on_event on_event) {
	if (is_initialized == FALSE) {
		return FALSE;
	}

    // create the array if it doesn't exist
	if (state.registered[code].events == 0) {
		state.registered[code].events = darray_create(sizeof(registered_event));
	}

    // check if the listener is already registered
	u64 registered_count = darray_length(state.registered[code].events);
	for (u64 i = 0; i < registered_count; ++i) {
        registered_event* event = &state.registered[code].events[i];
        if (event->listener == listener && event->callback == on_event) {
            LOG_WARN("Event listener already registered");
            return FALSE;
        }
    }

    // register
    registered_event event = { listener, on_event };
    darray_push(state.registered[code].events, event);

    return TRUE;
}

b8 event_unregister(u16 code, void* listener, PFN_on_event on_event) {
    if (is_initialized == FALSE) {
        return FALSE;
    }

    // if nothign is registered of the code, return false
    if (state.registered[code].events == 0) {
        LOG_WARN("No events registered for code %d", code);
        return FALSE;
    }

    // check if the listener is already registered
    u64 registered_count = darray_length(state.registered[code].events);
    for (u64 i = 0; i < registered_count; ++i) {
        registered_event event = state.registered[code].events[i];
        if (event.listener == listener && event.callback == on_event) {
            registered_event popped;
            darray_pop_at(state.registered[code].events, i, &popped);
            return TRUE;
        }
    }

    // not found
    return FALSE;
}

b8 event_fire(u16 code, void* sender, event_context context) {
    if (is_initialized == FALSE) {
        return FALSE;
    }

    // if nothign is registered of the code, return false
    if (state.registered[code].events == 0) {
        return FALSE;
    }

    // fire the events
    u64 registered_count = darray_length(state.registered[code].events);
    for (u64 i = 0; i < registered_count; ++i) {
        registered_event event = state.registered[code].events[i];
        if (event.callback(code, sender, event.listener, context)) {
            return TRUE;
        }
    }

    return FALSE;
}








