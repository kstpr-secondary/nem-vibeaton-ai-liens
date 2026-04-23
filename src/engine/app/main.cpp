#include <engine.h>
#include <sokol_app.h>          // SAPP_KEYCODE_* constants
#include <glm/gtc/quaternion.hpp>
#include <cstdio>
#include <cmath>
#include <random>

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

    // WASD movements in camera-local space
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

// ---------------------------------------------------------------------------
// Frame callback
// ---------------------------------------------------------------------------

static void setup_scene();  // forward declaration — defined below

static bool g_scene_setup_done = false;

static void frame_cb(float dt, void* /*user_data*/) {
    if (!g_scene_setup_done) {
        setup_scene();
        g_scene_setup_done = true;
    }
    renderer_begin_frame();
    update_camera(dt);
    engine_tick(dt);
    renderer_end_frame();
}

// ---------------------------------------------------------------------------
// E-M4 scene setup: zero-gravity physics demo
// ---------------------------------------------------------------------------

static void setup_scene() {
    // Camera at distance to see the whole scene
    g_cam_entity = engine_create_entity();
    engine_add_component<Transform>(g_cam_entity).position = {0.f, 0.f, 40.f};
    engine_add_component<Camera>(g_cam_entity);
    engine_set_active_camera(g_cam_entity);

    // Directional light
    auto light = engine_create_entity();
    auto& l    = engine_add_component<Light>(light);
    l.direction = {-0.6f, -1.f, -0.4f};
    l.color     = {1.f, 1.f, 1.f};
    l.intensity = 1.5f;

    // Materials
    float rgba_red[4]   = {1.f, 0.25f, 0.25f, 1.f};
    float rgba_gray[4]  = {0.5f, 0.5f, 0.5f, 1.f};

    Material dyn_mat   = renderer_make_unlit_material(rgba_red);
    Material wall_mat  = renderer_make_unlit_material(rgba_gray);

    // Random number generator for initial velocities
    std::mt19937 gen(42);  // Fixed seed for reproducibility
    std::uniform_real_distribution<float> pos_dist(-8.f, 8.f);
    std::uniform_real_distribution<float> vel_dist(-3.f, 3.f);

    // Spawn 20+ dynamic rigid bodies with random initial velocities
    constexpr int kNumBodies = 24;
    for (int i = 0; i < kNumBodies; ++i) {
        auto e = engine_create_entity();

        // Random position in a box
        auto& t = engine_add_component<Transform>(e);
        t.position = {pos_dist(gen), pos_dist(gen), pos_dist(gen) * 0.5f};

        // Mesh: cube with half-extent 0.5
        engine_add_component<Mesh>(e).handle = renderer_make_cube_mesh(0.5f);
        engine_add_component<EntityMaterial>(e).mat = dyn_mat;

        // Collider
        engine_add_component<Collider>(e).half_extents = {0.5f, 0.5f, 0.5f};

        // RigidBody
        auto& rb = engine_add_component<RigidBody>(e);
        rb.mass = 1.0f;
        rb.inv_mass = 1.0f;
        rb.restitution = 1.0f;  // Perfectly elastic
        rb.linear_velocity = {vel_dist(gen), vel_dist(gen), vel_dist(gen)};
        rb.angular_velocity = {vel_dist(gen) * 0.5f, vel_dist(gen) * 0.5f, vel_dist(gen) * 0.5f};

        // Compute body-space inverse inertia for a box
        rb.inv_inertia_body = make_box_inv_inertia_body(rb.mass, {0.5f, 0.5f, 0.5f});
        rb.inv_inertia = rb.inv_inertia_body;  // Initial world-space inertia

        // Tags and physics components
        engine_add_component<Dynamic>(e);
        engine_add_component<ForceAccum>(e);
        engine_add_component<Interactable>(e);
    }

    // Spawn 4 static obstacle walls forming a box arena
    struct WallDef {
        glm::vec3 position;
        glm::vec3 half_extents;
    };

    WallDef walls[] = {
        {{0.f, 12.f, 0.f},  {12.f, 1.f, 12.f}},   // Top
        {{0.f, -12.f, 0.f}, {12.f, 1.f, 12.f}},   // Bottom
        {{-12.f, 0.f, 0.f}, {1.f, 12.f, 12.f}},   // Left
        {{12.f, 0.f, 0.f},  {1.f, 12.f, 12.f}},   // Right
    };

    for (const auto& wall : walls) {
        auto e = engine_create_entity();

        auto& t = engine_add_component<Transform>(e);
        t.position = wall.position;

        // Visual mesh (scaled box)
        engine_add_component<Mesh>(e).handle = renderer_make_cube_mesh(1.0f);
        engine_add_component<EntityMaterial>(e).mat = wall_mat;
        t.scale = wall.half_extents;  // Scale to match wall size

        // Collider
        engine_add_component<Collider>(e).half_extents = wall.half_extents;

        // Static tag (no RigidBody needed)
        engine_add_component<Static>(e);
    }

    fprintf(stderr,
            "[ENGINE] E-M4 scene ready: 1 camera, 1 light, "
            "%d dynamic rigid bodies, 4 static walls\n",
            kNumBodies);
    fprintf(stderr, "[ENGINE] Controls: WASD move, QE up/down, mouse look\n");
    fprintf(stderr, "[ENGINE] Physics: zero-gravity elastic collisions at 120 Hz\n");
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int main() {
    RendererConfig rcfg{};
    rcfg.title  = "engine_app — E-M4: Euler Physics + Elastic Collisions";
    rcfg.width  = 1280;
    rcfg.height = 720;
    renderer_init(rcfg);

    EngineConfig ecfg{};
    engine_init(ecfg);

    renderer_set_frame_callback(frame_cb, nullptr);
    renderer_run();

    engine_shutdown();
    renderer_shutdown();
    return 0;
}
