#include <engine.h>
#include <paths.h>
#include <sokol_app.h>          // SAPP_KEYCODE_* constants
#include <glm/gtc/quaternion.hpp>
#include <cstdio>
#include <cmath>
#include <random>
#include <filesystem>
#include <string>
#include <vector>

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

static void setup_scene();        // forward declaration — defined below
static void update_highlight(float dt);

static bool g_scene_setup_done = false;

static void frame_cb(float dt, void* /*user_data*/) {
    if (!g_scene_setup_done) {
        setup_scene();
        g_scene_setup_done = true;
    }
    renderer_begin_frame();
    update_camera(dt);
    engine_tick(dt);
    update_highlight(dt);
    renderer_end_frame();
}

// ---------------------------------------------------------------------------
// E-M3 / E-M4 scene setup: mixed cubes + asteroids with Lambertian materials
// ---------------------------------------------------------------------------

static bool g_highlight_active = false;
static RaycastHit g_last_hit   = {};

static void setup_scene() {
    // Camera — FPS-style controller position
    g_cam_entity = engine_create_entity();
    engine_add_component<Transform>(g_cam_entity).position = {0.f, 2.f, 35.f};
    engine_add_component<Camera>(g_cam_entity);
    engine_set_active_camera(g_cam_entity);

    // Directional light for Lambertian shading
    auto light = engine_create_entity();
    auto& l    = engine_add_component<Light>(light);
    l.direction = {-0.6f, -1.f, -0.4f};
    l.color     = {1.f, 1.f, 1.f};
    l.intensity = 2.0f;

    // Lambertian materials — earthy / rocky tones for cubes and asteroids
    float rgb_cube[3]   = {0.55f, 0.40f, 0.30f};
    float rgb_asteroid[3] = {0.35f, 0.30f, 0.28f};

    Material cube_mat     = renderer_make_lambertian_material(rgb_cube);
    Material asteroid_mat = renderer_make_lambertian_material(rgb_asteroid);

    // Unlit transparent material for front/back walls (alpha < 1.0 triggers
    // the transparent pipeline with alpha blending and depth-write OFF)
    float rgba_wall[4]    = {0.55f, 0.40f, 0.30f, 0.30f};
    Material wall_mat     = renderer_make_unlit_material(rgba_wall);

    // Random number generator — fixed seed for reproducibility
    std::mt19937 gen(42);
    std::uniform_real_distribution<float> pos_dist(-8.f, 8.f);
    std::uniform_real_distribution<float> vel_dist(-9.f, 9.f);

    // --- Dynamic cubes: 40 entities ---
    constexpr int kNumCubes = 40;
    for (int i = 0; i < kNumCubes; ++i) {
        auto e = engine_create_entity();

        auto& t = engine_add_component<Transform>(e);
        t.position = {pos_dist(gen), pos_dist(gen), pos_dist(gen) * 0.5f};

        float half = 0.3f + 0.2f * (static_cast<float>(i % 5)) / 5.0f;
        engine_add_component<Mesh>(e).handle = renderer_make_cube_mesh(half);
        engine_add_component<EntityMaterial>(e).mat = cube_mat;

        engine_add_component<Collider>(e).half_extents = {half, half, half};

        auto& rb = engine_add_component<RigidBody>(e);
        rb.mass = 1.0f;
        rb.inv_mass = 1.0f;
        rb.restitution = 1.0f;
        rb.linear_velocity = {vel_dist(gen), vel_dist(gen), vel_dist(gen)};
        rb.angular_velocity = {vel_dist(gen) * 0.5f, vel_dist(gen) * 0.5f, vel_dist(gen) * 0.5f};
        rb.inv_inertia_body = make_box_inv_inertia_body(rb.mass, {half, half, half});
        rb.inv_inertia = rb.inv_inertia_body;

        engine_add_component<Dynamic>(e);
        engine_add_component<ForceAccum>(e);
        engine_add_component<Interactable>(e);
    }

    // --- Dynamic asteroids: 20 entities (glTF) ---
    constexpr int kNumAsteroids = 20;
    const char* k_asset_path = "Asteroid_1a.glb";
    auto asteroid_handle = engine_load_gltf(k_asset_path);

    if (!renderer_handle_valid(asteroid_handle)) {
        fprintf(stderr, "[ENGINE] glTF: failed to load %s/%s (%d will be cubes instead)\n",
                ASSET_ROOT, k_asset_path, kNumAsteroids);
    }

    int asteroid_count = 0;
    for (int i = 0; i < kNumAsteroids; ++i) {
        auto e = engine_create_entity();
        auto& t = engine_add_component<Transform>(e);
        t.position = {pos_dist(gen), pos_dist(gen), pos_dist(gen) * 0.5f};

        if (renderer_handle_valid(asteroid_handle)) {
            float scale = 1.5f + 0.8f * (static_cast<float>(i % 4)) / 4.0f;
            t.scale = {scale, scale, scale};
            engine_add_component<Mesh>(e).handle = asteroid_handle;
            engine_add_component<EntityMaterial>(e).mat = asteroid_mat;

            // Approximate AABB for scaled asteroid
            engine_add_component<Collider>(e).half_extents = {2.0f * scale, 1.5f * scale, 1.5f * scale};

            auto& rb = engine_add_component<RigidBody>(e);
            rb.mass = 3.0f;
            rb.inv_mass = 1.0f / 3.0f;
            rb.restitution = 1.0f;
            rb.linear_velocity = {vel_dist(gen), vel_dist(gen), vel_dist(gen)};
            rb.angular_velocity = {vel_dist(gen) * 0.3f, vel_dist(gen) * 0.3f, vel_dist(gen) * 0.3f};
            rb.inv_inertia_body = make_box_inv_inertia_body(rb.mass, {2.0f * scale, 1.5f * scale, 1.5f * scale});
            rb.inv_inertia = rb.inv_inertia_body;

            asteroid_count++;
        } else {
            // Fallback: spawn a cube instead
            float half = 0.6f + 0.2f * (static_cast<float>(i % 3)) / 3.0f;
            t.scale = {half, half, half};
            engine_add_component<Mesh>(e).handle = renderer_make_cube_mesh(1.0f);
            engine_add_component<EntityMaterial>(e).mat = asteroid_mat;
            engine_add_component<Collider>(e).half_extents = {half, half, half};

            auto& rb = engine_add_component<RigidBody>(e);
            rb.mass = 2.0f;
            rb.inv_mass = 0.5f;
            rb.restitution = 1.0f;
            rb.linear_velocity = {vel_dist(gen), vel_dist(gen), vel_dist(gen)};
            rb.angular_velocity = {vel_dist(gen) * 0.3f, vel_dist(gen) * 0.3f, vel_dist(gen) * 0.3f};
            rb.inv_inertia_body = make_box_inv_inertia_body(rb.mass, {half, half, half});
            rb.inv_inertia = rb.inv_inertia_body;
        }

        engine_add_component<Dynamic>(e);
        engine_add_component<ForceAccum>(e);
        engine_add_component<Interactable>(e);
    }

    // --- Static arena walls: 6 (box ~70 units across) ---
    struct WallDef {
        glm::vec3 position;
        glm::vec3 half_extents;
    };

    WallDef walls[] = {
        {{0.f, 35.f, 0.f},   {35.f, 1.f, 35.f}},   // Top
        {{0.f, -35.f, 0.f},  {35.f, 1.f, 35.f}},   // Bottom
        {{-35.f, 0.f, 0.f},  {1.f, 35.f, 35.f}},   // Left
        {{35.f, 0.f, 0.f},   {1.f, 35.f, 35.f}},   // Right
        {{0.f, 0.f, 35.f},   {35.f, 35.f, 1.f}},   // Front (transparent)
        {{0.f, 0.f, -35.f},  {35.f, 35.f, 1.f}},   // Back (transparent)
    };

    for (int i = 0; i < 6; ++i) {
        auto e = engine_create_entity();
        auto& t = engine_add_component<Transform>(e);
        t.position = walls[i].position;

        engine_add_component<Mesh>(e).handle = renderer_make_cube_mesh(1.0f);
        engine_add_component<EntityMaterial>(e).mat = (i < 4) ? asteroid_mat : wall_mat;
        t.scale = walls[i].half_extents;

        engine_add_component<Collider>(e).half_extents = walls[i].half_extents;
        engine_add_component<Interactable>(e);
        engine_add_component<Static>(e);
    }

    // --- Central feature: one large asteroid at origin ---
    if (renderer_handle_valid(asteroid_handle)) {
        auto central = engine_create_entity();
        Transform ct{};
        ct.position   = {0.f, 0.f, 0.f};
        ct.scale      = {3.5f, 3.5f, 3.5f};
        engine_add_component<Transform>(central, ct);
        auto& c_mesh   = engine_add_component<Mesh>(central);
        c_mesh.handle  = asteroid_handle;
        engine_add_component<EntityMaterial>(central).mat = asteroid_mat;
        engine_add_component<Collider>(central).half_extents = {7.f, 5.5f, 5.5f};
        engine_add_component<Interactable>(central);
        engine_add_component<Static>(central);
    }

    int total = kNumCubes + asteroid_count + 6;
    if (renderer_handle_valid(asteroid_handle))
        total += 1;  // central asteroid
    fprintf(stderr,
            "[ENGINE] E-M3 scene ready: 1 camera, 1 light, "
            "%d dynamic entities (%d cubes, %d asteroids), static walls + central feature\n",
            total, kNumCubes, asteroid_count);
    fprintf(stderr, "[ENGINE] Controls: WASD move, QE up/down, mouse look\n");
    fprintf(stderr, "[ENGINE] Crosshair raycasts nearest Interactable entity\n");
}

// ---------------------------------------------------------------------------
// Raycast highlight — called every frame after engine_tick()
// ---------------------------------------------------------------------------

static void update_highlight(float dt) {
    (void)dt;

    // Cast ray from camera center along forward direction
    if (!engine_registry().valid(g_cam_entity)) return;

    auto& cam_t = engine_get_component<Transform>(g_cam_entity);
    auto& cam_c = engine_get_component<Camera>(g_cam_entity);
    (void)cam_c;

    glm::quat rot = cam_t.rotation;
    glm::vec3 forward = rot * glm::vec3(0.f, 0.f, -1.f);
    forward = glm::normalize(forward);

    auto hit_opt = engine_raycast(cam_t.position, forward, 200.f);

    if (hit_opt.has_value()) {
        g_highlight_active = true;
        g_last_hit         = hit_opt.value();

        // Print hit info every second (approximately)
        static float s_last_print = -1.f;
        float t = static_cast<float>(engine_now());
        if (t - s_last_print >= 1.0f) {
            s_last_print = t;
            fprintf(stderr,
                    "[ENGINE] Raycast hit: entity=%u, dist=%.2f, point=(%.2f, %.2f, %.2f), normal=(%.2f, %.2f, %.2f)\n",
                    static_cast<unsigned>(g_last_hit.entity),
                    g_last_hit.distance,
                    g_last_hit.point.x, g_last_hit.point.y, g_last_hit.point.z,
                    g_last_hit.normal.x, g_last_hit.normal.y, g_last_hit.normal.z);
        }
    } else {
        g_highlight_active = false;
    }
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int main() {
    RendererConfig rcfg{};
    rcfg.title  = "engine_app — E-M4: Rigid Bodies in Transparent Arena";
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
