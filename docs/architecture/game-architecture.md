# Game Architecture

> **Status:** Populated from Game SpecKit (`docs/planning/speckit/game/`) on 2026-04-26.
> **Upstream**: Engine architecture (`engine-architecture.md`, FROZEN v1.2). Renderer architecture (`renderer-architecture.md`, FROZEN v1.1).

---

## Overview

The game is a single C++17 executable (`game`) that hosts a Freelancer-style third-person space shooter inside a contained asteroid field. It consumes only the public headers `engine.h` and `renderer.h`. The game owns no second `entt::registry`, no second window, no second ImGui context, and no second sokol context — every shared resource is borrowed from upstream.

The game owns: player flight controller, third-person camera rig, asteroid field placement + spherical containment, weapon systems (laser raycast + plasma projectile), HP/Shield/Boost resources + regeneration, game-local seek+shoot enemy AI, Dear ImGui HUD widgets, and the 4-state match flow (Playing → PlayerDead/Victory → Restarting → Playing).

---

## Ownership and Boundaries

- **Game owns**: player flight controller, camera rig, asteroid field + containment math, weapon systems, HP/Shield/Boost components and pipelines, enemy AI, ImGui HUD widgets, match state machine, restart flow, all game-layer ECS components (tags + state structs).
- **Game does not own**: window, GPU context, shader pipelines, sokol_app loop (renderer); ECS registry, physics integrator, AABB collision detection, raycast/overlap, asset import, input event pump (engine).
- **Game consumes**: `engine.h` (FROZEN v1.2) and `renderer.h` (FROZEN v1.1) only. Internal headers of either upstream are never included.

---

## Integration with Engine and Renderer

```
sokol_app main loop (renderer)
  └─► renderer internal frame_callback()
         ├─ simgui_new_frame()
         ├─ FrameCallback(dt, user_data)         <-- registered by game/main.cpp
         │     ├─ renderer_begin_frame()
         │     ├─ game_tick(dt)
         │     │     1. engine_tick(dt)          <-- physics, collision, DestroyPending sweep
         │     │     2. containment_update()
         │     │     3. player_update(dt)
         │     │     4. enemy_ai_update(dt)
         │     │     5. weapon_update(dt)
         │     │     6. projectile_update(dt)
         │     │     7. damage_resolve()
         │     │     8. match_state_update(dt)
         │     │     9. camera_rig_update(dt)
         │     │    10. render_submit()           <-- enqueue_draw for every visible entity
         │     │    11. hud_render()              <-- ImGui widgets and overlays
         │     └─ renderer_end_frame()
         └─ renderer post-frame: skybox, opaque, transparent, simgui_render
```

### Key dependencies

| Library / target | Role | Version |
|------------------|------|---------|
| `engine` (static lib) | ECS, physics, asset import, input, raycast | FROZEN v1.2 |
| `renderer` (static lib) | Window, GPU, pipelines, ImGui, lines | FROZEN v1.1 |
| GLM | vec3 / mat4 / quat math (game-side helpers) | 1.0.1 |
| Dear ImGui | HUD widgets only — game emits, renderer renders | v1.92.7-docking |
| entt | Component view iteration via `engine_registry()` | v3.13.2 |

---

## Module Map

> Source layout: `src/game/` flat (no subdirectories beyond the implicit `app` pattern used by other workstreams — `game` is itself the executable).

| File | Responsibility | Milestone |
|------|----------------|-----------|
| `main.cpp` | Entry point: `renderer_init` → `engine_init` → `game_init` → register frame + input callbacks → `renderer_run` → shutdown | SETUP |
| `game.h` / `game.cpp` | `game_init`, `game_tick`, `game_shutdown`; orchestrates the 11-step tick; owns `MatchState` singleton + match transitions; `render_submit` | SETUP / FOUNDATION / G-M1+ |
| `components.h` | Game-layer ECS components: `PlayerTag`, `EnemyTag`, `AsteroidTag`, `ProjectileTag`, `Health`, `Shield`, `Boost`, `WeaponState`, `ProjectileData`, `EnemyAI`, `AsteroidData`; enums `WeaponType`, `SizeTier`, `MatchPhase` | FOUNDATION |
| `constants.h` | Tuning constants (speeds, damage, cooldowns, field radius, camera offsets, regen rates, asteroid mass/scale tiers, auto-restart delays) | FOUNDATION |
| `spawn.h` / `spawn.cpp` | Entity factories: `spawn_player`, `spawn_enemy`, `spawn_asteroid`, `spawn_projectile`; composes engine + game components per archetype | FOUNDATION |
| `player.h` / `player.cpp` | Flight controller (W/S/A/D + mouse look with LMB hold), boost toggle, shield + boost regen logic | G-M1 / G-M2 |
| `camera_rig.h` / `camera_rig.cpp` | Third-person follow camera with offset + lag (LERP); first-person fallback (offset = 0) | G-M1 |
| `asteroid_field.h` / `asteroid_field.cpp` | Procedural placement of 200 asteroids in 3 size tiers; spherical containment reflection + post-reflection speed cap | G-M1 / G-M2 |
| `damage.h` / `damage.cpp` | Damage pipeline: collision kinetic-energy damage, weapon hit damage, shield → HP cascade, last-damage timestamping for shield-regen gate, death detection | G-M2 / G-M3 |
| `weapons.h` / `weapons.cpp` | Weapon definitions, cooldown tracking, Q/E switch (edge-triggered), laser raycast + line visual, plasma projectile spawn | G-M3 |
| `projectile.h` / `projectile.cpp` | Plasma projectile lifetime + first-collision despawn | G-M3 |
| `enemy_ai.h` / `enemy_ai.cpp` | Game-local seek vector + line-of-sight raycast + cooldown-gated plasma fire | G-M3 |
| `hud.h` / `hud.cpp` | ImGui HUD: HP/Shield/Boost bars, crosshair, weapon indicator, cooldown display, win/death overlays | G-M4 |

**Structure decision**: single executable, flat layout. The game has no static-lib boundary because nothing consumes it. There is no `game/app/` subdir; `main.cpp` lives directly in `src/game/`.

---

## Data Model — Game-Layer ECS

The game attaches its own components to entities owned by the engine's `entt::registry`. Engine-owned components (`Transform`, `Mesh`, `EntityMaterial`, `RigidBody`, `Collider`, `Camera`, `Light`, `ForceAccum`, `Static`, `Dynamic`, `Interactable`, `CameraActive`, `DestroyPending`) coexist on the same entities. There is **no** game-side registry.

### Tag components (empty structs)

| Tag | Attached to | Purpose |
|-----|-------------|---------|
| `PlayerTag` | Player ship | Single-instance query target |
| `EnemyTag` | Enemy ships | Win-condition tally, AI targeting filter |
| `AsteroidTag` | Asteroids | Containment + collision-damage classification |
| `ProjectileTag` | Plasma projectiles | Lifetime + first-collision despawn |

### State components (summarized — full schema in `docs/planning/speckit/game/data-model.md`)

- `Health { current, max }` — clamped [0, max], shield-cascaded
- `Shield { current, max, regen_rate, regen_delay, last_damage_time }` — regenerates only after `regen_delay` seconds of no damage
- `Boost { current, max, drain_rate, regen_rate, active }` — player-only; drains while active, regenerates while inactive
- `WeaponState { active_weapon, laser_*, plasma_*, plasma_speed, plasma_lifetime }` — independent per-weapon cooldowns
- `ProjectileData { owner, damage, spawn_time, lifetime }` — owner-excluded from damage; despawns on first non-owner collision or lifetime expiry
- `EnemyAI { pursuit_speed, fire_range, fire_cooldown, last_fire_time }` — game-local; no engine E-M5 dependency
- `AsteroidData { size_tier }` — enum `{Small, Medium, Large}` driving mesh scale + initial mass/velocity tier

### Singleton game state (not ECS)

- `MatchState { phase, phase_enter_time, auto_restart_delay, enemies_remaining }` — drives the 4-state machine; `phase` ∈ `{Playing, PlayerDead, Victory, Restarting}`
- `FieldConfig { radius = 1000.0f, asteroid_count = 200, max_asteroid_speed = 50.0f }` — read-only after init; drives placement and post-reflection clamping

### Entity archetypes

| Archetype | Engine components | Game components |
|-----------|------------------|-----------------|
| Player ship | `Transform`, `Mesh`, `EntityMaterial`, `RigidBody`, `ForceAccum`, `Collider`, `Dynamic` | `PlayerTag`, `Health`, `Shield`, `Boost`, `WeaponState` |
| Enemy ship | `Transform`, `Mesh`, `EntityMaterial`, `RigidBody`, `ForceAccum`, `Collider`, `Dynamic` | `EnemyTag`, `Health`, `Shield`, `WeaponState`, `EnemyAI` |
| Asteroid | `Transform`, `Mesh`, `EntityMaterial`, `RigidBody`, `ForceAccum`, `Collider`, `Dynamic` | `AsteroidTag`, `AsteroidData` |
| Plasma projectile | `Transform`, `Mesh`, `EntityMaterial`, `RigidBody`, `ForceAccum`, `Collider`, `Dynamic`, `Interactable` | `ProjectileTag`, `ProjectileData` |
| Camera | `Transform`, `Camera`, `CameraActive` | (none — managed by camera rig) |
| Directional light | `Light` | (none — set once at init) |

`Dynamic` entities must have `ForceAccum` and `RigidBody.inv_inertia_body` initialized at spawn — `spawn_*` factories handle this via `make_box_inv_inertia_body` / `make_sphere_inv_inertia_body`.

---

## System Execution Order — `game_tick(dt)`

The eleven-step order is part of the public game contract (see `docs/interfaces/game-interface-spec.md` for the rationale per step). Summary:

1. `engine_tick(dt)` — physics + collision + DestroyPending sweep
2. `containment_update()` — boundary reflection + asteroid speed cap
3. `player_update(dt)` — input → thrust/strafe/rotation, boost drain, shield regen
4. `enemy_ai_update(dt)` — seek + line-of-sight check
5. `weapon_update(dt)` — fire processing, cooldown advance, laser raycast / plasma spawn
6. `projectile_update(dt)` — lifetime expiry / despawn
7. `damage_resolve()` — collision energy + weapon hits → shield → HP, mark deaths
8. `match_state_update(dt)` — Victory / PlayerDead / Restarting transitions (Victory checked first)
9. `camera_rig_update(dt)` — follow with offset + lag, push to `engine_set_active_camera`
10. `render_submit()` — `view<Transform, Mesh, EntityMaterial>` → `renderer_enqueue_draw`; laser visuals via `renderer_enqueue_line_quad`
11. `hud_render()` — ImGui widgets + overlays

---

## Key Design Decisions (from SpecKit research)

| ID | Decision | Rationale |
|----|----------|-----------|
| R1 | 4-state match machine (Playing → PlayerDead/Victory → Restarting → Playing) | Separating overlay-display from entity-cleanup avoids reading destroyed components during the terminal frame. |
| R2 | Simultaneous death → Victory | Win condition is "enemies == 0"; objective met → player wins. Enemy deaths evaluated before player death within `match_state_update`. |
| R3 | No collision damage threshold | Damage proportional to relative kinetic energy on every contact; no magic-number cutoff to tune. Shield regen masks negligible damage. |
| R4 | Third-person rig with first-person fallback | Better spatial awareness for asteroid avoidance + Freelancer feel. Fallback is one-line offset change if rig overruns budget. |
| R5 | Game-local enemy AI for MVP | Seek + LOS + cooldown is ~20 lines; eliminates dependency on engine E-M5 (Desirable). |
| R6 | Laser = `engine_raycast`, plasma = `RigidBody` projectile entity | Matches engine API capabilities directly; preserves distinct weapon feel. |
| R7 | Game emits ImGui widgets only — renderer owns lifecycle | Renderer already owns `simgui_setup`/`simgui_new_frame`/`simgui_render`; second context would force game to depend on sokol internals. |
| R8 | Spherical containment as game-layer constraint | Engine has only AABB colliders; spherical boundary stays game-side. Speed cap on each reflection prevents floating-point energy growth. |

---

## Performance Budget

- ≥30 FPS at 200 asteroids + 1 enemy + active combat on RTX 3090.
- Plasma projectile spawn/despawn must not cause visible frame hitches at 0.1s cadence (≤10 projectiles/sec).
- Brute-force O(N²) collision is the engine's contract; game must keep total entity count under ~250 (200 asteroids + 1 player + 1 enemy + ≤50 active projectiles).
- HUD pass cost is ImGui-bounded; the seven HUD elements are stable per frame.

---

## Mock Strategy

The game is the final consumer; it ships no mock of its own. While upstream milestones land it builds with `-DUSE_RENDERER_MOCKS=ON` and/or `-DUSE_ENGINE_MOCKS=ON` (see `docs/interfaces/game-interface-spec.md` "Mock Surface").

At Game SpecKit promotion (2026-04-26) the renderer is FROZEN v1.1 and the engine is FROZEN v1.2 — both real implementations are available, so the game runs without mocks for all MVP milestones.

---

## Milestones

### MVP

| ID | Name | Key deliverables | Expected outcome |
|----|------|-----------------|------------------|
| G-M1 | Flight + Scene + Camera | Player flight controller, third-person camera rig, 200 procedurally-placed asteroids, directional light, real engine + renderer wired | Player can fly through a static asteroid field with a camera that follows; nothing collides yet |
| G-M2 | Physics, Containment, Asteroid Dynamics | Rigid bodies on asteroids + ship, initial asteroid linear/angular velocities, spherical containment reflection, post-reflection speed cap, collision-energy damage, shield/boost regen | Field feels alive; ship bounces off asteroids and the boundary; boost drains and regens; player takes damage on hard contacts |
| G-M3 | Weapons + Enemies + HP/Shield | Laser (raycast + line-quad visual + 5s cooldown), plasma (RigidBody projectile + 0.1s cadence), Q/E weapon switch, 1 enemy with seek + LOS + plasma fire, minimal explosion VFX on death | Full combat loop with one enemy; weapons feel distinct; enemy can damage and be destroyed |
| G-M4 | HUD + Game Flow + Restart | ImGui HUD (HP/Shield/Boost bars, crosshair, weapon indicator, cooldown), 4-state match machine, win/death overlays, auto + manual restart, Esc quit | **Game MVP complete.** Full demo loop: start → fly → fight → win or die → overlay → restart |

### Desirable

| ID | Name | Key deliverables |
|----|------|------------------|
| G-M5 | AI Upgrade & Scaling | Swap game-local AI for engine E-M5 steering; scale enemy count to 3–8 |
| G-M6 | Visual Polish | Explosion VFX via renderer R-M5 custom shader hook; laser hit-flash on asteroid surface |
| G-M7 | Feel Tuning | Final pass on accelerations, damping, FOV, camera lag, weapon cadence, damage values, asteroid density |

### Stretch

Screen-space damage numbers, power-ups, rocket launcher, asteroid-asteroid collisions. All explicitly cut unless MVP lands with significant headroom.

### Milestone execution order

```
SETUP -> FOUNDATION -> G-M1 -> G-M2 -> G-M3 -> G-M4 -> POLISH
                                                  └─► (Desirable: G-M5..G-M7 if time)
```

User-story phases (US1..US3) from `docs/planning/speckit/game/tasks.md` map onto milestones as: US1 = G-M1 + G-M2; US2 = G-M3; US3 = G-M4.

---

## Cross-Workstream Dependencies

| Game milestone | Renderer requirement | Engine requirement |
|----------------|---------------------|-------------------|
| G-M1 | R-M1 (or `USE_RENDERER_MOCKS=ON`) | E-M1 (or `USE_ENGINE_MOCKS=ON`) |
| G-M2 | R-M1 | E-M4 |
| G-M3 | R-M1 + R-M3 | E-M3 + E-M4 |
| G-M4 | R-M1 + renderer-owned `sokol_imgui` | E-M1 |
| G-M5 (Desirable) | — | E-M5 |
| G-M6 (Desirable) | R-M5 | E-M4 |

At promotion time both upstream interfaces are FROZEN through the relevant milestones; no mock fallback is required for MVP work.

---

## Risks and Mitigations

| Risk | Mitigation |
|------|------------|
| Third-person camera rig overruns G-M1 budget | First-person fallback is a one-line offset change in `camera_rig.cpp`; document decision in TASKS.md and notify renderer/engine workstreams |
| Floating-point energy growth from repeated containment reflections | Hard speed cap (`FieldConfig.max_asteroid_speed`) applied on every reflection event |
| Brute-force O(N²) collision at 200+ entities under-performs | Performance budget assumes ≤250 dynamic colliders; enforce projectile cap (≤50 active) and clamp asteroid count if frame time slips |
| Plasma projectile entity churn causes HUD/draw hitches | Reuse projectile entities via `engine_destroy_entity` (deferred sweep) rather than create-destroy churn within a tick; cap concurrent projectiles |
| Simultaneous death edge case yields a confusing terminal state | Match state machine evaluates Victory before PlayerDead unconditionally (R2); spec test G-029 covers this scenario |
| Restart leaves stale entities | Restarting phase iterates and `engine_destroy_entity` everything except camera + light entities, then re-runs the spawn sequence; verified during G-M4 acceptance |
