#include "spawn.h"
#include "constants.h"
#include "shield_vfx.h"
#include "vfx.h"
#include "game_shaders.h"
#include <engine.h>
#include <physics.h>
#include <renderer.h>
#include <glm/gtc/quaternion.hpp>

#include <cstdlib>
#include <cstring>

// Model paths (relative to ASSET_ROOT).
static constexpr const char* k_player_model    = "spaceship1_mod2.glb";
static constexpr const char* k_enemy_model     = "spaceship1_mod2.glb";

static constexpr const char* k_asteroid_models[] = {
    "Asteroid_1a.glb",
    "Asteroid_1e.glb",
    "Asteroid_2a.glb",
    "Asteroid_2b.glb",
};
static constexpr int k_asteroid_model_count = 4;

// Scale factors per model — tuned so the ship occupies ~10% of screen width.
// glTF models are typically 50-80 Blender units; divide to fit the viewport.
static constexpr float k_player_scale          = 0.0125f;
static constexpr float k_enemy_scale           = 0.0125f;

// Approximate collider half-extents, proportional to the scaled mesh size.
// Collider should roughly match the visual AABB so collision feels correct.
static constexpr float k_player_half_extent  = 2.0f;
static constexpr float k_enemy_half_extent   = 2.0f;

// Base orientation to map glTF model (+Y up, nose along +Y) to game space (-Z forward, +Y up).
// This converts model +Y to game -Z by flipping around Z (making it face backwards on Z) 
// and then pitching 90 degrees down.
static const glm::quat k_ship_base_orientation = 
    glm::angleAxis(glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f)) * 
    glm::angleAxis(glm::pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f));

// Scale factors per asteroid tier to match the constants.
static constexpr float k_asteroid_scale_small  = constants::asteroid_small_scale  / 2.0f;
static constexpr float k_asteroid_scale_medium = constants::asteroid_medium_scale / 2.0f;
static constexpr float k_asteroid_scale_large  = constants::asteroid_large_scale  / 2.0f;

// Asteroid base color variations — applied to the sampled albedo texture so
// lit faces are not uniformly white-gray. The grayscale asteroid textures
// average ~0.31, and lighting pushes values toward white, so multipliers
// must be aggressive enough to survive the full lighting pipeline.
static const float* asteroid_color_variation() {
    int r = std::rand() % 100;
    if (r < 25) {
        // Light warm gray — slight beige/brown tint
        static const float c[3] = {0.88f, 0.82f, 0.76f};
        return c;
    } else if (r < 50) {
        // Cool gray — slightly blue-ish
        static const float c[3] = {0.72f, 0.74f, 0.78f};
        return c;
    } else if (r < 75) {
        // Medium warm gray — brown/ochre tint
        static const float c[3] = {0.84f, 0.76f, 0.68f};
        return c;
    } else {
        // Neutral medium gray
        static const float c[3] = {0.70f, 0.70f, 0.70f};
        return c;
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static bool is_asteroid_model(const char* path) {
    for (int i = 0; i < k_asteroid_model_count; ++i) {
        if (strcmp(path, k_asteroid_models[i]) == 0)
            return true;
    }
    return false;
}

static const char* random_asteroid_model() {
    // Simple modulo-based random — seeded by std::rand which is fine for this use.
    static int s_idx = 0;
    ++s_idx;
    return k_asteroid_models[s_idx % k_asteroid_model_count];
}

// Uniform-sphere inverse inertia tensor: (5 / (2 * m * r^2)) * I
static glm::mat3 sphere_inv_inertia(float mass, float radius) {
    return glm::mat3(5.0f / (2.0f * mass * radius * radius));
}

// Uniform-box inverse inertia tensor.
static glm::mat3 box_inv_inertia(float mass, float half_extent) {
    return glm::mat3(3.0f / (2.0f * mass * half_extent * half_extent));
}

// Spawn an entity from a GLB model at the given position and scale.
static entt::entity spawn_from_model(const char* model_path,
                                      const glm::vec3& position,
                                      float scale) {
    auto& reg = engine_registry();
    auto e = reg.create();

    Transform t{};
    t.position = position;
    t.scale    = glm::vec3(scale);
    t.rotation = glm::quat(1.f, 0.f, 0.f, 0.f);  // identity — caller may override
    reg.emplace<Transform>(e, t);

    RendererTextureHandle texture_handle = {};
    RendererMeshHandle handle = engine_load_gltf(model_path, &texture_handle);

    const float* base_color;
    float shininess;

    if (texture_handle.id != 0) {
        if (is_asteroid_model(model_path)) {
            base_color = asteroid_color_variation(); // tint sampled albedo
            shininess  = 64.0f;
        } else {
            base_color = nullptr;  // use texture directly
            shininess  = 128.0f;
        }
    } else {
        if (is_asteroid_model(model_path)) {
            base_color = nullptr;  // fallback handled below
            shininess  = 64.0f;
        } else {
            static const float s_player_fallback[3] = {0.5f, 0.55f, 0.6f};
            static const float s_enemy_fallback[3]  = {0.55f, 0.3f, 0.3f};
            base_color = (strcmp(model_path, k_player_model) == 0) ? s_player_fallback : s_enemy_fallback;
            shininess  = 128.0f;
        }
    }

  Material mat;
    if (texture_handle.id != 0) {
        const float* rgb = (base_color != nullptr) ? base_color : nullptr;
        mat = renderer_make_blinnphong_material(rgb, shininess, texture_handle);
        // For textured asteroids: blend 50% procedural color with 50% sampled texture.
        // base_color.a encodes the blend factor (1.0 = fully procedural, 0.0 = fully textured).
        if (is_asteroid_model(model_path)) {
            auto* p = material_uniforms_as<BlinnPhongFSParams>(mat);
            p->base_color.a = 0.5f;
            p->spec_shin.w  = ((float)std::rand() / (float)RAND_MAX) * 64.0f;
        } else {
            auto* p = material_uniforms_as<BlinnPhongFSParams>(mat);
            p->base_color.a = 0.0f;
        }
    } else {
        static const float s_fallback_gray[3] = {0.7f, 0.7f, 0.7f};
        const float* color_ptr = (base_color != nullptr) ? base_color : s_fallback_gray;
        mat = renderer_make_blinnphong_material(color_ptr, shininess, {});
    }
    reg.emplace<EntityMaterial>(e, EntityMaterial{mat});

    reg.emplace<Mesh>(e, Mesh{handle});

    return e;
}

// ---------------------------------------------------------------------------
// spawn_shield_sphere
// ---------------------------------------------------------------------------

static entt::entity spawn_shield_sphere(entt::entity owner, float half_extent) {
    float radius = half_extent * constants::shield_sphere_scale;

    Material mat{};
    mat.shader = g_shield_shader;
    mat.pipeline = { BlendMode::AlphaBlend, CullMode::Off, false, 2 };
    ShieldFSParams p{};
    p.shield_color  = {0.3f, 0.5f, 1.0f, constants::shield_max_alpha};
    p.view_pos_w    = {};
    p.fresnel_params = {constants::shield_fresnel_exp, constants::shield_rim_intensity, 0, 0};
    material_set_uniforms(mat, p);

    const auto& ot = engine_get_component<Transform>(owner);
    entt::entity e = engine_spawn_sphere(ot.position, radius, mat);
    auto& ss = engine_add_component<ShieldSphere>(e);
    ss.owner  = owner;
    ss.radius = radius;
    return e;
}

// ---------------------------------------------------------------------------
// spawn_player
// ---------------------------------------------------------------------------

entt::entity spawn_player(const glm::vec3& position) {
    entt::entity e = spawn_from_model(k_player_model, position, k_player_scale);
    engine_get_component<Transform>(e).rotation = k_ship_base_orientation;

    auto& rb             = engine_add_component<RigidBody>(e);
    rb.mass              = 10.0f;
    rb.inv_mass          = 1.0f / rb.mass;
    rb.linear_velocity   = glm::vec3(0.f, 0.f, 0.f);
    rb.angular_velocity  = glm::vec3(0.f, 0.f, 0.f);
    rb.restitution       = 0.4f;
    rb.inv_inertia_body  = box_inv_inertia(rb.mass, k_player_half_extent);
    rb.inv_inertia       = rb.inv_inertia_body;

    auto& col            = engine_add_component<Collider>(e);
    col.half_extents     = glm::vec3(k_player_half_extent);

    engine_registry().emplace<Dynamic>(e);
    engine_registry().emplace<ForceAccum>(e);
    engine_registry().emplace<PlayerTag>(e);
    auto& hp             = engine_add_component<Health>(e);
    hp.current           = constants::player_health_max;
    hp.max               = constants::player_health_max;

    auto& sh             = engine_add_component<Shield>(e);
    sh.current           = constants::player_shield_max;
    sh.max               = constants::player_shield_max;
    sh.regen_rate        = constants::shield_regen_rate;
    sh.regen_delay       = constants::shield_regen_delay;

    auto& boost          = engine_add_component<Boost>(e);
    boost.current        = constants::boost_max;
    boost.max            = constants::boost_max;
    boost.drain_rate     = constants::boost_drain_rate;
    boost.regen_rate     = constants::boost_regen_rate;

    engine_add_component<WeaponState>(e);
    engine_add_component<CameraRigState>(e);

    engine_registry().emplace<Interactable>(e);

    spawn_shield_sphere(e, k_player_half_extent);

    return e;
}

// ---------------------------------------------------------------------------
// spawn_enemy
// ---------------------------------------------------------------------------

entt::entity spawn_enemy(const glm::vec3& position) {
    entt::entity e = spawn_from_model(k_enemy_model, position, k_enemy_scale);
    engine_get_component<Transform>(e).rotation = k_ship_base_orientation;

    auto& rb             = engine_add_component<RigidBody>(e);
    rb.mass              = 10.0f;
    rb.inv_mass          = 1.0f / rb.mass;
    rb.linear_velocity   = glm::vec3(0.f, 0.f, 0.f);
    rb.angular_velocity  = glm::vec3(0.f, 0.f, 0.f);
    rb.restitution       = 0.4f;
    rb.inv_inertia_body  = box_inv_inertia(rb.mass, k_enemy_half_extent);
    rb.inv_inertia       = rb.inv_inertia_body;

    auto& col            = engine_add_component<Collider>(e);
    col.half_extents     = glm::vec3(k_enemy_half_extent);

    engine_registry().emplace<Dynamic>(e);
    engine_registry().emplace<ForceAccum>(e);
    engine_registry().emplace<EnemyTag>(e);

    auto& hp             = engine_add_component<Health>(e);
    hp.current           = constants::enemy_health_max;
    hp.max               = constants::enemy_health_max;

    auto& sh             = engine_add_component<Shield>(e);
    sh.current           = constants::enemy_shield_max;
    sh.max               = constants::enemy_shield_max;
    sh.regen_rate        = constants::shield_regen_rate;
    sh.regen_delay       = constants::shield_regen_delay;

    auto& ws             = engine_add_component<WeaponState>(e);
    ws.active_weapon     = WeaponType::Plasma;

    auto& ai             = engine_add_component<EnemyAI>(e);
    ai.pursuit_speed     = constants::enemy_pursuit_speed;
    ai.fire_range        = constants::enemy_fire_range;
    ai.fire_cooldown     = constants::enemy_fire_cooldown;

    engine_registry().emplace<Interactable>(e);

    spawn_shield_sphere(e, k_enemy_half_extent);

    return e;
}

// ---------------------------------------------------------------------------
// spawn_asteroid
// ---------------------------------------------------------------------------

entt::entity spawn_asteroid(const glm::vec3& position,
                              const glm::vec3& linear_velocity,
                              const glm::vec3& angular_velocity,
                              SizeTier tier) {
    float radius;
    float mass;
    float scale;

    switch (tier) {
        case SizeTier::Small:
            radius = constants::asteroid_small_scale;
            mass   = constants::asteroid_small_mass;
            scale  = k_asteroid_scale_small;
            break;
        case SizeTier::Medium:
            radius = constants::asteroid_medium_scale;
            mass   = constants::asteroid_medium_mass;
            scale  = k_asteroid_scale_medium;
            break;
        case SizeTier::Large:
        default:
            radius = constants::asteroid_large_scale;
            mass   = constants::asteroid_large_mass;
            scale  = k_asteroid_scale_large;
            break;
    }

    const char* model = random_asteroid_model();
    entt::entity e = spawn_from_model(model, position, scale);

    auto& rb             = engine_add_component<RigidBody>(e);
    rb.mass              = mass;
    rb.inv_mass          = 1.0f / mass;
    rb.linear_velocity   = linear_velocity;
    rb.angular_velocity  = angular_velocity;
    rb.restitution       = 0.8f;
    rb.inv_inertia_body  = sphere_inv_inertia(mass, radius);
    rb.inv_inertia       = rb.inv_inertia_body;

    auto& col            = engine_add_component<Collider>(e);
    col.half_extents     = glm::vec3(radius);

    engine_registry().emplace<Dynamic>(e);
    engine_registry().emplace<ForceAccum>(e);
    engine_registry().emplace<AsteroidTag>(e);
    auto& ad             = engine_add_component<AsteroidData>(e);
    ad.size_tier         = tier;

    engine_registry().emplace<Interactable>(e);

    return e;
}

// ---------------------------------------------------------------------------
// spawn_projectile
// ---------------------------------------------------------------------------

entt::entity spawn_projectile(entt::entity owner,
                                const glm::vec3& position,
                                const glm::vec3& velocity) {
    Material mat{};
    mat.shader   = g_plasma_shader;
    mat.pipeline = { BlendMode::Additive, CullMode::Off, false, 3 };

    PlasmaBallFSParams p{};
    p.core_color  = {1.0f, 1.0f, 0.9f, 1.0f};
    p.rim_color   = {0.4f, 0.6f, 1.0f, 1.0f};
    p.bolt_color  = {0.5f, 0.7f, 1.0f, 1.0f};
    p.params      = {0.0f, 3.0f, 0.65f, 1.0f};
    p.view_pos_w  = {};
    material_set_uniforms(mat, p);

    entt::entity e       = engine_spawn_sphere(position,
                                                 constants::plasma_sphere_radius, mat);

    auto& rb             = engine_add_component<RigidBody>(e);
    rb.mass              = 0.1f;
    rb.inv_mass          = 1.0f / rb.mass;
    rb.linear_velocity   = velocity;
    rb.restitution       = 0.0f;
    rb.inv_inertia_body  = sphere_inv_inertia(rb.mass, constants::plasma_sphere_radius);
    rb.inv_inertia       = rb.inv_inertia_body;

    auto& col            = engine_add_component<Collider>(e);
    col.half_extents     = glm::vec3(constants::plasma_sphere_radius);

    engine_registry().emplace<Dynamic>(e);
    engine_registry().emplace<ForceAccum>(e);
    engine_registry().emplace<ProjectileTag>(e);
    auto& pd             = engine_add_component<ProjectileData>(e);
    pd.owner             = owner;
    pd.damage            = constants::plasma_damage;
    pd.spawn_time        = engine_now();
    pd.lifetime          = constants::plasma_lifetime;

    return e;
}
