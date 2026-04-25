# Game API Contract

**Date**: 2026-04-23
**Status**: Draft — to be promoted to `docs/interfaces/game-interface-spec.md` after G-M1 freeze
**Upstream**: Engine interface (`engine-interface-spec.md`, FROZEN v1.1), Renderer interface (`renderer-interface-spec.md`, FROZEN v1.1)

---

## Overview

The game exposes a minimal lifecycle API. The entry point (`main.cpp`) initializes renderer and engine, registers a frame callback and input callback, then calls `renderer_run()` (blocking). Inside the frame callback, the game calls `game_tick(dt)` which drives all gameplay systems. The game is the final consumer in the dependency chain — nothing depends on its API.

---

## Public API (`src/game/game.h`)

```cpp
#pragma once

// =========================================================================
// Game Lifecycle
// =========================================================================

/// Initialize all game state: spawn player, enemy, asteroid field, set up
/// camera and light. Must be called after engine_init() and before
/// renderer_run().
void game_init();

/// Advance all gameplay systems by dt seconds. Called inside the renderer
/// FrameCallback between renderer_begin_frame() and renderer_end_frame().
/// Internally calls engine_tick(dt) and submits draw commands.
///
/// System execution order per tick:
///   1. engine_tick(dt)              — physics, collision, entity cleanup
///   2. containment_update()         — boundary reflection + speed cap
///   3. player_update(dt)            — flight controls, boost, resource regen
///   4. enemy_ai_update(dt)          — seek + shoot logic
///   5. weapon_update(dt)            — fire processing, cooldown advancement
///   6. projectile_update(dt)        — lifetime check, despawn expired
///   7. damage_resolve()             — collision damage + projectile damage → shield/HP
///   8. match_state_update(dt)       — phase transitions, win/loss check, restart
///   9. camera_rig_update(dt)        — follow player with offset + lag
///  10. render_submit()              — enqueue_draw for all visible entities
///  11. hud_render()                 — ImGui widgets + overlays
void game_tick(float dt);

/// Tear down all game state. Called after renderer_run() returns (or on
/// fatal error). Destroys all game-created entities.
void game_shutdown();
```

---

## Calling Convention

```
main():
  renderer_init(config)
  engine_init(engine_config)
  game_init()
  renderer_set_frame_callback(frame_cb, nullptr)
  renderer_set_input_callback(input_cb, nullptr)
  renderer_run()           // blocking — calls frame_cb each frame
  game_shutdown()
  engine_shutdown()
  renderer_shutdown()

frame_cb(dt, user_data):
  renderer_begin_frame()
  game_tick(dt)            // calls engine_tick(dt) internally
  renderer_end_frame()

input_cb(event, user_data):
  // forward to engine input handler (engine owns input state)
  engine_handle_input(event)
```

---

## System Execution Order Detail

The order within `game_tick(dt)` is critical for correctness:

1. **engine_tick(dt)**: Runs physics substeps, detects collisions, cleans up `DestroyPending` entities. Must run first so game systems see updated positions and collision results.
2. **containment_update()**: Reads asteroid/player positions, reflects velocities at boundary, caps speeds. Must run after physics so positions are current.
3. **player_update(dt)**: Reads input, applies thrust/strafe/rotation, manages boost drain/regen. Must run after containment (player might have been reflected).
4. **enemy_ai_update(dt)**: Computes seek vector toward player, checks fire conditions. Must run after player_update (needs current player position).
5. **weapon_update(dt)**: Processes fire inputs (player) and AI fire decisions (enemy), spawns projectiles or performs raycasts, advances cooldowns.
6. **projectile_update(dt)**: Checks lifetimes, despawns expired projectiles.
7. **damage_resolve()**: Reads collision events from current tick, applies weapon damage from projectile hits and laser raycasts, resolves shield → HP cascade, detects deaths, despawns dead entities.
8. **match_state_update(dt)**: Checks enemy count for victory, checks player HP for death, manages phase transitions and restart countdown. Enemy deaths evaluated before player death (simultaneous death → player wins).
9. **camera_rig_update(dt)**: Follows player position with configurable offset and lag. Sets active camera via `engine_set_active_camera`.
10. **render_submit()**: Iterates all entities with Transform + Mesh + EntityMaterial, calls `renderer_enqueue_draw` for each. Also submits laser line visuals via `renderer_enqueue_line_quad`.
11. **hud_render()**: Builds ImGui widgets for HP/shield/boost bars, crosshair, weapon indicator, cooldown display, and win/death overlays.

---

## Input Mapping

The game does not own input event handling directly. It polls input state via the engine API:

| Game Action | Engine API Call | Condition |
|-------------|----------------|-----------|
| Pitch/Yaw | `engine_mouse_delta()` | `engine_mouse_button(0)` held (left mouse) |
| Fire | Weapon system checks | `engine_mouse_button(1)` held (right mouse) |
| Thrust forward | `engine_key_down(SAPP_KEYCODE_W)` | Per-frame poll |
| Thrust reverse | `engine_key_down(SAPP_KEYCODE_S)` | Per-frame poll |
| Strafe left | `engine_key_down(SAPP_KEYCODE_A)` | Per-frame poll |
| Strafe right | `engine_key_down(SAPP_KEYCODE_D)` | Per-frame poll |
| Boost | `engine_key_down(SAPP_KEYCODE_SPACE)` | Per-frame poll |
| Switch to Plasma | `engine_key_down(SAPP_KEYCODE_Q)` | Edge-triggered (track prev state) |
| Switch to Laser | `engine_key_down(SAPP_KEYCODE_E)` | Edge-triggered (track prev state) |
| Restart | `engine_key_down(SAPP_KEYCODE_ENTER)` | Edge-triggered |
| Quit | `engine_key_down(SAPP_KEYCODE_ESCAPE)` | Edge-triggered |

---

## Error Behavior

| Situation | Behavior |
|-----------|----------|
| `game_tick` called before `game_init` | UB — not guarded; caller responsibility |
| Engine returns invalid mesh handle for asset load | Entity spawned with invalid mesh; renderer silently skips draw |
| Raycast hits nothing (laser misses) | Line visual drawn to max distance; no damage applied |
| Projectile collides with its owner | Ignored — projectile owner is excluded from damage |
| Player entity destroyed unexpectedly | Match transitions to PlayerDead |
| No enemies at init (config error) | Immediate Victory transition |
| `game_shutdown` called twice | Silently ignored (entities already destroyed) |

---

## Mock Surface

The game does not provide mocks — it is the final consumer. The game itself runs against engine mocks (`USE_ENGINE_MOCKS=ON`) and renderer mocks (`USE_RENDERER_MOCKS=ON`) during early development before upstream milestones land.
