#pragma once
#include "define.h"

typedef struct app app;

typedef struct application_configuration {
    i16 start_pos_x;
    i16 start_pos_y;
    i16 window_width;
    i16 window_height;
    char* window_title;
} application_configuration;