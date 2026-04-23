#pragma once

// All gameplay tuning lives here. Systems read these constants directly.
// T028 (Polish) is the designated tuning pass — change values there.

namespace constants {

// ---------------------------------------------------------------------------
// Field / containment
// ---------------------------------------------------------------------------

constexpr float  field_radius          = 1000.0f;  // spherical play boundary (units)
constexpr int    asteroid_count        = 200;
constexpr float  max_asteroid_speed    = 50.0f;    // post-reflection speed cap (units/sec)

// ---------------------------------------------------------------------------
// Player flight
// ---------------------------------------------------------------------------

constexpr float  player_thrust         = 60.0f;    // forward/backward acceleration (units/sec²)
constexpr float  player_strafe         = 40.0f;    // lateral acceleration (units/sec²)
constexpr float  player_drag           = 0.97f;    // per-frame velocity damping factor
constexpr float  player_turn_speed     = 1.8f;     // mouse sensitivity (radians/pixel·sec)
constexpr float  boost_multiplier      = 2.0f;     // thrust scale while boost active

// ---------------------------------------------------------------------------
// Player resources
// ---------------------------------------------------------------------------

constexpr float  player_health_max     = 100.0f;
constexpr float  player_shield_max     = 100.0f;
constexpr float  shield_regen_rate     = 2.0f;     // units/sec
constexpr float  shield_regen_delay    = 3.0f;     // seconds after last damage before regen starts
constexpr float  boost_max             = 100.0f;
constexpr float  boost_drain_rate      = 20.0f;    // units/sec (~5s full drain)
constexpr float  boost_regen_rate      = 5.0f;     // units/sec (~20s full regen)

// ---------------------------------------------------------------------------
// Weapons — laser
// ---------------------------------------------------------------------------

constexpr float  laser_damage          = 10.0f;
constexpr float  laser_cooldown        = 5.0f;     // seconds
constexpr float  laser_max_range       = 800.0f;   // units
constexpr float  laser_line_width      = 0.15f;    // visual quad width (units)

// ---------------------------------------------------------------------------
// Weapons — plasma
// ---------------------------------------------------------------------------

constexpr float  plasma_damage         = 0.5f;
constexpr float  plasma_cooldown       = 0.1f;     // seconds (~10 shots/sec)
constexpr float  plasma_speed          = 200.0f;   // units/sec
constexpr float  plasma_lifetime       = 3.0f;     // seconds before auto-despawn
constexpr float  plasma_sphere_radius  = 0.4f;     // projectile mesh radius (units)

// ---------------------------------------------------------------------------
// Enemy AI
// ---------------------------------------------------------------------------

constexpr float  enemy_health_max      = 100.0f;
constexpr float  enemy_shield_max      = 50.0f;
constexpr float  enemy_pursuit_speed   = 30.0f;    // units/sec seek velocity
constexpr float  enemy_fire_range      = 300.0f;   // units — fire only within this distance
constexpr float  enemy_fire_cooldown   = 0.5f;     // seconds between enemy plasma shots

// ---------------------------------------------------------------------------
// Asteroid size tiers — Small
// ---------------------------------------------------------------------------

constexpr float  asteroid_small_scale      = 2.0f;
constexpr float  asteroid_small_mass       = 5.0f;
constexpr float  asteroid_small_speed_min  = 5.0f;
constexpr float  asteroid_small_speed_max  = 15.0f;
constexpr float  asteroid_small_spin_max   = 1.5f;   // angular velocity magnitude (rad/sec)

// ---------------------------------------------------------------------------
// Asteroid size tiers — Medium
// ---------------------------------------------------------------------------

constexpr float  asteroid_medium_scale     = 5.0f;
constexpr float  asteroid_medium_mass      = 20.0f;
constexpr float  asteroid_medium_speed_min = 3.0f;
constexpr float  asteroid_medium_speed_max = 10.0f;
constexpr float  asteroid_medium_spin_max  = 0.8f;

// ---------------------------------------------------------------------------
// Asteroid size tiers — Large
// ---------------------------------------------------------------------------

constexpr float  asteroid_large_scale      = 10.0f;
constexpr float  asteroid_large_mass       = 60.0f;
constexpr float  asteroid_large_speed_min  = 1.0f;
constexpr float  asteroid_large_speed_max  = 5.0f;
constexpr float  asteroid_large_spin_max   = 0.3f;

// Tier distribution out of asteroid_count (must sum to 1.0)
constexpr float  asteroid_tier_small_frac  = 0.5f;  // 100 small
constexpr float  asteroid_tier_medium_frac = 0.35f; // 70 medium
// large fills the remainder                         // 30 large

// ---------------------------------------------------------------------------
// Camera rig (third-person)
// ---------------------------------------------------------------------------

constexpr float  cam_offset_back       = 15.0f;    // units behind player
constexpr float  cam_offset_up         = 5.0f;     // units above player
constexpr float  cam_lag_factor        = 8.0f;     // lerp rate (higher = tighter follow)

// ---------------------------------------------------------------------------
// Match state / timers
// ---------------------------------------------------------------------------

constexpr int    enemies_at_start      = 1;
constexpr float  restart_delay_death   = 2.0f;     // seconds before auto-restart after death
constexpr float  restart_delay_win     = 3.0f;     // seconds before auto-restart after win

// ---------------------------------------------------------------------------
// Collision damage scaling
// ---------------------------------------------------------------------------

constexpr float  kinetic_damage_scale  = 0.001f;   // multiply raw KE by this before applying to shield/HP

} // namespace constants
