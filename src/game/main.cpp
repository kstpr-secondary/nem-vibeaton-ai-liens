#include "game.h"
#include <engine.h>
#include <renderer.h>

static void frame_callback(float dt, void* /*user_data*/) {
    renderer_begin_frame();
    game_tick(dt);
    renderer_end_frame();
}

static void input_callback(const void* /*sapp_event*/, void* /*user_data*/) {
    // Input is polled via engine_key_down / engine_mouse_delta in game_tick.
}

int main() {
    RendererConfig renderer_cfg;
    renderer_cfg.title = "Space Shooter";
    renderer_init(renderer_cfg);

    EngineConfig engine_cfg;
    engine_init(engine_cfg);

    game_init();

    renderer_set_frame_callback(frame_callback, nullptr);
    renderer_set_input_callback(input_callback, nullptr);
    renderer_run();

    game_shutdown();
    engine_shutdown();
    renderer_shutdown();
    return 0;
}
