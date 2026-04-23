#include "renderer.h"
#include "sokol_app.h"
#include <cstdio>

static void on_input(const void* event, void* /*user_data*/) {
    const sapp_event* e = static_cast<const sapp_event*>(event);
    printf("[input] event type: %d\n", static_cast<int>(e->type));
}

int main() {
    RendererConfig cfg;
    cfg.title   = "R-M0 Bootstrap";
    cfg.clear_r = 0.05f;
    cfg.clear_g = 0.05f;
    cfg.clear_b = 0.10f;
    cfg.clear_a = 1.0f;

    renderer_init(cfg);
    renderer_set_input_callback(on_input, nullptr);
    renderer_run();
    return 0;
}
