#include <engine.h>
#include <paths.h>
#include <mesh_builders.h>
#include <sokol_app.h>          // SAPP_KEYCODE_* constants
#include <imgui.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdio>
#include <cmath>
#include <cstring>
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
static float rgb_cyan[3]    = {0.2f, 0.7f, 0.9f};
static float rgb_blue[3]    = {0.15f, 0.6f, 0.85f};
static float rgb_gray[3]    = {0.55f, 0.55f, 0.55f};
constexpr int kNumCubes    = 40;

static void frame_cb(float dt, void* /*user_data*/) {
    if (!g_scene_setup_done) {
        setup_scene();
        g_scene_setup_done = true;
    }

   renderer_begin_frame();
     update_camera(dt);
     engine_tick(dt);

  // --- Debug: detect entities outside arena bounds ---
     {
         constexpr float kHalfX = 75.f, kHalfY = 30.f, kHalfZ = 60.f;
         auto& reg = engine_registry();

         std::vector<entt::entity> to_add, to_remove;

         // Pass 1: collect OOB IDs without modifying registry
         reg.view<Transform, Collider, Dynamic>().each([&](auto e, const Transform& tr, const Collider& col) {
             float hx = col.half_extents.x * tr.scale.x;
             float hy = col.half_extents.y * tr.scale.y;
             float hz = col.half_extents.z * tr.scale.z;

             bool out = (tr.position.x + hx > kHalfX) || (tr.position.x - hx < -kHalfX) ||
                        (tr.position.y + hy > kHalfY) || (tr.position.y - hy < -kHalfY) ||
                        (tr.position.z + hz > kHalfZ) || (tr.position.z - hz < -kHalfZ);

             if (out) {
                 to_add.push_back(e);
             } else if (reg.all_of<OutOfBounds>(e)) {
                 to_remove.push_back(e);
             }
         });

         // Pass 2: apply structural changes after iteration
         for (auto e : to_add)
             reg.emplace_or_replace<OutOfBounds>(e);
         for (auto e : to_remove)
             reg.remove<OutOfBounds>(e);

         if (!to_add.empty()) {
             static int log_counter = 0;
             ++log_counter;
             if (log_counter % 30 == 0)
                 fprintf(stderr, "[ENGINE] DEBUG: %d entity(ies) outside arena bounds\n", (int)to_add.size());
         }
     }

     // Submit draw calls for all entities with Transform + Mesh + EntityMaterial.
    // CollisionFlash timers decay; entity is drawn with blended color if timer > 0.
    {
        auto& reg = engine_registry();
        auto view = reg.view<Transform, Mesh, EntityMaterial>();
        for (auto e : view) {
            const auto& t   = view.get<Transform>(e);
            const auto& m   = view.get<Mesh>(e);
            const auto& em  = view.get<EntityMaterial>(e);

            if (!renderer_handle_valid(m.handle))
                continue;

            // Decay CollisionFlash timer (optional component — not in view)
            auto* cf = engine_try_get_component<CollisionFlash>(e);
            if (cf) {
                cf->timer -= dt;
                if (cf->timer <= 0.f) {
                    reg.remove<CollisionFlash>(e);
                    cf = nullptr;
                }
            }

           Material mat = em.mat;
             if (cf && cf->timer > 0.f) {
                 float t_norm = std::min(cf->timer, 1.0f);
                 // Timer 1.0→0.5: flash→mid-blend; 0.5→0.0: mid-blend→base
                 float blend = (t_norm > 0.5f) ? (t_norm - 0.5f) * 2.0f : t_norm * 2.0f;
                 blend = std::clamp(blend, 0.0f, 1.0f);
                 // Write blended RGB into the first 12 bytes of the uniform blob
                 // (works for both UnlitFSParams::color and BlinnPhongFSParams::base_color)
                 float blended[3] = {
                     cf->flash_color[0] + blend * (cf->base_color[0] - cf->flash_color[0]),
                     cf->flash_color[1] + blend * (cf->base_color[1] - cf->flash_color[1]),
                     cf->flash_color[2] + blend * (cf->base_color[2] - cf->flash_color[2]),
                 };
                 std::memcpy(mat.uniforms, blended, 3 * sizeof(float));
             } else if (reg.all_of<OutOfBounds>(e)) {
                 // OOB override — bright green (collision red already takes priority above)
                 float green[] = { 0.f, 1.f, 0.f };
                 std::memcpy(mat.uniforms, green, 3 * sizeof(float));
             }

            const glm::mat4 world =
                glm::translate(glm::mat4(1.f), t.position)
                * glm::mat4_cast(t.rotation)
                * glm::scale(glm::mat4(1.f), t.scale);

            renderer_enqueue_draw(m.handle, glm::value_ptr(world), mat);
        }
    }

    update_highlight(dt);

    // ---- ImGui HUD: FPS + draw count + triangle count + entities ----
    // Must be called before renderer_end_frame() (which calls simgui_render())
    {
        float fps = ImGui::GetIO().Framerate;
        int   draw_count = renderer_get_draw_count();
        int   tri_count  = renderer_get_triangle_count();
        int   entity_count = 0;
        engine_registry().view<Dynamic>().each([&](auto) { ++entity_count; });

        ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Once);
        ImGui::Begin("HUD", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs);
        {
            char buf[256];
            snprintf(buf, sizeof(buf), "FPS: %.1f\nDraws: %d\nTriangles: %d\nEntities: %d",
                     fps, draw_count, tri_count, entity_count);
            ImGui::TextUnformatted(buf);
        }
        ImGui::End();
    }

   renderer_end_frame();

    // ---- Stress-test spawner ----
    {
        static float spawn_timer = 0.f;
        static float spawn_interval = 2.0f;
        constexpr float spawn_interval_min = 0.3f;
        constexpr int max_entities = 200;

        static std::mt19937 spawner_gen(12345u);
       std::uniform_real_distribution<float> spawner_pos(-25.f, 25.f);
        std::uniform_int_distribution<int> spawner_tier(0, 2);

        int dyn_count = 0;
        engine_registry().view<SpawnTarget>().each([&](auto) { ++dyn_count; });

        if (dyn_count < max_entities) {
            spawn_timer += dt;
            if (spawn_timer >= spawn_interval) {
                spawn_timer -= spawn_interval;

                glm::vec3 pos = {spawner_pos(spawner_gen), spawner_pos(spawner_gen), spawner_pos(spawner_gen) * 0.5f};
                int tier = spawner_tier(spawner_gen);
                float mass = (tier == 0) ? 1.0f : (tier == 1) ? 3.0f : 5.0f;

                constexpr int k_num_models = 4;
                const char* model_names[k_num_models] = {
                    "Asteroid_1a.glb", "Asteroid_1e.glb", "Asteroid_2a.glb", "Asteroid_2b.glb"
                };

                auto h = engine_load_gltf(model_names[spawner_gen() % k_num_models]);
                bool is_asset = renderer_handle_valid(h);

                entt::entity e;
                if (is_asset) {
                    float scale = (tier == 0) ? 1.2f : (tier == 1) ? 2.0f : 3.0f;
                    e = engine_create_entity();
                    auto& t = engine_add_component<Transform>(e);
                    t.position = pos;
                    t.scale = {scale, scale, scale};
                    auto& mesh_comp = engine_add_component<Mesh>(e);
                    mesh_comp.handle = h;
                    mesh_store_ref_inc(h.id);

                    engine_add_component<EntityMaterial>(e).mat = renderer_make_lambertian_material(rgb_gray);

                    engine_add_component<Collider>(e).half_extents = {scale, scale * 0.8f, scale};
                } else {
                    float half = (tier == 0) ? 0.3f : (tier == 1) ? 0.5f : 0.7f;
                    e = engine_spawn_cube(pos, half, renderer_make_lambertian_material(rgb_gray));
                }

                auto& rb = engine_add_component<RigidBody>(e);
                rb.mass = mass;
                rb.inv_mass = 1.0f / mass;
                rb.restitution = 1.0f;
                float vel_min = (tier == 0) ? -5.f : (tier == 1) ? -8.f : -12.f;
                float vel_max = (tier == 0) ? 5.f : (tier == 1) ? 8.f : 12.f;
                std::uniform_real_distribution<float> vel(vel_min, vel_max);
                rb.linear_velocity = {vel(spawner_gen), vel(spawner_gen), vel(spawner_gen)};
                float ang_min = (tier == 0) ? -1.5f : (tier == 1) ? -2.5f : -4.f;
                float ang_max = (tier == 0) ? 1.5f : (tier == 1) ? 2.5f : 4.f;
                std::uniform_real_distribution<float> ang_vel(ang_min, ang_max);
                rb.angular_velocity = {ang_vel(spawner_gen), ang_vel(spawner_gen), ang_vel(spawner_gen)};

                if (is_asset) {
                    float scale = (tier == 0) ? 1.2f : (tier == 1) ? 2.0f : 3.0f;
                    rb.inv_inertia_body = make_box_inv_inertia_body(mass, {scale, scale * 0.8f, scale});
                } else {
                    float half = (tier == 0) ? 0.3f : (tier == 1) ? 0.5f : 0.7f;
                    rb.inv_inertia_body = make_box_inv_inertia_body(mass, {half, half, half});
                }
                rb.inv_inertia = rb.inv_inertia_body;

                engine_add_component<Dynamic>(e);
                engine_add_component<ForceAccum>(e);
                engine_add_component<SpawnTarget>(e);
                engine_add_component<Interactable>(e);

                spawn_interval = std::max(spawn_interval_min, spawn_interval - 0.15f);
            }
        }

        // Destroy oldest SpawnTarget entities if count exceeds max
        if (dyn_count > max_entities) {
            constexpr int target_count = 150;
            int to_destroy = dyn_count - target_count;
            if (to_destroy > 0 && to_destroy < dyn_count) {
                int destroyed = 0;
                engine_registry().view<entt::entity, SpawnTarget>().each([&](auto e) {
                    engine_destroy_entity(e);
                    if (++destroyed >= to_destroy) return;
                });
            }
        }
    }
}

// ---------------------------------------------------------------------------
// E-M3 / E-M4 scene setup: mixed cubes + asteroids with Lambertian materials
// ---------------------------------------------------------------------------

static bool g_highlight_active = false;
static RaycastHit g_last_hit   = {};

static void setup_scene() {
    // Camera — FPS-style controller position (back from arena for overview)
    g_cam_entity = engine_create_entity();
    engine_add_component<Transform>(g_cam_entity).position = {0.f, 8.f, 180.f};
    engine_add_component<Camera>(g_cam_entity);
    engine_set_active_camera(g_cam_entity);

    // Directional light for Lambertian shading
    auto light = engine_create_entity();
    auto& l    = engine_add_component<Light>(light);
    l.direction = {0.6f, 1.f, 0.4f};   // direction FROM surface TO light (world space)
    l.color     = {1.f, 1.f, 1.f};
    l.intensity = 2.0f;

    // Dynamic objects — standard gray
    float rgb_dynamic[3]  = {0.55f, 0.55f, 0.55f};

    Material dynamic_mat  = renderer_make_lambertian_material(rgb_dynamic);

    // Unlit transparent material for front/back walls (reserved for future use)

    // Random number generator — fixed seed for reproducibility
    std::mt19937 gen(42);
    std::uniform_real_distribution<float> pos_dist(-25.f, 25.f);
    std::uniform_real_distribution<float> vel_dist(-12.f, 12.f);

    // --- Dynamic cubes: 40 entities ---
    for (int i = 0; i < kNumCubes; ++i) {
        auto e = engine_create_entity();

        auto& t = engine_add_component<Transform>(e);
        t.position = {pos_dist(gen), pos_dist(gen), pos_dist(gen) * 0.5f};

        float half = 0.3f + 0.2f * (static_cast<float>(i % 5)) / 5.0f;
        engine_add_component<Mesh>(e).handle = renderer_make_cube_mesh(half);
        engine_add_component<EntityMaterial>(e).mat = dynamic_mat;

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
            engine_add_component<EntityMaterial>(e).mat = dynamic_mat;

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
            engine_add_component<EntityMaterial>(e).mat = dynamic_mat;
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

    // --- Static asteroids: 10 spatially separated in a ring pattern ---
    constexpr int kNumStaticAsteroids = 10;
    const char* k_asteroid_models[4] = {
        "Asteroid_1a.glb",
        "Asteroid_1e.glb",
        "Asteroid_2a.glb",
        "Asteroid_2b.glb",
    };
    RendererMeshHandle static_mesh_handles[4] = {};
    int static_mesh_count = 0;

    for (int m = 0; m < 4; ++m) {
        auto h = engine_load_gltf(k_asteroid_models[m]);
        if (renderer_handle_valid(h)) {
            static_mesh_handles[static_mesh_count++] = h;
        } else {
            static_mesh_handles[static_mesh_count++] = renderer_make_cube_mesh(1.0f);
        }
    }

    std::uniform_int_distribution<int> model_dist(0, static_mesh_count - 1);
    std::uniform_int_distribution<int> tier_dist(0, 2);  // 0=small, 1=medium, 2=large

    const float k_scales[] = {1.0f, 1.5f};       // small
    const float k_scales_m[] = {2.0f, 2.5f};     // medium
    const float k_scales_l[] = {3.0f, 4.0f};     // large
    const float* all_scales[] = {k_scales, k_scales_m, k_scales_l};

    // Place static asteroids widely spaced across the larger arena
    // Outer ring (radius 45), inner ring (radius 20), plus scattered mid-zone
    int static_count = 0;
    for (int i = 0; i < kNumStaticAsteroids; ++i) {
        auto e = engine_create_entity();
        auto& t = engine_add_component<Transform>(e);

        float radius, angle, y_val;
        if (i < 6) {
            // Outer ring — 6 asteroids widely spaced
            radius = 45.f + std::uniform_real_distribution<float>(-3.f, 3.f)(gen);
            angle = (2.f * 3.14159265f / 6.f) * static_cast<float>(i);
            y_val = -8.f + 16.f * (static_cast<float>(i % 3)) / 2.f;
        } else if (i < 9) {
            // Inner ring — 3 asteroids
            radius = 20.f + std::uniform_real_distribution<float>(-2.f, 2.f)(gen);
            angle = (2.f * 3.14159265f / 3.f) * static_cast<float>(i - 6) + 0.5f;
            y_val = -12.f + 12.f * (static_cast<float>((i - 6) % 2)) / 1.f;
        } else {
            // Mid-zone scattered — 1 asteroid
            radius = 30.f + std::uniform_real_distribution<float>(-5.f, 5.f)(gen);
            angle = std::uniform_real_distribution<float>(0.f, 6.2831853f)(gen);
            y_val = std::uniform_real_distribution<float>(-15.f, 15.f)(gen);
        }

        t.position = {radius * cosf(angle), y_val, radius * sinf(angle)};

        int model_idx = model_dist(gen);
        int tier = tier_dist(gen);
        float scale = all_scales[tier][std::uniform_int_distribution<int>(0, 1)(gen)];

        t.scale = {scale, scale, scale};
        engine_add_component<Mesh>(e).handle = static_mesh_handles[model_idx];

        float* rgb = ((i % 2) == 0) ? rgb_cyan : rgb_blue;
        engine_add_component<EntityMaterial>(e).mat = renderer_make_lambertian_material(rgb);

        engine_add_component<Collider>(e).half_extents = {scale, scale * 0.8f, scale};
        engine_add_component<Interactable>(e);
        engine_add_component<Static>(e);

        static_count++;
    }

    int total = kNumCubes + asteroid_count + static_count;

  // --- Boundary walls: 6-sided closed box, 3x larger arena ---
    constexpr float half_x = 75.f;
    constexpr float half_z = 60.f;
    constexpr float half_y = 30.f;
    // Wall thickness must exceed max_velocity * substep_time to prevent tunneling.
    // Max velocity ~12 units/s, substep 8.3ms => 0.1 units minimum. Using 4 for margin.
    constexpr float wall_thick = 4.f;

    struct WallDef {
        glm::vec3 pos;
        glm::vec3 dim;
        bool transparent;
    };
    const WallDef wall_defs[6] = {
        // Front wall (Z+) — transparent, inner face at z=+60
        {{0.f, 0.f, half_z - wall_thick / 2.f}, {half_x * 2.f, half_y * 2.f, wall_thick}, true},
        // Back wall (Z-) — transparent, inner face at z=-60
        {{0.f, 0.f, -(half_z - wall_thick / 2.f)}, {half_x * 2.f, half_y * 2.f, wall_thick}, true},
        // Left wall (X-) — opaque, inner face at x=-75
        {{-(half_x - wall_thick / 2.f), 0.f, 0.f}, {wall_thick, half_y * 2.f, half_z * 2.f}, false},
        // Right wall (X+) — opaque, inner face at x=+75
        {{half_x - wall_thick / 2.f, 0.f, 0.f}, {wall_thick, half_y * 2.f, half_z * 2.f}, false},
        // Top wall (Y+) — opaque, inner face at y=+30
        {{0.f, half_y - wall_thick / 2.f, 0.f}, {half_x * 2.f, wall_thick, half_z * 2.f}, false},
        // Bottom floor (Y-) — opaque, inner face at y=-30
        {{0.f, -(half_y - wall_thick / 2.f), 0.f}, {half_x * 2.f, wall_thick, half_z * 2.f}, false},
    };

    float rgb_wall[3] = {0.4f, 0.35f, 0.32f};
    float rgba_fog[4] = {0.75f, 0.8f, 0.85f, 0.18f};

    for (int w = 0; w < 6; ++w) {
        auto we = engine_create_entity();
        auto& wt = engine_add_component<Transform>(we);
        wt.position = wall_defs[w].pos;
        // cube_mesh(1) spans [-1,+1] → size 2, so scale by dim/2 to get exact dimensions
        wt.scale = {0.5f * wall_defs[w].dim.x, 0.5f * wall_defs[w].dim.y, 0.5f * wall_defs[w].dim.z};

        engine_add_component<Mesh>(we).handle = renderer_make_cube_mesh(1.f);

        if (wall_defs[w].transparent) {
            float rgba[4] = {rgba_fog[0], rgba_fog[1], rgba_fog[2], rgba_fog[3]};
            engine_add_component<EntityMaterial>(we).mat = renderer_make_unlit_material(rgba);
        } else {
            engine_add_component<EntityMaterial>(we).mat = renderer_make_lambertian_material(rgb_wall);
        }

        engine_add_component<Collider>(we).half_extents = {
            0.5f * wall_defs[w].dim.x, 0.5f * wall_defs[w].dim.y, 0.5f * wall_defs[w].dim.z};
        engine_add_component<Interactable>(we);
        engine_add_component<Static>(we);
    }

    fprintf(stderr,
            "[ENGINE] DEMO-SCENE ready: 1 camera, 1 light, "
            "%d dynamic entities (%d cubes, %d asteroids), %d static obstacles\n",
            total + 4, kNumCubes, asteroid_count, static_count);
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
