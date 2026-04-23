#pragma once

#include <entt/entt.hpp>
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

struct WeaponState {
    WeaponType active_weapon   = WeaponType::Laser;
    float      laser_cooldown  = 5.0f;
    double     laser_last_fire = -std::numeric_limits<double>::infinity();
    float      laser_damage    = 10.0f;
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
