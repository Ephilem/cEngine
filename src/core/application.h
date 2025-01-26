#pragma once
#include "application_types.h"

b8 application_create(app* application_inst);
b8 application_run(void);

void application_get_framebuffer_size(u32* width, u32* height);