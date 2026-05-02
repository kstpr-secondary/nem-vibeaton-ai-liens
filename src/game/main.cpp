#include "game.h"
#include <engine.h>
#include <renderer.h>
#include <paths.h>
#include <sokol_app.h>
#include <cstdio>

// stb_image implementation is already provided by librenderer.a (texture.cpp).
// Do NOT define STB_IMAGE_IMPLEMENTATION here — it causes duplicate symbols.
#include "stb_image.h"

static bool s_game_inited = false;
static bool s_culling_on  = true;

static void frame_callback(float dt, void* /*user_data*/) {
    // Toggle frustum culling with C key (checked via engine's input state).
    if (engine_key_down(SAPP_KEYCODE_C)) {
        s_culling_on = !s_culling_on;
        renderer_set_culling_enabled(s_culling_on);
        printf("[game] frustum culling: %s\n", s_culling_on ? "ON" : "OFF");
    }
    renderer_begin_frame();

    // game_init must run after sg_setup() (called by sapp_run's init_cb),
    // so defer it to the first frame.
    if (!s_game_inited) {
        game_init();

        // Load skybox cubemap from ASSET_ROOT/skybox/ — same 6-face order as
        // renderer_app: +X, -X, +Y, -Y, +Z, -Z.
        const char* skybox_faces[6] = {
            "px.png", "nx.png",
            "py.png", "ny.png",
            "pz.png", "nz.png"
        };
        const void* raw_faces[6] = {};
        int w = 0, h = 0, ch = 0;
        for (int i = 0; i < 6; ++i) {
            char path[512];
            snprintf(path, sizeof(path), "%s/skybox/%s", ASSET_ROOT, skybox_faces[i]);
            unsigned char* data = stbi_load(path, &w, &h, &ch, 4);
            if (data) {
                raw_faces[i] = data;
            }
        }
        RendererTextureHandle skybox_tex = renderer_upload_cubemap(raw_faces, w, h, 4);
        for (int i = 0; i < 6; ++i) {
            if (raw_faces[i])
                stbi_image_free(const_cast<unsigned char*>(static_cast<const unsigned char*>(raw_faces[i])));
        }
        if (renderer_handle_valid(skybox_tex))
            renderer_set_skybox(skybox_tex);

        s_game_inited = true;
    }

    game_tick(dt);
    renderer_end_frame();
}

int main() {
    RendererConfig renderer_cfg;
    renderer_cfg.title = "Space Shooter";
    renderer_init(renderer_cfg);

    EngineConfig engine_cfg;
    engine_init(engine_cfg);

    renderer_set_frame_callback(frame_callback, nullptr);
    // engine_init() already registered the engine's input callback via input_init().
    // Do NOT overwrite it here — the engine owns input event handling.

    renderer_run();

    game_shutdown();
    engine_shutdown();
    renderer_shutdown();
    return 0;
}
