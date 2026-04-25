# Game Workstream Design

> **Status:** Populated from Game SpecKit (`docs/planning/speckit/game/`) on 2026-04-26.
> **Upstream**: Engine interface spec (`engine-interface-spec.md`, **FROZEN v1.2**). Renderer interface spec (`renderer-interface-spec.md`, **FROZEN v1.1**). Game design document (`GAME_DESIGN.md`).

---

## Purpose

This document is the workstream-level companion to `GAME_DESIGN.md` (player-facing concept) and `docs/architecture/game-architecture.md` (system-level architecture). It records the practical decisions a game developer needs day-of: what files exist, what each system reads and writes, what depends on what, what stays game-local versus what calls into the engine, and how the workstream consumes the SpecKit task plan.

---

## File Topology (`src/game/`)

Single C++17 executable, flat layout. Every `.h`/`.cpp` pair owns one concern. There is no static-lib boundary — `game` is the executable and nothing consumes it.

```
src/game/
├── main.cpp               # Entry, lifecycle wiring, callback registration
├── game.h / game.cpp      # game_init / game_tick / game_shutdown; MatchState; render_submit
├── components.h           # All game-layer ECS components + enums
├── constants.h            # All tuning values
├── spawn.h / spawn.cpp    # Entity factories per archetype
├── player.h / player.cpp  # Flight controller, shield/boost regen
├── camera_rig.h / .cpp    # Third-person follow with offset + lag
├── asteroid_field.h/.cpp  # Procedural placement + spherical containment
├── damage.h / damage.cpp  # Kinetic-energy + weapon damage pipeline
├── weapons.h / .cpp       # Laser raycast + plasma spawn + cooldowns + Q/E switch
├── projectile.h / .cpp    # Plasma lifetime + collision despawn
├── enemy_ai.h / .cpp      # Game-local seek + LOS + shoot
└── hud.h / hud.cpp        # ImGui HUD + win/death overlays
```

---

## ECS Component Layout

The engine owns the single `entt::registry`. The game attaches its own components to engine-owned entities. There is **no** game-side registry.

### Tag components (empty)

`PlayerTag`, `EnemyTag`, `AsteroidTag`, `ProjectileTag`.

### Game state components

| Component | Read by | Written by |
|-----------|---------|------------|
| `Health` | `damage_resolve`, `match_state_update`, `hud_render` | `damage_resolve`, `spawn_*` |
| `Shield` | `damage_resolve`, `player_update`, `hud_render` | `damage_resolve`, `player_update` (regen), `spawn_*` |
| `Boost` | `player_update`, `hud_render` | `player_update`, `spawn_player` |
| `WeaponState` | `weapon_update`, `enemy_ai_update`, `hud_render` | `weapon_update`, `spawn_player`/`spawn_enemy` |
| `ProjectileData` | `projectile_update`, `damage_resolve` | `weapon_update` (spawn), `spawn_projectile` |
| `EnemyAI` | `enemy_ai_update` | `spawn_enemy` |
| `AsteroidData` | `containment_update`, `damage_resolve` | `spawn_asteroid` |

### Singletons (not ECS)

- `MatchState` — owned in a file-static in `game.cpp`; mutated by `match_state_update` and read by `hud_render`.
- `FieldConfig` — read-only after `game_init`.

Full schema in `docs/planning/speckit/game/data-model.md`.

---

## System Boundaries — what the game owns vs the engine

| Concern | Owner | Notes |
|---------|-------|-------|
| Window, GPU context, sokol_app loop | Renderer | Game registers a `FrameCallback`; never touches sokol directly |
| ImGui setup / event forward / new_frame / render | Renderer | Game emits widgets only; no second context |
| Entity lifecycle (`create_entity`, `destroy_entity`) | Engine | Game uses `engine_*` wrappers; deferred destruction via `DestroyPending` |
| Component attach / get / view / iterate | Engine | Game uses `engine_add/get/has_component<T>()` and `engine_registry().view<...>()` |
| AABB collision detection | Engine | Game gets contact events from physics, not from raw geometry |
| Rigid-body integration (linear + angular Euler) | Engine | Game applies forces/impulses via `engine_apply_*` |
| Raycast / overlap_aabb | Engine | Game uses for laser hits, AI line-of-sight |
| Asset import (glTF/OBJ → mesh handle) | Engine | Game calls `engine_load_gltf` / `engine_load_obj` / `engine_spawn_from_asset` |
| Camera view / projection composition | Engine | Game owns the rig pose; engine computes matrices via `engine_set_active_camera` |
| Player flight controller, boost, regen | **Game** | Reads polled input through engine API |
| Spherical containment + speed cap | **Game** | Engine has only AABB colliders; sphere stays game-side |
| Weapons (laser raycast, plasma projectiles, cooldowns) | **Game** | Engine provides primitives (raycast, RigidBody) |
| Damage pipeline (energy → shield → HP) | **Game** | Reads engine collision results, applies game rules |
| Enemy AI (seek + LOS + shoot) | **Game** | MVP is game-local; E-M5 swap is Desirable (G-M5) |
| HUD widgets, overlays, restart flow | **Game** | Renderer renders; game emits |
| Match state machine | **Game** | Four-state machine in `game.cpp` |

---

## Tick Order (eleven steps)

The order is contractual (see `docs/interfaces/game-interface-spec.md`). Every gameplay system must be safe to read engine state set by the previous step.

1. `engine_tick(dt)` — physics, collision, DestroyPending sweep
2. `containment_update()` — boundary reflection + asteroid speed cap
3. `player_update(dt)` — input → thrust/strafe/rotation, boost drain, shield regen
4. `enemy_ai_update(dt)` — seek + LOS check
5. `weapon_update(dt)` — fire processing, cooldown advance, raycast / projectile spawn
6. `projectile_update(dt)` — lifetime expiry / despawn
7. `damage_resolve()` — collision energy + weapon hits → shield → HP, mark deaths
8. `match_state_update(dt)` — Victory / PlayerDead / Restarting transitions (Victory checked first)
9. `camera_rig_update(dt)` — follow with offset + lag
10. `render_submit()` — `enqueue_draw` per visible entity; `enqueue_line_quad` for laser visuals
11. `hud_render()` — ImGui widgets + overlays

---

## Mocking and Build Modes

| Mode | CMake flag | Purpose |
|------|-----------|---------|
| Real upstream | (none) | Default at promotion time — both renderer and engine are FROZEN |
| Renderer mocks | `-DUSE_RENDERER_MOCKS=ON` | Validate game compiles without GPU context |
| Engine mocks | `-DUSE_ENGINE_MOCKS=ON` | Validate game compiles without physics/ECS |
| Both mocks | both | Headless wire validation only — no demoable scene |

The game itself ships no mock. Demo-day fallback (per `pre_planning_docs/Hackathon Master Blueprint.md` §13.2): if either upstream regresses at T-30, re-enable its mock to keep `game` launchable.

---

## Asset Requirements

- Ship meshes: `assets/spaceship1.glb` … `spaceship3.glb` (existing) — load via `engine_load_gltf`.
- Asteroid meshes: `assets/Asteroid_*.glb` (existing) — load via `engine_load_gltf`.
- Skybox cubemap: `assets/skybox/` (existing) — loaded via renderer skybox API at `game_init` time.
- Plasma projectile: procedural sphere via `renderer_make_sphere_mesh` (no asset).
- Crosshair / HUD: ImGui-rendered (no asset).

If asset loading fails (engine returns `{0}`), the spawn factory falls back to a procedural primitive so the game remains demoable.

---

## Risk Register (workstream-local)

| Risk | Severity | Mitigation |
|------|----------|------------|
| Third-person camera rig overruns G-M1 | MED | First-person fallback (offset = 0); document switch in TASKS.md |
| Containment reflection + speed cap leaves asteroids stuck on the boundary | MED | Validate during G-M2 acceptance: visually walk the boundary with the camera and confirm reflection looks elastic, not sticky |
| Plasma projectile entity churn under sustained fire | LOW | Cap concurrent active projectiles; rely on engine deferred-destroy sweep |
| Simultaneous death misclassified as defeat | LOW | Match state evaluates Victory before PlayerDead unconditionally; G-029 acceptance check |
| Restart leaves stale ECS entries | MED | Restarting phase iterates and `engine_destroy_entity` everything game-tagged; verify the next Playing phase sees `enemies_remaining == 1` |
| Laser line visual depth-fighting near asteroid surfaces | LOW | Renderer uses transparent pass for line quads; depth test on, depth write off — already part of R-M3 |

---

## Cross-References

- Concept and player rules: `docs/game-design/GAME_DESIGN.md`
- System architecture: `docs/architecture/game-architecture.md`
- Procedural API contract + tick order: `docs/interfaces/game-interface-spec.md`
- Live task board: `_coordination/game/TASKS.md`
- SpecKit artifacts (planning source-of-truth): `docs/planning/speckit/game/`
- Quickstart / runbook: `docs/planning/speckit/game/quickstart.md`
