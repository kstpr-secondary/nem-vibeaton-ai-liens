#pragma once

// All gameplay tuning lives here. Systems read these constants directly.
// T028 (Polish) is the designated tuning pass — change values there.

namespace constants {

// ---------------------------------------------------------------------------
// WeaponState defaults — must be first so components.h can reference them
// ---------------------------------------------------------------------------

constexpr float  laser_dps                      = 20.0f;  // damage per second while firing
constexpr float  player_health_max     = 100.0f;
constexpr float  player_shield_max     = 100.0f;
constexpr float  boost_max             = 100.0f;

// ---------------------------------------------------------------------------
// Field / containment
// ---------------------------------------------------------------------------

constexpr float  field_radius          = 350.0f;   // spherical play boundary (units)
constexpr int    asteroid_count        = 600;
constexpr float  max_asteroid_speed    = 50.0f;    // post-reflection speed cap (units/sec)

// ---------------------------------------------------------------------------
// Player flight
// ---------------------------------------------------------------------------

constexpr float  player_thrust         = 60.0f;    // forward/backward acceleration (units/sec²)
constexpr float  player_strafe         = 40.0f;    // lateral acceleration (units/sec²)
constexpr float  player_drag_coeff     = 23.4f;    // continuous drag force coefficient (F = -c*v)
constexpr float  player_turn_speed     = 0.002f;   // mouse look sensitivity (radians/pixel, no dt factor)
constexpr float  boost_multiplier      = 2.0f;     // thrust scale while boost active

// ---------------------------------------------------------------------------
// Player resources
// ---------------------------------------------------------------------------

constexpr float  shield_regen_rate     = 2.0f;     // units/sec
constexpr float  shield_regen_delay    = 3.0f;     // seconds after last damage before regen starts
constexpr float  boost_drain_rate      = 20.0f;    // units/sec (~5s full drain)
constexpr float  boost_regen_rate      = 5.0f;     // units/sec (~20s full regen)

// ---------------------------------------------------------------------------
// Weapons — laser
// ---------------------------------------------------------------------------

constexpr float  laser_damage          = 10.0f;
constexpr float  laser_cooldown        = 5.0f;     // seconds
constexpr float  laser_max_range       = 800.0f;   // units
constexpr float  laser_line_width      = 0.15f;    // visual quad width (units)
constexpr float  laser_charge_time             = 0.8f;   // hold duration before firing
constexpr float  laser_fire_duration             = 3.0f;   // active damage window
constexpr float  laser_fade_duration             = 0.5f;   // glow-down after depletion
constexpr float  laser_impulse_per_second       = 30.0f;  // asteroid impulse per second
constexpr float  laser_core_width               = 0.3f;
constexpr float  laser_halo_width               = 2.5f;
constexpr float  laser_core_color[4]            = {1.0f, 1.0f, 0.9f, 1.0f};
constexpr float  laser_halo_color[4]            = {0.3f, 0.85f, 1.0f, 1.0f};

// ---------------------------------------------------------------------------
// Weapons — plasma
// ---------------------------------------------------------------------------

constexpr float  plasma_damage         = 1.0f;
constexpr float  plasma_cooldown       = 0.1f;     // seconds (~10 shots/sec)
constexpr float  plasma_speed          = 80.0f;    // units/sec
constexpr float  plasma_lifetime       = 3.0f;     // seconds before auto-despawn
constexpr float  plasma_sphere_radius  = 0.4f;     // projectile mesh radius (units)

// ---------------------------------------------------------------------------
// Enemy AI
// ---------------------------------------------------------------------------

constexpr float  enemy_health_max      = 100.0f;
constexpr float  enemy_shield_max      = 50.0f;
constexpr float  enemy_pursuit_speed   = 12.0f;    // units/sec seek velocity
constexpr float  enemy_fire_range      = 300.0f;   // units — fire only within this distance
constexpr float  enemy_fire_cooldown   = 0.5f;     // seconds between enemy plasma shots

// ---------------------------------------------------------------------------
// Asteroid size tiers — Small
// ---------------------------------------------------------------------------

constexpr float  asteroid_small_scale      = 4.5f;
constexpr float  asteroid_small_mass       = 15.0f;
constexpr float  asteroid_small_speed_min  = 5.0f;
constexpr float  asteroid_small_speed_max  = 15.0f;
constexpr float  asteroid_small_spin_max   = 1.5f;   // angular velocity magnitude (rad/sec)

// ---------------------------------------------------------------------------
// Asteroid size tiers — Medium
// ---------------------------------------------------------------------------

constexpr float  asteroid_medium_scale     = 9.0f;
constexpr float  asteroid_medium_mass      = 70.0f;
constexpr float  asteroid_medium_speed_min = 3.0f;
constexpr float  asteroid_medium_speed_max = 10.0f;
constexpr float  asteroid_medium_spin_max  = 0.8f;

// ---------------------------------------------------------------------------
// Asteroid size tiers — Large
// ---------------------------------------------------------------------------

constexpr float  asteroid_large_scale      = 36.0f;
constexpr float  asteroid_large_mass       = 1620.0f;
constexpr float  asteroid_large_speed_min  = 1.0f;
constexpr float  asteroid_large_speed_max  = 5.0f;
constexpr float  asteroid_large_spin_max   = 0.3f;

// Tier distribution out of asteroid_count (must sum to 1.0)
constexpr float  asteroid_tier_small_frac  = 0.35f; // 210 small (from 600 total)
constexpr float  asteroid_tier_medium_frac = 0.30f; // 180 medium
// large fills the remainder                         // 210 large

// ---------------------------------------------------------------------------
// Shield VFX
// ---------------------------------------------------------------------------

constexpr float  shield_sphere_scale   = 1.45f;
constexpr float  shield_max_alpha      = 0.55f;
constexpr float  shield_fresnel_exp    = 3.0f;
constexpr float  shield_rim_intensity  = 1.2f;

// Impact VFX durations
constexpr float  plasma_impact_duration  = 0.5f;
constexpr float  laser_impact_duration   = 0.4f;
constexpr float  shield_impact_duration  = 0.25f;

// ---------------------------------------------------------------------------
// Camera rig (third-person)
// ---------------------------------------------------------------------------

constexpr float  cam_offset_back       = 30.0f;    // units behind player
constexpr float  cam_offset_up         = 10.0f;    // units above player
constexpr float  cam_lag_factor        = 8.0f;     // lerp rate (higher = tighter follow)

// ---------------------------------------------------------------------------
// Camera rig V2
// ---------------------------------------------------------------------------

constexpr float  cam_turn_rate          = 0.6f;    // rad/sec per unit of mouse delta
constexpr float  cam_look_ahead         = 25.0f;   // units ahead of ship that camera aims at
constexpr float  cam_look_up_bias       = 8.0f;    // units above look-ahead point (ships toward screen bottom)
constexpr float  visual_bank_gain       = 0.35f;   // radians of bank per pixel/sec of lateral steering
constexpr float  visual_bank_max        = 0.55f;   // radians (~31°) maximum bank angle
constexpr float  visual_bank_spring     = 5.0f;    // spring rate for bank return (rad/sec²/rad)
constexpr float  collision_roll_gain    = 0.008f;  // radians of roll per unit of KE impulse
constexpr float  collision_roll_max     = 1.0f;    // radians max collision roll
constexpr float  collision_roll_spring  = 3.5f;    // spring rate for collision roll return

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
