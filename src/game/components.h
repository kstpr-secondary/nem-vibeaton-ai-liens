#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <limits>

// ---------------------------------------------------------------------------
// Enums
// ---------------------------------------------------------------------------

enum class WeaponType : uint8_t {
    Laser = 0,
    Plasma = 1,
};

enum class SizeTier : uint8_t {
    Small  = 0,
    Medium = 1,
    Large  = 2,
};

enum class MatchPhase : uint8_t {
    Playing    = 0,
    PlayerDead = 1,
    Victory    = 2,
    Restarting = 3,
};

// ---------------------------------------------------------------------------
// Tag components (empty — presence is semantic)
// ---------------------------------------------------------------------------

struct PlayerTag    {};
struct EnemyTag     {};
struct AsteroidTag  {};
struct ProjectileTag {};

// ---------------------------------------------------------------------------
// State components
// ---------------------------------------------------------------------------

struct Health {
    float current = 100.0f;
    float max     = 100.0f;
};

struct Shield {
    float  current          = 100.0f;
    float  max              = 100.0f;
    float  regen_rate       = 2.0f;
    float  regen_delay      = 3.0f;
    double last_damage_time = -std::numeric_limits<double>::infinity();
};

struct Boost {
    float current    = 100.0f;
    float max        = 100.0f;
    float drain_rate = 20.0f;
    float regen_rate = 5.0f;
    bool  active     = false;
};

struct CameraRigState {
    glm::quat rig_rotation           = glm::quat(1.f, 0.f, 0.f, 0.f);
    float     visual_bank            = 0.f;
    float     collision_roll         = 0.f;
    float     collision_roll_vel     = 0.f;
    float     collision_roll_impulse = 0.f;
};

struct WeaponState {
    WeaponType active_weapon   = WeaponType::Laser;
    float      laser_cooldown  = 5.0f;
    double     laser_last_fire = -std::numeric_limits<double>::infinity();
    float      laser_dps       = 20.0f;
    float      plasma_cooldown  = 0.1f;
    double     plasma_last_fire = -std::numeric_limits<double>::infinity();
    float      plasma_damage    = 0.5f;
    float      plasma_speed     = 200.0f;
    float      plasma_lifetime  = 3.0f;
};

struct ProjectileData {
    entt::entity owner      = entt::null;
    float        damage     = 0.5f;
    double       spawn_time = 0.0;
    float        lifetime   = 3.0f;
};

struct EnemyAI {
    float  pursuit_speed  = 30.0f;
    float  fire_range     = 300.0f;
    float  fire_cooldown  = 0.5f;
    double last_fire_time = -std::numeric_limits<double>::infinity();
};

struct AsteroidData {
    SizeTier size_tier = SizeTier::Medium;
};

// Attached to short-lived VFX entities (explosions, etc.).
// When engine_now() - spawn_time >= lifetime, the entity is marked DestroyPending.
struct Lifetime {
    double spawn_time = 0.0;
    float  lifetime   = 1.0f;  // seconds
};

// Visual shield sphere — follows owner transform, alpha driven by shield health.
struct ShieldSphere {
    entt::entity owner  = entt::null;
    float        radius = 0.0f;
};

struct VFXData {
    float initial_scale = 1.0f;
    float final_scale   = 5.0f;
    float initial_alpha = 1.0f;
    float final_alpha   = 0.0f;
    float r = 1.0f, g = 0.5f, b = 0.1f;
};

struct LaserBeam {
    entt::entity owner      = entt::null;
    glm::vec3    start      = {};
    glm::vec3    end        = {};
    bool         active     = false;
    float        charge_t   = 0.0f;
    float        fire_t     = 0.0f;
    float        fade_t     = 0.0f;
    double       spawn_time = 0.0;
    entt::entity last_hit_entity = entt::null;
};

struct LaserCharge {
    double start_time = 0.0;
};
