# Tasks: Space Shooter MVP

**Input**: Design documents from `/specs/003-space-shooter-mvp/`
**Prerequisites**: plan.md (required), spec.md (required), research.md, data-model.md, contracts/game-api.md, quickstart.md

**Tests**: No automated test tasks generated — game layer uses human behavioral checks per constitution. No `game_tests` Catch2 target.

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story. Phases map to game milestones: Setup → Foundational → US1 (G-M1/G-M2) → US2 (G-M3) → US3 (G-M4) → Polish.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies on incomplete tasks)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Path Conventions

All game source lives under `src/game/`. The game consumes `engine.h` and `renderer.h` public APIs only.

---

## Phase 1: Setup (Project Initialization)

**Purpose**: Create the game executable skeleton and entry point

- [ ] T001 Create game entry point with renderer/engine init, frame callback, and input callback in src/game/main.cpp
- [ ] T002 Create game lifecycle skeleton (game_init, game_tick, game_shutdown) in src/game/game.h and src/game/game.cpp

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Define all shared data types, tuning values, and entity factories that every user story depends on

**CRITICAL**: No user story work can begin until this phase is complete

- [ ] T003 [P] Define all game-layer ECS components (PlayerTag, EnemyTag, AsteroidTag, ProjectileTag, Health, Shield, Boost, WeaponState, ProjectileData, EnemyAI, AsteroidData) and enums (WeaponType, SizeTier, MatchPhase) in src/game/components.h
- [ ] T004 [P] Define all tuning constants (player speeds, damage values, cooldowns, field radius, camera offsets, regen rates, asteroid size/mass/velocity ranges) in src/game/constants.h
- [ ] T005 Implement entity spawn factories (spawn_player, spawn_enemy, spawn_asteroid, spawn_projectile) that compose engine + game components per data-model archetypes in src/game/spawn.h and src/game/spawn.cpp
- [ ] T006 Implement render_submit() to iterate all entities with Transform + Mesh + EntityMaterial and call renderer_enqueue_draw for each, plus renderer_set_directional_light in src/game/game.cpp

**Checkpoint**: Foundation ready — game compiles and runs (empty scene, clear color only). User story implementation can now begin.

---

## Phase 3: User Story 1 — Fly and Survive in the Asteroid Field (Priority: P1) — G-M1 + G-M2

**Goal**: Player can fly a ship through 200 drifting asteroids inside a bounded field, collide with them, take kinetic energy damage absorbed by shield then HP, use boost, and see the field feel alive.

**Independent Test**: Start match, fly through field, collide with asteroids and boundary, use boost until drained, confirm shield absorbs damage and regens after delay, confirm death on HP zero.

### Implementation for User Story 1

- [ ] T007 [P] [US1] Implement player flight controller (W/S thrust, A/D strafe, mouse pitch/yaw with left-hold, boost toggle on Space) reading engine input API in src/game/player.h and src/game/player.cpp
- [ ] T008 [P] [US1] Implement third-person camera rig (configurable offset behind/above player, smooth lag follow via lerp, set active camera each frame) in src/game/camera_rig.h and src/game/camera_rig.cpp
- [ ] T009 [P] [US1] Implement asteroid field procedural placement (200 asteroids, random positions inside containment sphere, 3 size tiers with mass/scale/velocity ranges, random initial linear + angular velocity) in src/game/asteroid_field.h and src/game/asteroid_field.cpp
- [ ] T010 [US1] Wire game_init() to call spawn_player, spawn 200 asteroids via asteroid_field_init, create camera entity, set directional light in src/game/game.cpp
- [ ] T011 [US1] Implement containment update (check entity distance from origin > field radius, reflect velocity along inward normal, clamp asteroid speed to max_asteroid_speed, damp player velocity slightly) in src/game/asteroid_field.cpp
- [ ] T012 [US1] Implement collision damage pipeline (compute relative kinetic energy from RigidBody velocities and masses, apply damage to shield first then HP, record last_damage_time for shield regen gating, detect death at HP zero) in src/game/damage.h and src/game/damage.cpp
- [ ] T013 [US1] Implement shield regen (delay-gated passive regen after no damage) and boost drain/regen (drain while active, regen while inactive) in src/game/player.cpp
- [ ] T014 [US1] Wire player_update, camera_rig_update, containment_update, damage_resolve into game_tick() system execution order in src/game/game.cpp

**Checkpoint**: Player flies through drifting asteroid field. Asteroids tumble and bounce off containment boundary. Ship collides with asteroids and takes shield→HP damage. Boost drains and regens. Death triggers on HP zero. Corresponds to G-M1 + G-M2 milestone outcomes.

---

## Phase 4: User Story 2 — Engage Enemies with Two Distinct Weapons (Priority: P2) — G-M3

**Goal**: Player can switch between laser (instant raycast, 10 dmg, 5s cooldown) and plasma (rapid projectiles, 0.5 dmg, 0.1s cooldown), fight 1 enemy that seeks and shoots back, destroy enemy to see death VFX.

**Independent Test**: Spawn enemy, switch weapons, fire each weapon, confirm cooldown enforcement, confirm damage values, confirm enemy death and despawn with explosion.

### Implementation for User Story 2

- [ ] T015 [P] [US2] Implement weapon definitions (laser/plasma stats from constants), cooldown tracking, weapon switch (Q/E edge-triggered) in src/game/weapons.h and src/game/weapons.cpp
- [ ] T016 [P] [US2] Implement plasma projectile lifetime tracking and collision despawn (check engine_now() - spawn_time > lifetime, mark DestroyPending on collision) in src/game/projectile.h and src/game/projectile.cpp
- [ ] T017 [P] [US2] Implement enemy AI (compute seek vector toward player, apply as RigidBody velocity, check fire range + LOS via engine_raycast, fire plasma on cooldown) in src/game/enemy_ai.h and src/game/enemy_ai.cpp
- [ ] T018 [US2] Implement laser fire logic (engine_raycast from ship forward, renderer_enqueue_line_quad from muzzle to hit/max-range, apply laser_damage to hit entity, apply impulse to asteroid hits) in src/game/weapons.cpp
- [ ] T019 [US2] Implement plasma fire logic (spawn projectile entity via spawn_projectile with forward velocity from WeaponState.plasma_speed, auto-fire while right mouse held and cooldown allows) in src/game/weapons.cpp
- [ ] T020 [US2] Extend damage pipeline for weapon hits (detect projectile-entity collisions, apply ProjectileData.damage to target shield→HP, apply laser raycast damage, exclude projectile self-owner from damage) in src/game/damage.cpp
- [ ] T021 [US2] Add enemy spawn to game_init() via spawn_enemy and implement death handling (detect Health.current <= 0 on EnemyTag entities, despawn with minimal explosion VFX via short-lived scaled sphere, decrement enemies_remaining) in src/game/spawn.cpp and src/game/game.cpp
- [ ] T022 [US2] Wire weapon_update, projectile_update, enemy_ai_update into game_tick() system execution order in src/game/game.cpp

**Checkpoint**: Full combat loop with 1 enemy. Laser fires with 5s cooldown and line visual. Plasma fires rapidly with projectile entities. Enemy seeks player and shoots back. Weapons deal correct damage. Enemy dies with explosion VFX. Corresponds to G-M3 milestone outcome.

---

## Phase 5: User Story 3 — Complete Match Flow with HUD and Restart (Priority: P3) — G-M4

**Goal**: Player can see HP/shield/boost/weapon status on HUD, win by destroying all enemies, die when HP reaches zero, see win/death overlays, and restart or quit.

**Independent Test**: Play full match to win and death outcomes, verify HUD updates in real time, observe overlays, confirm auto-restart timing, test manual restart and Esc quit.

### Implementation for User Story 3

- [ ] T023 [P] [US3] Implement ImGui HUD with HP bar (red), shield bar (blue), boost bar (yellow), centered crosshair, active weapon text, and cooldown timer display in src/game/hud.h and src/game/hud.cpp
- [ ] T024 [P] [US3] Implement 4-state match state machine (Playing → PlayerDead/Victory → Restarting → Playing) with phase transition logic, auto-restart countdown timers (2s death, 3s win), and enemy count tracking in src/game/game.cpp
- [ ] T025 [US3] Implement win overlay ("YOU WIN" + Enter prompt) and death overlay ("YOU DIED" + Enter prompt) as ImGui overlays driven by MatchState.phase in src/game/hud.cpp
- [ ] T026 [US3] Implement restart flow (Restarting phase destroys all game entities, rebuilds scene via game_init sequence, transitions to Playing) and quit (Esc edge-triggered calls sapp_request_quit) in src/game/game.cpp
- [ ] T027 [US3] Wire hud_render() and match_state_update() into game_tick() system execution order, ensure HUD reads live component data each frame in src/game/game.cpp

**Checkpoint**: **Game MVP complete.** Full demo loop: start → fly → fight → win or die → overlay → restart. HUD shows all combat state. Esc quits. Corresponds to G-M4 milestone outcome.

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Tuning, validation, and cleanup that affect multiple user stories

- [ ] T028 [P] Tune gameplay constants (player acceleration, turn rate, boost multiplier, damage scaling, weapon cadence, enemy speed, field radius, camera lag factor) in src/game/constants.h
- [ ] T029 [P] Validate full gameplay loop against all acceptance scenarios in spec.md and quickstart.md
- [ ] T030 Run end-to-end quickstart.md validation (build, launch, play through all controls, confirm all HUD elements, complete win + death + restart cycle)

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — can start immediately
- **Foundational (Phase 2)**: Depends on Setup (T001, T002) — BLOCKS all user stories
- **US1 (Phase 3)**: Depends on Foundational (T003–T006) completion
- **US2 (Phase 4)**: Depends on US1 (Phase 3) completion — builds on damage pipeline and game_tick wiring
- **US3 (Phase 5)**: Depends on US2 (Phase 4) completion — needs combat loop for meaningful HUD/match flow
- **Polish (Phase 6)**: Depends on US3 (Phase 5) completion

### User Story Dependencies

- **US1 (P1)**: Depends on Foundational only → first story to implement
- **US2 (P2)**: Depends on US1 → extends damage.cpp, adds systems to game_tick()
- **US3 (P3)**: Depends on US2 → match flow needs combat outcomes; HUD needs weapon/health data

> **Note**: US2 and US3 depend sequentially on prior stories because they extend shared files (game.cpp, damage.cpp). This matches the game milestone order (G-M1→G-M2→G-M3→G-M4).

### Within Each User Story

- Components and factories before systems (Foundational phase covers this)
- Independent systems (player, camera, asteroid_field) can be built in parallel [P]
- Integration wiring (into game_tick) comes last in each phase
- Each phase ends with a checkpoint matching a game milestone

### Parallel Opportunities per Phase

**Phase 2 (Foundational)**:
```
T003 (components.h) ─┐
                      ├──→ T005 (spawn.h/cpp) ──→ T006 (render_submit)
T004 (constants.h) ──┘
```

**Phase 3 (US1)**:
```
T007 (player.h/cpp) ────┐
T008 (camera_rig.h/cpp) ─┼──→ T010 (wire game_init) ──→ T011 (containment) ──→ T012 (damage) ──→ T013 (regen) ──→ T014 (wire game_tick)
T009 (asteroid_field.h/cpp) ┘
```

**Phase 4 (US2)**:
```
T015 (weapons.h/cpp) ───┐
T016 (projectile.h/cpp) ─┼──→ T018 (laser fire) ──→ T019 (plasma fire) ──→ T020 (weapon damage) ──→ T021 (enemy death) ──→ T022 (wire game_tick)
T017 (enemy_ai.h/cpp) ──┘
```

**Phase 5 (US3)**:
```
T023 (hud.h/cpp) ───┐
                     ├──→ T025 (overlays) ──→ T027 (wire game_tick)
T024 (match state) ─┼──→ T026 (restart/quit) ──→ T027
                     │
```

---

## Implementation Strategy

### MVP Scope

User Story 1 (P1) alone delivers a flyable, collidable, demoable scene — this is the minimum viable increment. Each subsequent story adds a complete layer:

- **After US1**: Fly through alive asteroid field (G-M1 + G-M2)
- **After US2**: Full combat with 1 enemy (G-M3)
- **After US3**: **Complete MVP** with HUD and game flow (G-M4)

### Incremental Delivery

Each phase ends at a game milestone checkpoint. If time runs out at any checkpoint, the game is in a demoable state. The game workstream never has a half-built intermediate.

### Build Command

```bash
cmake --build build --target game
```
