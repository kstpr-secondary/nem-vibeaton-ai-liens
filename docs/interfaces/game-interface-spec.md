# Game Interface Spec

> **Status:** `DRAFT — v0.1` (promoted from Game SpecKit `contracts/game-api.md` on 2026-04-26)
> **Source**: `docs/planning/speckit/game/contracts/game-api.md` (SpecKit cycle `specs/003-space-shooter-mvp/`)
> **Upstream dependencies**:
> - Renderer interface spec (`renderer-interface-spec.md`, **FROZEN — v1.1**)
> - Engine interface spec (`engine-interface-spec.md`, **FROZEN — v1.2**)

---

## Version

`v0.1-draft`

### Changelog

- **v0.1-draft** (2026-04-26): Initial promotion from Game SpecKit contracts. Lifecycle API defined; system execution order locked; input mapping documented; error behavior enumerated.

---

## Freeze Rules

- This document becomes authoritative only after a human supervisor sets `**Status**: FROZEN — v1.0` at the top of the file.
- The game is the **final consumer** in the dependency chain; no other workstream depends on this interface, so freezing is informational rather than blocking.
- Any post-freeze change to the lifecycle / system order / input mapping requires human approval and a version bump.
- A change in the `game_tick()` system order is not a syntactic interface change but is a behavioral contract: it must be reviewed before merge because it affects every gameplay system's input assumptions.

---

## Overview

The game is a single C++17 executable (`game`) that consumes only the public headers of the engine and renderer static libs. It owns no second `entt::registry`, no second ImGui context, and no second window. The entry point initializes renderer + engine, registers a frame callback and an input callback, then hands control to `renderer_run()`. Inside the frame callback the game calls `game_tick(dt)`, which drives every gameplay system.

The game does not export anything for downstream consumption — its public surface exists only so that `main.cpp`, the frame callback, and the input callback can call into the rest of the game implementation cleanly.

---

## Public Header Shape (`src/game/game.h`)

```cpp
#pragma once

// =========================================================================
// Game Lifecycle
// =========================================================================

// Initialize all game state: spawn player, enemy, asteroid field, set up
// camera and light. Must be called after engine_init() and before
// renderer_run().
void game_init();

// Advance all gameplay systems by dt seconds. Called inside the renderer
// FrameCallback between renderer_begin_frame() and renderer_end_frame().
// Internally calls engine_tick(dt) and submits draw commands.
//
// System execution order per tick:
//   1. engine_tick(dt)            -- physics, collision, entity cleanup
//   2. containment_update()       -- boundary reflection + speed cap
//   3. player_update(dt)          -- flight controls, boost, resource regen
//   4. enemy_ai_update(dt)        -- seek + shoot logic
//   5. weapon_update(dt)          -- fire processing, cooldown advancement
//   6. projectile_update(dt)      -- lifetime check, despawn expired
//   7. damage_resolve()           -- collision + projectile damage -> shield/HP
//   8. match_state_update(dt)     -- phase transitions, win/loss, restart
//   9. camera_rig_update(dt)      -- follow player with offset + lag
//  10. render_submit()            -- enqueue_draw for all visible entities
//  11. hud_render()               -- ImGui widgets + overlays
void game_tick(float dt);

// Tear down all game state. Called after renderer_run() returns.
// Destroys all game-created entities. Idempotent.
void game_shutdown();
```

---

## Calling Convention

```text
main():
  renderer_init(config)
  engine_init(engine_config)
  game_init()
  renderer_set_frame_callback(frame_cb, nullptr)
  renderer_set_input_callback(input_cb, nullptr)
  renderer_run()           // blocking
  game_shutdown()
  engine_shutdown()
  renderer_shutdown()

frame_cb(dt, user_data):
  renderer_begin_frame()
  game_tick(dt)            // engine_tick(dt) called inside
  renderer_end_frame()

input_cb(event, user_data):
  // forward sokol_app event to engine input handler
  // (engine owns input polling state; game polls through engine)
```

The game **must not** call `engine_tick(dt)` from `main.cpp` or the frame callback directly — it is called once, at step 1 of `game_tick`, so the rest of the system order can rely on engine-resolved positions, collisions, and the `DestroyPending` sweep.

---

## System Execution Order — Rationale

Each step has a hard dependency on the steps above it. Reordering breaks at least one acceptance scenario.

| Step | System | Why this position |
|------|--------|-------------------|
| 1 | `engine_tick(dt)` | Runs physics substeps and the `DestroyPending` sweep. Everything below reads post-physics positions and collision results. |
| 2 | `containment_update()` | Reflects velocity at the boundary. Must be after physics so `RigidBody.linear_velocity` is current; must be before player input so player/asteroid corrections are applied to the latest state. |
| 3 | `player_update(dt)` | Reads input, applies thrust/strafe/rotation, manages boost drain/regen. Runs after containment so the player can fly out of a reflected pose without the controller fighting the reflection. |
| 4 | `enemy_ai_update(dt)` | Computes seek vector toward player. Needs current player position. |
| 5 | `weapon_update(dt)` | Processes player fire input and AI fire decisions; spawns projectiles or performs raycasts. Runs after AI so enemy fire is decided this tick. |
| 6 | `projectile_update(dt)` | Lifetime check / despawn-expired. Runs after weapon spawn so a freshly spawned projectile is age 0, not 1 frame. |
| 7 | `damage_resolve()` | Reads engine collision events, applies weapon + collision damage, resolves shield → HP, marks deaths. Runs after weapons so this tick's hits are resolved this tick. |
| 8 | `match_state_update(dt)` | Checks enemy count for victory, player HP for death, advances restart countdown. **Enemy deaths are evaluated before player death** so simultaneous death yields a Victory transition (clarification 2026-04-23). |
| 9 | `camera_rig_update(dt)` | Camera follows player with offset + lag and pushes the active-camera tag through `engine_set_active_camera`. Runs late so the camera reads the post-update player pose. |
| 10 | `render_submit()` | Iterates `view<Transform, Mesh, EntityMaterial>` and calls `renderer_enqueue_draw` per entity. Also submits laser line visuals via `renderer_enqueue_line_quad`. |
| 11 | `hud_render()` | Builds ImGui widgets for HP/Shield/Boost bars, crosshair, weapon indicator, cooldown display, and win/death overlays. Runs last so HUD reads final per-tick state. Lives between `renderer_begin_frame` and `renderer_end_frame`; the renderer-owned `simgui` integration handles the actual draw. |

---

## Match State Machine (clarification 2026-04-23)

```
              start
                │
                ▼
            Playing ──────────────┐
              │ │                 │
              │ └── enemies==0 ──►│
              │                   ▼
              └── HP==0 ─────► Victory ◄─── enemies==0 (priority)
                                  │
                            (timer 3s OR Enter)
                                  │
                                  ▼
                              Restarting ──── (rebuild) ────► Playing
                                  ▲
                            (timer 2s OR Enter)
                                  │
                            PlayerDead ◄──────── HP==0
```

- **Playing → Victory** is checked **before** **Playing → PlayerDead** within `match_state_update`. Simultaneous death therefore yields Victory (research R2).
- **PlayerDead** auto-transitions to **Restarting** after `auto_restart_delay = 2.0s`; **Victory** after `3.0s`. Manual `Enter` skips the countdown from either terminal phase.
- **Restarting** clears all game-created entities, then re-runs the spawn sequence from `game_init()` and transitions to **Playing**. The HUD reads post-cleanup state during this single tick — overlays are not displayed in Restarting.
- **Esc** transitions out of the loop entirely via `sapp_request_quit`; not a match-state transition.

---

## Input Mapping

The game **does not** own input event handling. It polls input state via the engine API; the input callback registered with the renderer simply forwards the `sapp_event*` to the engine, which updates its polled `InputState`.

| Game action | Engine API | Trigger |
|-------------|------------|---------|
| Pitch / Yaw | `engine_mouse_delta()` | `engine_mouse_button(0)` (left) held |
| Fire | weapon system gate | `engine_mouse_button(1)` (right) held |
| Thrust forward | `engine_key_down(SAPP_KEYCODE_W)` | per-frame poll |
| Thrust reverse | `engine_key_down(SAPP_KEYCODE_S)` | per-frame poll |
| Strafe left | `engine_key_down(SAPP_KEYCODE_A)` | per-frame poll |
| Strafe right | `engine_key_down(SAPP_KEYCODE_D)` | per-frame poll |
| Boost | `engine_key_down(SAPP_KEYCODE_SPACE)` | per-frame poll |
| Switch to Plasma | `engine_key_down(SAPP_KEYCODE_Q)` | edge-triggered (track previous state) |
| Switch to Laser | `engine_key_down(SAPP_KEYCODE_E)` | edge-triggered |
| Restart | `engine_key_down(SAPP_KEYCODE_ENTER)` | edge-triggered |
| Quit | `engine_key_down(SAPP_KEYCODE_ESCAPE)` | edge-triggered → `sapp_request_quit()` |

Game-layer code that needs edge detection keeps its own `prev_*` flags; the engine exposes only the polled level state.

---

## Resource and Tick Contracts

- **One `entt::registry`**: the engine owns it. Game iterates via `engine_registry().view<...>()` or the templated component helpers (`engine_get_component<T>(e)`, etc.). Game **must not** instantiate a second registry.
- **Camera ownership**: the game creates a Camera entity, attaches the `CameraActive` tag via `engine_set_active_camera(e)`, and updates its `Transform` each tick from the camera rig. The engine computes view + projection and pushes them to `renderer_set_camera`.
- **Directional light**: created once at `game_init` as a single Light entity. The engine pushes it to `renderer_set_directional_light` from inside `engine_tick`.
- **ImGui**: renderer owns `simgui_setup`, event forwarding, `simgui_new_frame`, and `simgui_render`. The game calls only `ImGui::*` widget functions inside `hud_render()`. No second ImGui context.
- **Physics setup**: every entity tagged `Dynamic` must have `ForceAccum` attached and `RigidBody.inv_inertia_body` initialized via `make_box_inv_inertia_body` or `make_sphere_inv_inertia_body` **before** the first `engine_tick()`.
- **Entity destruction**: game uses `engine_destroy_entity(e)` (deferred); never calls `registry.destroy(e)` directly.

---

## Error Behavior

| Situation | Behavior |
|-----------|----------|
| `game_tick` called before `game_init` | Undefined behavior — caller responsibility, not guarded |
| `engine_load_gltf` returns `{0}` | Entity is spawned with an invalid mesh handle; renderer silently skips draw; logs at engine layer |
| Laser raycast misses everything | Line visual drawn to `max_distance`; no damage applied |
| Plasma projectile collides with its owner | Ignored — `ProjectileData.owner` is excluded from damage in `damage_resolve` |
| Player entity destroyed unexpectedly | Match transitions to PlayerDead on next `match_state_update` |
| `enemies_remaining == 0` at end of `game_init` | Match transitions to Victory immediately (acceptance: enemies_remaining is the source of truth) |
| `game_shutdown` called twice | Silently ignored — entity cleanup is idempotent |
| Restart triggered during Restarting | No-op — already cleaning up |
| Manual restart while auto-countdown active | Manual restart wins; countdown discarded |

---

## Mock Surface

The game **does not** ship its own mock — it is the final consumer. Instead it builds against:

- `src/renderer/mocks/renderer_mock.cpp` (toggled by `-DUSE_RENDERER_MOCKS=ON`)
- `src/engine/mocks/engine_mock.cpp` (toggled by `-DUSE_ENGINE_MOCKS=ON`)

Mock build behavior:

- Renderer mocks: window/lifecycle no-ops; `renderer_run()` returns immediately or runs an empty loop; all draw calls discarded; handle-returning functions return valid sentinels (`{1}`).
- Engine mocks: scene API returns valid sentinel entities; component helpers compile against a static mock registry; physics, raycast, and overlap return empty/null. Inertia helpers stay numerically correct (pure functions).

The game runs end-to-end (no crash) in either or both mock configurations. With both engaged the game window does not open and the demo path is not exercised — the configuration exists only for compile/wire validation while upstream milestones land.

---

## Cross-Workstream Dependency Pinning

| Game milestone | Renderer requirement | Engine requirement |
|----------------|---------------------|-------------------|
| G-M1 | R-M1 (unlit + camera) or `USE_RENDERER_MOCKS=ON` | E-M1 (scene API) or `USE_ENGINE_MOCKS=ON` |
| G-M2 | R-M1 | E-M4 (Euler physics + collision response) |
| G-M3 | R-M1 + R-M3 (alpha-blended line quads) | E-M3 (raycast) + E-M4 (rigid-body projectiles) |
| G-M4 | R-M1 + renderer-owned `sokol_imgui` | E-M1 |
| G-M5 (desirable) | — | E-M5 (steering) |
| G-M6 (desirable) | R-M5 (custom shader hook) | E-M4 |

Current upstream state at SpecKit promotion (2026-04-26): renderer FROZEN v1.1 includes lines + ImGui; engine FROZEN v1.2 includes physics + raycast. All MVP milestones (G-M1..G-M4) can run against real upstream interfaces without mocks.

---

## Freeze Procedure

1. G-M1 lands and the lifecycle calling convention is validated end-to-end against real upstream.
2. Human supervisor edits the status banner to `**Status:** FROZEN — v1.0`, sets the freeze date, commits, and pushes.
3. Any subsequent change to the lifecycle, system order, input mapping, or error behavior requires human approval + version bump.
4. The system execution order is part of the contract — changes there are reviewed even though the symbol-level header signature is unchanged.

---

## Reference Inputs

- `pre_planning_docs/Hackathon Master Blueprint.md`
- `pre_planning_docs/Game Concept and Milestones.md`
- `docs/interfaces/renderer-interface-spec.md` (FROZEN v1.1)
- `docs/interfaces/engine-interface-spec.md` (FROZEN v1.2)
- `docs/architecture/game-architecture.md`
- `docs/game-design/GAME_DESIGN.md`
- `docs/planning/speckit/game/spec.md`
- `docs/planning/speckit/game/plan.md`
- `docs/planning/speckit/game/data-model.md`
- `docs/planning/speckit/game/contracts/game-api.md`
- `.agents/skills/game-developer/SKILL.md`
