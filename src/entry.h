#pragma once

#include "core/application.h"
#include "core/logger.h"
#include "core/cmemory.h"
#include "app.h"

// extern for externally-defined function to create application (defined elsewhere)
extern b8 create_application(app* application_inst);

/**
 * Main entry point for the application.
 */
int main(void) {

    app app_inst;
    if (!create_application(&app_inst)) {
        LOG_FATAL("Failed to create application");
        return -1;
    }

    if (!app_inst.update || !app_inst.shutdown || !app_inst.on_resize || !app_inst.initialize) {
        LOG_FATAL("Application must implement all required functions");
        return -2;
    }

    if (!application_create(&app_inst)) {
        LOG_FATAL("Failed to create application");
        return 1;
    }

    if (!application_run()) {
        LOG_FATAL("Failed to run application");
        return 2;
    }

    shutdown_memory();

    return 0;
}


