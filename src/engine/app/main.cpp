#include <engine.h>
#include <cstdio>

// ---------------------------------------------------------------------------
// Frame callback — called by the renderer once per frame
// ---------------------------------------------------------------------------

static void frame_cb(float dt, void* /*user_data*/) {
    renderer_begin_frame();
    engine_tick(dt);
    renderer_end_frame();
}

// ---------------------------------------------------------------------------
// E-M2 scene setup: camera + light + procedural primitives + glTF asset
// ---------------------------------------------------------------------------

static void setup_scene() {
    // Camera: pulled back to see the full scene
    auto cam = engine_create_entity();
    engine_add_component<Transform>(cam).position = {0.f, 2.f, 18.f};
    engine_add_component<Camera>(cam);
    engine_set_active_camera(cam);

    // Directional light: angled from upper-left
    auto light = engine_create_entity();
    auto& l = engine_add_component<Light>(light);
    l.direction = {-0.6f, -1.f, -0.4f};
    l.color     = {1.f,  1.f,  1.f};
    l.intensity = 1.f;

    // 6 spheres spread along X at y=2
    float rgba_red[4] = {1.f, 0.25f, 0.25f, 1.f};
    Material sphere_mat = renderer_make_unlit_material(rgba_red);
    for (int i = 0; i < 6; ++i) {
        float x = (static_cast<float>(i) - 2.5f) * 2.2f;
        engine_spawn_sphere({x, 2.f, 0.f}, 0.6f, sphere_mat);
    }

    // 4 cubes spread along X at y=-1
    float rgba_cyan[4] = {0.2f, 0.8f, 1.f, 1.f};
    Material cube_mat = renderer_make_unlit_material(rgba_cyan);
    for (int i = 0; i < 4; ++i) {
        float x = (static_cast<float>(i) - 1.5f) * 3.f;
        engine_spawn_cube({x, -1.f, 0.f}, 0.6f, cube_mat);
    }

    // E-M2: load a glTF asset and spawn it alongside the procedurals
    float rgba_gold[4] = {1.f, 0.8f, 0.3f, 1.f};
    Material asset_mat = renderer_make_unlit_material(rgba_gold);

    auto asset_e = engine_spawn_from_asset(
        "Asteroid_1a.glb",
        /*position=*/{0.f, -4.f, 0.f},
        /*rotation=*/glm::quat(1.f, 0.f, 0.f, 0.f),
        asset_mat
    );

    if (asset_e == entt::null) {
        fprintf(stderr, "[ENGINE] E-M2: asset load failed — running E-M1 scene only\n");
    } else {
        fprintf(stderr, "[ENGINE] E-M2: Asteroid_1a.glb spawned\n");
    }

    fprintf(stderr, "[ENGINE] E-M2 scene ready: 1 camera, 1 light, 6 spheres, 4 cubes, 1 glTF asset\n");
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int main() {
    RendererConfig rcfg{};
    rcfg.title  = "engine_app — E-M2: ECS Scene + glTF Asset";
    rcfg.width  = 1280;
    rcfg.height = 720;
    renderer_init(rcfg);

    EngineConfig ecfg{};
    engine_init(ecfg);

    setup_scene();

    renderer_set_frame_callback(frame_cb, nullptr);
    renderer_run();  // blocks until window is closed

    engine_shutdown();
    renderer_shutdown();
    return 0;
}
