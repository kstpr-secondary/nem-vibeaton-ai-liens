#include "spawn.h"
#include "constants.h"
#include <engine.h>
#include <renderer.h>
#include <glm/gtc/quaternion.hpp>

// Ship half-extent (cube mesh). Collider matches.
static constexpr float k_ship_half_extent = 1.5f;

// Sphere subdivisions per asteroid tier.
static constexpr int k_asteroid_subdivisions_small  = 2;
static constexpr int k_asteroid_subdivisions_medium = 3;
static constexpr int k_asteroid_subdivisions_large  = 3;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Uniform-sphere inverse inertia tensor: (5 / (2 * m * r^2)) * I
static glm::mat3 sphere_inv_inertia(float mass, float radius) {
    return glm::mat3(5.0f / (2.0f * mass * radius * radius));
}

// Uniform-cube inverse inertia tensor (axes-aligned): (3 / (2 * m * h^2)) * I
// where h is half-extent (full side = 2h, I_axis = (1/6)*m*(2h)^2 = (2/3)*m*h^2)
static glm::mat3 cube_inv_inertia(float mass, float half_extent) {
    return glm::mat3(3.0f / (2.0f * mass * half_extent * half_extent));
}

// ---------------------------------------------------------------------------
// spawn_player
// ---------------------------------------------------------------------------

entt::entity spawn_player(const glm::vec3& position) {
    const float  rgb[3]  = {0.3f, 0.5f, 1.0f};
    const Material mat   = renderer_make_lambertian_material(rgb);
    entt::entity e       = engine_spawn_cube(position, k_ship_half_extent, mat);

    auto& rb             = engine_add_component<RigidBody>(e);
    rb.mass              = 10.0f;
    rb.inv_mass          = 1.0f / rb.mass;
    rb.restitution       = 0.4f;
    rb.inv_inertia_body  = cube_inv_inertia(rb.mass, k_ship_half_extent);
    rb.inv_inertia       = rb.inv_inertia_body;

    auto& col            = engine_add_component<Collider>(e);
    col.half_extents     = glm::vec3(k_ship_half_extent);

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

    engine_add_component<WeaponState>(e);  // defaults match constants

    return e;
}

// ---------------------------------------------------------------------------
// spawn_enemy
// ---------------------------------------------------------------------------

entt::entity spawn_enemy(const glm::vec3& position) {
    const float  rgb[3]  = {1.0f, 0.25f, 0.2f};
    const Material mat   = renderer_make_lambertian_material(rgb);
    entt::entity e       = engine_spawn_cube(position, k_ship_half_extent, mat);

    auto& rb             = engine_add_component<RigidBody>(e);
    rb.mass              = 10.0f;
    rb.inv_mass          = 1.0f / rb.mass;
    rb.restitution       = 0.4f;
    rb.inv_inertia_body  = cube_inv_inertia(rb.mass, k_ship_half_extent);
    rb.inv_inertia       = rb.inv_inertia_body;

    auto& col            = engine_add_component<Collider>(e);
    col.half_extents     = glm::vec3(k_ship_half_extent);

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
    ws.active_weapon     = WeaponType::Plasma;  // enemies use plasma only

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
    int   subdivisions;

    switch (tier) {
        case SizeTier::Small:
            radius       = constants::asteroid_small_scale;
            mass         = constants::asteroid_small_mass;
            subdivisions = k_asteroid_subdivisions_small;
            break;
        case SizeTier::Medium:
            radius       = constants::asteroid_medium_scale;
            mass         = constants::asteroid_medium_mass;
            subdivisions = k_asteroid_subdivisions_medium;
            break;
        case SizeTier::Large:
        default:
            radius       = constants::asteroid_large_scale;
            mass         = constants::asteroid_large_mass;
            subdivisions = k_asteroid_subdivisions_large;
            break;
    }

    const float  rgb[3]  = {0.45f, 0.40f, 0.35f};
    const Material mat   = renderer_make_lambertian_material(rgb);
    entt::entity e       = engine_spawn_sphere(position, radius, mat);

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

    engine_registry().emplace<AsteroidTag>(e);
    auto& ad             = engine_add_component<AsteroidData>(e);
    ad.size_tier         = tier;

    (void)subdivisions;  // used when renderer_make_sphere_mesh is called directly
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

    engine_registry().emplace<ProjectileTag>(e);
    auto& pd             = engine_add_component<ProjectileData>(e);
    pd.owner             = owner;
    pd.damage            = constants::plasma_damage;
    pd.spawn_time        = engine_now();
    pd.lifetime          = constants::plasma_lifetime;

    return e;
}
