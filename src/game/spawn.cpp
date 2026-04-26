#include "spawn.h"
#include "constants.h"
#include <engine.h>
#include <physics.h>
#include <renderer.h>
#include <glm/gtc/quaternion.hpp>

// Model paths (relative to ASSET_ROOT).
static constexpr const char* k_player_model    = "models/spaceship1.glb";
static constexpr const char* k_enemy_model     = "models/spaceship1.glb";
static constexpr const char* k_asteroid_model  = "models/Asteroid_1a.glb";

// Scale factors per model — tuned so the ship occupies ~10% of screen width.
// glTF models are typically 50-80 Blender units; divide to fit the viewport.
static constexpr float k_player_scale          = 0.0125f;
static constexpr float k_enemy_scale           = 0.0125f;

// Approximate collider half-extents, proportional to the scaled mesh size.
// Collider should roughly match the visual AABB so collision feels correct.
static constexpr float k_player_half_extent  = 2.0f;
static constexpr float k_enemy_half_extent   = 2.0f;

// Scale factors per asteroid tier to match the constants.
static constexpr float k_asteroid_scale_small  = constants::asteroid_small_scale  / 2.0f;
static constexpr float k_asteroid_scale_medium = constants::asteroid_medium_scale / 2.0f;
static constexpr float k_asteroid_scale_large  = constants::asteroid_large_scale  / 2.0f;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

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
    reg.emplace<Transform>(e, t);

    const float default_color[3] = {0.7f, 0.7f, 0.7f};
    const Material mat = renderer_make_lambertian_material(default_color);
    reg.emplace<EntityMaterial>(e, EntityMaterial{mat});

    RendererMeshHandle handle = engine_load_gltf(model_path);
    reg.emplace<Mesh>(e, Mesh{handle});

    return e;
}

// ---------------------------------------------------------------------------
// spawn_player
// ---------------------------------------------------------------------------

entt::entity spawn_player(const glm::vec3& position) {
    entt::entity e = spawn_from_model(k_player_model, position, k_player_scale);

    auto& rb             = engine_add_component<RigidBody>(e);
    rb.mass              = 10.0f;
    rb.inv_mass          = 1.0f / rb.mass;
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

    return e;
}

// ---------------------------------------------------------------------------
// spawn_enemy
// ---------------------------------------------------------------------------

entt::entity spawn_enemy(const glm::vec3& position) {
    entt::entity e = spawn_from_model(k_enemy_model, position, k_enemy_scale);

    auto& rb             = engine_add_component<RigidBody>(e);
    rb.mass              = 10.0f;
    rb.inv_mass          = 1.0f / rb.mass;
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

    entt::entity e = spawn_from_model(k_asteroid_model, position, scale);

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

    return e;
}

// ---------------------------------------------------------------------------
// spawn_projectile
// ---------------------------------------------------------------------------

entt::entity spawn_projectile(entt::entity owner,
                                const glm::vec3& position,
                                const glm::vec3& velocity) {
    const float  rgba[4] = {1.0f, 0.75f, 0.1f, 1.0f};
    const Material mat   = renderer_make_unlit_material(rgba);
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
