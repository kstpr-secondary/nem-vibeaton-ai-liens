#include "input.h"
#include "engine.h"

#include <sokol_app.h>

#include <cstring>

// ---------------------------------------------------------------------------
// Singleton input state
// ---------------------------------------------------------------------------

struct InputState {
    bool  key_states[512]  = {};
    float mouse_x          = 0.f;
    float mouse_y          = 0.f;
    float mouse_dx         = 0.f;  // accumulated delta this frame
    float mouse_dy         = 0.f;
    bool  mouse_buttons[3] = {};   // 0=left, 1=right, 2=middle
};

static InputState g_input;

// ---------------------------------------------------------------------------
// Callback — called by the renderer for every sokol_app event
// ---------------------------------------------------------------------------

static void input_event_cb(const void* raw_event, void* /*user_data*/) {
    if (!raw_event) return;
    const sapp_event* ev = static_cast<const sapp_event*>(raw_event);

    switch (ev->type) {
        case SAPP_EVENTTYPE_KEY_DOWN:
            if (ev->key_code < 512)
                g_input.key_states[ev->key_code] = true;
            break;

        case SAPP_EVENTTYPE_KEY_UP:
            if (ev->key_code < 512)
                g_input.key_states[ev->key_code] = false;
            break;

        case SAPP_EVENTTYPE_MOUSE_DOWN:
            if (ev->mouse_button < 3)
                g_input.mouse_buttons[ev->mouse_button] = true;
            break;

        case SAPP_EVENTTYPE_MOUSE_UP:
            if (ev->mouse_button < 3)
                g_input.mouse_buttons[ev->mouse_button] = false;
            break;

        case SAPP_EVENTTYPE_MOUSE_MOVE:
            g_input.mouse_dx += ev->mouse_dx;
            g_input.mouse_dy += ev->mouse_dy;
            g_input.mouse_x   = ev->mouse_x;
            g_input.mouse_y   = ev->mouse_y;
            break;

        default: break;
    }
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void input_init() {
    memset(&g_input, 0, sizeof(g_input));
    renderer_set_input_callback(input_event_cb, nullptr);
}

void input_begin_frame() {
    g_input.mouse_dx = 0.f;
    g_input.mouse_dy = 0.f;
}

// ---------------------------------------------------------------------------
// Public API implementations (declared in engine.h)
// ---------------------------------------------------------------------------

bool engine_key_down(int keycode) {
    if (keycode < 0 || keycode >= 512) return false;
    return g_input.key_states[keycode];
}

glm::vec2 engine_mouse_delta() {
    return {g_input.mouse_dx, g_input.mouse_dy};
}

bool engine_mouse_button(int button) {
    if (button < 0 || button >= 3) return false;
    return g_input.mouse_buttons[button];
}

glm::vec2 engine_mouse_position() {
    return {g_input.mouse_x, g_input.mouse_y};
}
