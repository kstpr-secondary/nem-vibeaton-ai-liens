#include <engine.h>
#include <sokol_app.h>          // SAPP_KEYCODE_* constants
#include <glm/gtc/quaternion.hpp>
#include <cstdio>
#include <cmath>

// ---------------------------------------------------------------------------
// FPS camera controller state
// ---------------------------------------------------------------------------

static entt::entity g_cam_entity = entt::null;
static float        g_yaw        = 0.f;   // radians, around world Y
static float        g_pitch      = 0.f;   // radians, around local X

static void update_camera(float dt) {
    if (!engine_registry().valid(g_cam_entity)) return;

    // Mouse look
    glm::vec2 mdelta      = engine_mouse_delta();
    constexpr float kSens = 0.002f;
    g_yaw   -= mdelta.x * kSens;
    g_pitch -= mdelta.y * kSens;
    g_pitch  = glm::clamp(g_pitch, -1.4f, 1.4f);

    glm::quat q_yaw   = glm::angleAxis(g_yaw,   glm::vec3(0.f, 1.f, 0.f));
    glm::quat q_pitch = glm::angleAxis(g_pitch,  glm::vec3(1.f, 0.f, 0.f));
    glm::quat rot     = glm::normalize(q_yaw * q_pitch);

    auto& t = engine_get_component<Transform>(g_cam_entity);
    t.rotation = rot;

    // WASD movement in camera-local space
    glm::vec3 forward = rot * glm::vec3( 0.f, 0.f, -1.f);
    glm::vec3 right   = rot * glm::vec3( 1.f, 0.f,  0.f);
    constexpr float kSpeed = 10.f;
    float move = kSpeed * dt;

    if (engine_key_down(SAPP_KEYCODE_W)) t.position += forward * move;
    if (engine_key_down(SAPP_KEYCODE_S)) t.position -= forward * move;
    if (engine_key_down(SAPP_KEYCODE_A)) t.position -= right   * move;
    if (engine_key_down(SAPP_KEYCODE_D)) t.position += right   * move;
    if (engine_key_down(SAPP_KEYCODE_Q)) t.position.y -= move;
    if (engine_key_down(SAPP_KEYCODE_E)) t.position.y += move;
}

static void do_raycast() {
    if (!engine_registry().valid(g_cam_entity)) return;
    auto& t = engine_get_component<Transform>(g_cam_entity);

    glm::vec3 forward = t.rotation * glm::vec3(0.f, 0.f, -1.f);
    auto hit = engine_raycast(t.position, forward, 200.f);
    if (hit) {
        fprintf(stderr, "[ENGINE] Raycast hit entity %u  dist=%.2f  normal=(%.1f,%.1f,%.1f)\n",
                static_cast<uint32_t>(hit->entity),
                static_cast<double>(hit->distance),
                static_cast<double>(hit->normal.x),
                static_cast<double>(hit->normal.y),
                static_cast<double>(hit->normal.z));
    }
}

// ---------------------------------------------------------------------------
// Frame callback
// ---------------------------------------------------------------------------

static void frame_cb(float dt, void* /*user_data*/) {
    renderer_begin_frame();
    update_camera(dt);
    engine_tick(dt);
    do_raycast();
    renderer_end_frame();
}

// ---------------------------------------------------------------------------
// E-M3 scene setup
// ---------------------------------------------------------------------------

static void setup_scene() {
    // Camera at origin, looking down -Z
    g_cam_entity = engine_create_entity();
    engine_add_component<Transform>(g_cam_entity).position = {0.f, 0.f, 30.f};
    engine_add_component<Camera>(g_cam_entity);
    engine_set_active_camera(g_cam_entity);

    // Directional light
    auto light = engine_create_entity();
    auto& l    = engine_add_component<Light>(light);
    l.direction = {-0.6f, -1.f, -0.4f};
    l.color     = {1.f, 1.f, 1.f};
    l.intensity = 1.f;

    // Materials
    float rgba_red[4]  = {1.f, 0.25f, 0.25f, 1.f};
    float rgba_cyan[4] = {0.2f, 0.8f, 1.f, 1.f};
    float rgba_gold[4] = {1.f, 0.8f, 0.3f, 1.f};

    Material sphere_mat = renderer_make_unlit_material(rgba_red);
    Material cube_mat   = renderer_make_unlit_material(rgba_cyan);
    Material asset_mat  = renderer_make_unlit_material(rgba_gold);

    // 50 collidable spheres arranged in a 5×10 grid — validates SC-005 raycast scale
    for (int row = 0; row < 5; ++row) {
        for (int col = 0; col < 10; ++col) {
            float x = (col - 4.5f) * 4.f;
            float y = (row - 2.f) * 4.f;
            float z = -20.f;

            auto e = engine_create_entity();
            auto& t = engine_add_component<Transform>(e);
            t.position = {x, y, z};
            engine_add_component<Mesh>(e).handle       = renderer_make_sphere_mesh(0.8f, 12);
            engine_add_component<EntityMaterial>(e).mat = sphere_mat;
            engine_add_component<Collider>(e).half_extents = {0.8f, 0.8f, 0.8f};
            engine_add_component<Interactable>(e);       // visible to raycast
        }
    }

    // 4 procedural cubes (non-collidable) along the floor for reference
    for (int i = 0; i < 4; ++i) {
        float x = (i - 1.5f) * 8.f;
        engine_spawn_cube({x, -10.f, -20.f}, 1.f, cube_mat);
    }

    // glTF asset (E-M2 carry-over)
    auto asset_e = engine_spawn_from_asset(
        "Asteroid_1a.glb",
        {10.f, 0.f, -15.f},
        glm::quat(1.f, 0.f, 0.f, 0.f),
        asset_mat);
    if (asset_e != entt::null)
        fprintf(stderr, "[ENGINE] E-M3: Asteroid_1a.glb spawned\n");

    fprintf(stderr,
            "[ENGINE] E-M3 scene ready: 1 camera, 1 light, "
            "50 collidable spheres, 4 cubes, 1 glTF asset\n");
    fprintf(stderr, "[ENGINE] Controls: WASD move, QE up/down, mouse look\n");
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int main() {
    RendererConfig rcfg{};
    rcfg.title  = "engine_app — E-M3: Input + Colliders + Raycast";
    rcfg.width  = 1280;
    rcfg.height = 720;
    renderer_init(rcfg);

    EngineConfig ecfg{};
    engine_init(ecfg);

    setup_scene();

    renderer_set_frame_callback(frame_cb, nullptr);
    renderer_run();

    engine_shutdown();
    renderer_shutdown();
    return 0;
}
