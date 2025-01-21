#pragma once

#include "define.h"


/**
 * Appends the names of required externsion for this platform to
 * the extensions list.
 * @param extensions darray of extension names
 */
void platform_get_required_extension_names(const char*** extensions);
