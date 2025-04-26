#include "game.h"

#include <core/logger.h>
#include <core/input.h>
#include <core/cmemory.h>
#include <math/cmath.h>
#include <renderer/renderer_frontend.h>

#include "core/event.h"


static u64 alloc_count = 0;

void recalculate_view_matrix(game_state* state) {
    if (!state->camera_updated) return;

    mat4 translation = mat4_translation(state->camera_position);
    mat4 rotation = mat4_euler_xyz(
        state->camera_euler.x,
        state->camera_euler.y,
        state->camera_euler.z);

    state->view = mat4_multiply(rotation, translation);
    state->view = mat4_inverse(state->view);
    state->camera_updated = false;
}

void camera_yaw(game_state* state, f32 amount) {
    state->camera_euler.y += amount;
    state->camera_updated = true;
}

void camera_pitch(game_state* state, f32 amount) {
    state->camera_euler.x += amount;

    // avoid gimbal lock
    f32 limit = deg_to_rad(89.0f);
    state->camera_euler.x = cCLAMP(state->camera_euler.x, -limit, limit);

    state->camera_updated = true;
}

b8 game_initialize(app* application_inst) {
    LOG_INFO("Game initialized");

    game_state* state = (game_state*)application_inst->state;
    state->camera_position = (vec3){0.0f, 0.0f, 30.0f};
    state->camera_euler = (vec3){0.0f, 0.0f, 0.0f};
    state->camera_updated = true;

    return true;
}

b8 game_update(app* application_inst, f32 delta_time) {
    game_state* state = (game_state*)application_inst->state;

    u64 prev_alloc_count = alloc_count;
    alloc_count = get_memory_alloc_count();
    if (input_is_key_down('M') && input_was_key_up('M')) {
        LOG_DEBUG("Allocations: %llu (%llu this frame)", alloc_count, alloc_count - prev_alloc_count);
    }

    if (input_is_key_down('T') && input_was_key_up('T')) {
        event_context context = {};
        event_fire(EVENT_CODE_DEBUG0, application_inst, context);
    }

    if (input_is_key_down('Q')) {
        camera_yaw(state, delta_time * 1.0f);
    }
    if (input_is_key_down('E')) {
        camera_yaw(state, -delta_time * 1.0f);
    }
    if (input_is_key_down(KEY_UP)) {
        camera_pitch(state, delta_time * 1.0f);
    }
    if (input_is_key_down(KEY_DOWN)) {
        camera_pitch(state, -delta_time * 1.0f);
    }

    f32 temp_move_speed = 50.0f;
    vec3 velocity = vec3_zero();

    if (input_is_key_down('W')) {
        vec3 forward = mat4_forward(state->view);
        velocity = vec3_add(velocity, forward);
    }
    if (input_is_key_down('S')) {
        vec3 backward = mat4_backward(state->view);
        velocity = vec3_add(velocity, backward);
    }
    if (input_is_key_down('A')) {
        vec3 left = mat4_left(state->view);
        velocity = vec3_add(velocity, left);
    }
    if (input_is_key_down('D')) {
        vec3 right = mat4_right(state->view);
        velocity = vec3_add(velocity, right);
    }

    if (input_is_key_down(KEY_SPACE)) {
        velocity.y += 1.0f;
    }
    if (input_is_key_down('X')) {
        velocity.y -= 1.0f;
    }

    vec3 z = vec3_zero();
    if (!vec3_compare(z, velocity, 0.0002f)) {
        vec3_normalize(&velocity);
        state->camera_position.x += velocity.x * temp_move_speed * delta_time;
        state->camera_position.y += velocity.y * temp_move_speed * delta_time;
        state->camera_position.z += velocity.z * temp_move_speed * delta_time;
        state->camera_updated = true;
    }

    recalculate_view_matrix(state);


    renderer_set_view(state->view);
    return true;
}

void game_shutdown(app* application_inst) {
    LOG_INFO("Game shutdown");
}

void game_on_resize(app* application_inst, i16 width, i16 height) {
    LOG_INFO("Game resized");
}

