# TASKS.md — Game Workstream

> **Live operational task board.** Authoritative on `feature/game` branch.
> Translated 2026-04-26 from `docs/planning/speckit/game/tasks.md` (SpecKit cycle `specs/003-space-shooter-mvp/`). Source spec: `docs/planning/speckit/game/spec.md`.
> **Before claiming:** read the task row, then the spec section it references and the relevant skill / aspect file.
> **To claim:** human supervisor sets `Owner = <agent>@<machine>`, `Status = CLAIMED`, commits + pushes, then triggers the agent.
> Related docs: `docs/interfaces/game-interface-spec.md` (DRAFT v0.1 — tick order and lifecycle contract), `docs/architecture/game-architecture.md`, `docs/game-design/GAME_DESIGN.md`.

> **Upstream state at promotion:** Renderer FROZEN v1.1 (R-M1..R-M3 delivered). Engine FROZEN v1.2 (E-M1..E-M4 delivered). Game can run end-to-end against real upstream for the entire MVP. `USE_*_MOCKS=ON` remain available as demo-day fallback.

---

## SpecKit → Operational ID Mapping

SpecKit `T###` → operational `G-###` is 1:1 with one split: SpecKit `T009` (asteroid field placement + initial velocities) is single-task; we keep it as `G-009` and place it in **G-M1** because the file (`asteroid_field.cpp`) is created here. The dynamics-on-the-asteroids behavior (initial velocities + tumble) becomes visible only after E-M4 physics ticks; see `G-011` (containment) for the G-M2 sync point.

---

## Milestone: SETUP — Entry Point and Lifecycle Skeleton

**Expected outcome**: `cmake --build build --target game` succeeds. `./build/game` opens a window, clears to color, exits cleanly on Esc. No gameplay yet.

| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
|---|---|---|---|---|---|---|---|---|---|
| G-001 | Create game entry point in `src/game/main.cpp`: `renderer_init(config)` → `engine_init(engine_config)` → `game_init()` → `renderer_set_frame_callback(frame_cb, nullptr)` → `renderer_set_input_callback(input_cb, nullptr)` → `renderer_run()` → `game_shutdown()` → `engine_shutdown()` → `renderer_shutdown()`. Frame cb: `renderer_begin_frame() → game_tick(dt) → renderer_end_frame()`. Input cb: forward `sapp_event*` to engine input handler. | LOW | DONE | — | — | SETUP | PG-SETUP-A | SELF-CHECK | files: src/game/main.cpp |
| G-002 | Create game lifecycle skeleton in `src/game/game.h` and `src/game/game.cpp` with `void game_init();`, `void game_tick(float dt);`, `void game_shutdown();` declarations and empty bodies. Body of `game_tick` calls `engine_tick(dt)` only. | LOW | DONE | — | — | SETUP | PG-SETUP-A | SELF-CHECK | files: src/game/game.h, src/game/game.cpp |

**Checkpoint**: `game` builds and runs, opens a clear-color window, exits on Esc.

---

## Milestone: FOUNDATION — Components, Constants, Spawn Factories, Render Submit

**Expected outcome**: All shared types and entity factories exist. Game compiles with empty scene; `render_submit()` already iterates the (empty) view and would draw if entities existed. No user-story work begins until this milestone is DONE.

**CRITICAL**: BLOCKS all G-M1+ tasks per blueprint §10.4.

| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
|---|---|---|---|---|---|---|---|---|---|
| G-003 | Define all game-layer ECS components in `src/game/components.h`: tag structs (`PlayerTag`, `EnemyTag`, `AsteroidTag`, `ProjectileTag`); state structs (`Health`, `Shield`, `Boost`, `WeaponState`, `ProjectileData`, `EnemyAI`, `AsteroidData`); enums (`WeaponType { Laser, Plasma }`, `SizeTier { Small, Medium, Large }`, `MatchPhase { Playing, PlayerDead, Victory, Restarting }`). Field defaults match `docs/planning/speckit/game/data-model.md`. | MED | DONE | — | G-002 | FOUNDATION | PG-FOUND-A | SPEC-VALIDATE | files: src/game/components.h. Cross-cutting type: every other game system reads/writes one of these structs |
| G-004 | Define all tuning constants in `src/game/constants.h`: player accel/turn/strafe/boost multipliers; HP/Shield/Boost max + regen rates + delays; laser/plasma damage + cooldown + plasma_speed + plasma_lifetime; field_radius (1000), asteroid_count (200), max_asteroid_speed (50); 3 size-tier mass/scale/velocity ranges; camera offset + lag; auto-restart delays (death 2s, win 3s); enemy pursuit_speed/fire_range/fire_cooldown. | LOW | DONE | — | G-002 | FOUNDATION | PG-FOUND-A | SELF-CHECK | files: src/game/constants.h. All tuning lives here — never inline magic numbers in system code |
| G-005 | Implement entity spawn factories in `src/game/spawn.h` and `src/game/spawn.cpp`: `spawn_player(pos)`, `spawn_enemy(pos)`, `spawn_asteroid(pos, size_tier)`, `spawn_projectile(pos, vel, owner)`. Each composes engine + game components per archetypes table in `docs/architecture/game-architecture.md`. `Dynamic` entities must get `ForceAccum` + `RigidBody.inv_inertia_body` initialized via `make_box_inv_inertia_body` / `make_sphere_inv_inertia_body`. Asset spawns: `engine_spawn_from_asset` for ship/asteroid glTF; fallback to procedural primitive on `{0}` handle. | MED | DONE | — | G-003, G-004 | FOUNDATION | SEQUENTIAL | SELF-CHECK | files: src/game/spawn.h, src/game/spawn.cpp |
| G-006 | Implement `render_submit()` in `src/game/game.cpp`: iterate `engine_registry().view<Transform, Mesh, EntityMaterial>()`, compose TRS model matrix, call `renderer_enqueue_draw(mesh.handle, model[16], material)`. Push directional light via `renderer_set_directional_light()` from the singleton Light entity. Camera matrix is set by engine via `engine_set_active_camera()` (do not call `renderer_set_camera` here). | MED | DONE | — | G-003 | FOUNDATION | SEQUENTIAL | SELF-CHECK | files: src/game/game.cpp |

**Checkpoint**: Game compiles; `render_submit()` iterates an empty view; ready for G-M1.

---

## Milestone: G-M1 — Flight + Scene + Camera

**Expected outcome**: Player flies through 200 procedurally-placed asteroids with a third-person camera that follows. No collision damage yet (G-M2). No combat (G-M3). Match runs in `Playing` indefinitely.

**Cross-workstream prerequisites**: Renderer R-M1 + Engine E-M1 (both delivered).

| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
|---|---|---|---|---|---|---|---|---|---|
| G-007 | Implement player flight controller in `src/game/player.h` and `src/game/player.cpp`: `player_update(dt)` reads `engine_key_down(W/S/A/D/SPACE)` for thrust/strafe/boost; reads `engine_mouse_delta()` while `engine_mouse_button(0)` (LMB) is held for pitch/yaw; applies forces via `engine_apply_force(player, ...)`; toggles `Boost.active`; thrust multiplier doubles when `Boost.active && Boost.current > 0`. | HIGH | DONE | — | G-005 | G-M1 | PG-GM1-A | SELF-CHECK | files: src/game/player.h, src/game/player.cpp. Deviation: uses direct velocity modification (rb.linear_velocity += accel * dt) instead of engine_apply_force(); functionally equivalent for pure acceleration |
| G-008 | Implement third-person camera rig in `src/game/camera_rig.h` and `src/game/camera_rig.cpp`: `camera_rig_update(dt)` computes target pose = `player_transform + offset_local_to_player`; smooth-follow via `lerp(current, target, lag_factor * dt)`; writes to camera entity Transform; calls `engine_set_active_camera(camera_entity)`. Constants from `constants.h`. First-person fallback = offset_local = vec3(0). | MED | DONE | — | G-005 | G-M1 | PG-GM1-A | SELF-CHECK | files: src/game/camera_rig.h, src/game/camera_rig.cpp. Contingency: switch to first-person fallback if rig stalls > T+1h (per `docs/planning/speckit/game/research.md` R4). Also implements yaw-only offset and aims camera at player |
| G-009 | Implement asteroid field placement in `src/game/asteroid_field.h` and `src/game/asteroid_field.cpp`: `asteroid_field_init()` spawns `FieldConfig.asteroid_count` (200) asteroids with random positions inside the containment sphere, random size tier (`Small`/`Medium`/`Large`), per-tier mass/scale/initial-linear-velocity/initial-angular-velocity ranges from `constants.h`. Smaller asteroids start faster. Calls `spawn_asteroid` per entity. | MED | DONE | — | G-005 | G-M1 | PG-GM1-A | SELF-CHECK | files: src/game/asteroid_field.h, src/game/asteroid_field.cpp. Fixed seed (42u) for reproducible layout; uniform volume sampling; spawn_asteroid extended to accept initial velocities |
| G-010 | Wire `game_init()` in `src/game/game.cpp`: `spawn_player(origin)`, `asteroid_field_init()`, create camera entity (Transform + Camera + CameraActive), call `engine_set_active_camera(cam)`, create directional light entity (Light{direction, color, intensity}), zero MatchState (`phase=Playing, enemies_remaining=0` until G-M3). | MED | DONE | — | G-007, G-008, G-009 | G-M1 | SEQUENTIAL | SPEC-VALIDATE | files: src/game/game.cpp. Lights up the full G-M1 scene. Scope creep: enemy spawn (G-021), match state machine (G-024), VFX/death handling (G-021), restart/quit input (G-026), full 13-step tick wiring (G-M2+M3+M4) also in game.cpp |

**Checkpoint (G-M1 acceptance)**: `./build/game` opens, player flies through 200 asteroids with third-person camera. Asteroids hold initial velocities but physics has not yet been validated for the game-side wiring (asteroids drift only because engine E-M4 ticks them). Spec scenarios US1.1 (movement + boost behavior), partially US1.4 (containment is **not** yet implemented — G-M2). **Validation**: SPEC-VALIDATE on G-010 covers FR-001/FR-002/FR-014.

---

## Milestone: G-M2 — Physics, Containment, Asteroid Dynamics

**Expected outcome**: Field feels alive — asteroids drift and tumble, ship bounces off asteroids, ship and asteroids reflect off the spherical containment boundary, asteroid speeds are post-reflection-capped, player takes shield→HP damage on hard hits, shield regenerates after 3s no-damage, boost drains and regens.

**Cross-workstream prerequisites**: Renderer R-M1 + Engine E-M4 (delivered).

| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
|---|---|---|---|---|---|---|---|---|---|
| G-011 | Implement containment update in `src/game/asteroid_field.cpp`: `containment_update()` iterates entities with `Transform + RigidBody`; if `length(position) > FieldConfig.radius`, reflect `RigidBody.linear_velocity` along the inward normal `n = -normalize(position)`; for `AsteroidTag` entities, clamp post-reflection speed to `FieldConfig.max_asteroid_speed`; for `PlayerTag`, apply small damping (no damage) on reflection. | MED | DONE | — | G-010 | G-M2 | PG-GM2-A | SPEC-VALIDATE | files: src/game/asteroid_field.cpp. Energy containment: speed cap on every reflection event prevents floating-point growth (research R8) |
| G-012 | Implement collision damage pipeline in `src/game/damage.h` and `src/game/damage.cpp`: `damage_resolve()` reads engine collision events from the current tick (via `engine_overlap_aabb` or by querying entity-pair list — pick the engine-exposed path), computes relative kinetic energy `0.5 * (m1 + m2) * abs(v_rel)^2`, applies damage proportional to KE to shield first, then HP. Records `Shield.last_damage_time = engine_now()`. Marks death when `Health.current <= 0` by attaching `DestroyPending` (engine sweeps next tick). **No threshold** — every collision deals damage. | HIGH | DONE | — | G-010 | G-M2 | PG-GM2-A | SPEC-VALIDATE + REVIEW | files: src/game/damage.h, src/game/damage.cpp. Deviation: uses reduced-mass KE `0.5 * m_r * approach_spd²` (m_r = m1*m2/(m1+m2), approach_spd = v_rel·sep_dir) instead of spec formula `0.5*(m1+m2)*|v_rel|²`. Physically correct for collision energy dissipation; additionally gated by approach-direction check to fire once per contact. Damage scaled by `kinetic_damage_scale` constant. |
| G-013 | Implement shield + boost regen in `src/game/player.cpp`: `player_update(dt)` extends to (a) shield regen — if `engine_now() - Shield.last_damage_time > Shield.regen_delay`, `Shield.current = min(max, current + regen_rate * dt)`; (b) boost — if `Boost.active && Boost.current > 0`, `current = max(0, current - drain_rate*dt)`; if `!Boost.active`, `current = min(max, current + regen_rate*dt)`. | MED | DONE | — | G-007 | G-M2 | PG-GM2-A | SELF-CHECK | files: src/game/player.cpp. PG-GM2-A is disjoint: asteroid_field.cpp / damage.cpp+h / player.cpp |
| G-014 | Wire G-M2 systems into `game_tick(dt)` order in `src/game/game.cpp`: insert `containment_update()` (step 2), `player_update(dt)` (step 3), `damage_resolve()` (step 7) according to the 11-step tick contract in `docs/interfaces/game-interface-spec.md`. `camera_rig_update` (step 9) and `render_submit` (step 10) already wired in G-010. | MED | DONE | — | G-011, G-012, G-013 | G-M2 | SEQUENTIAL | SPEC-VALIDATE | files: src/game/game.cpp. Tick order is contractual — do not reorder without freeze-procedure approval. Tuning pass 2026-04-26: field_radius 1000→350, asteroid_count 200→400, all size tiers scaled up (~2×), large mass 60→200 |

**Checkpoint (G-M2 acceptance)**: Field feels alive; ship bounces; boundary reflects; speed cap holds; player takes damage and dies on enough collisions; boost works. Spec scenarios US1.2, US1.3, US1.4. **Validation**: SPEC-VALIDATE + REVIEW on G-012 (FR-005 risk). SPEC-VALIDATE on G-011 (FR-013) and G-014 (tick order contract).

---

## Milestone: G-M3 — Weapons + Enemies + HP/Shield

**Expected outcome**: Full combat loop with one enemy. Q/E switches weapons. Right-mouse fires. Laser instant-hit, 5s cooldown, line visual. Plasma rapid-fire projectile, 0.1s cooldown. Enemy seeks player, fires back. Enemy death produces minimal explosion VFX. `MatchState.enemies_remaining` decrements on enemy death.

**Cross-workstream prerequisites**: Renderer R-M1 + R-M3 (line quads delivered) + Engine E-M3 (raycast delivered) + E-M4 (rigid bodies delivered).

| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
|---|---|---|---|---|---|---|---|---|---|
| G-015 | Implement weapon definitions and switching in `src/game/weapons.h` and `src/game/weapons.cpp`: read laser/plasma stats from `WeaponState` (initialized from `constants.h`); Q/E edge-triggered (track `prev_q`/`prev_e`) writes `WeaponState.active_weapon`. Cooldown helper `can_fire(weapon, now)`. `weapon_update(dt)` body wires fire-input gating but does not yet emit fire (G-018/G-019). | MED | TODO | — | G-006 | G-M3 | PG-GM3-A | SELF-CHECK | files: src/game/weapons.h, src/game/weapons.cpp |
| G-016 | Implement projectile lifetime + collision despawn in `src/game/projectile.h` and `src/game/projectile.cpp`: `projectile_update(dt)` iterates `view<ProjectileTag, ProjectileData>()`; if `engine_now() - spawn_time > lifetime`, attach `DestroyPending`. Collision-despawn is recorded by `damage_resolve` (G-020) but the despawn marker is also written here for lifetime-driven cleanup. | MED | TODO | — | G-006 | G-M3 | PG-GM3-A | SELF-CHECK | files: src/game/projectile.h, src/game/projectile.cpp |
| G-017 | Implement game-local enemy AI in `src/game/enemy_ai.h` and `src/game/enemy_ai.cpp`: `enemy_ai_update(dt)` iterates `view<EnemyTag, Transform, EnemyAI, WeaponState>()`; computes `seek = normalize(player_pos - enemy_pos) * pursuit_speed` and writes to `RigidBody.linear_velocity` (or applies as force — pick one and document); LOS check via `engine_raycast(enemy_pos, normalize(player_pos - enemy_pos), fire_range)` — if hit returns the player entity AND distance < fire_range AND `engine_now() - last_fire_time > fire_cooldown`, spawn a plasma projectile aimed at the player. | HIGH | TODO | — | G-006 | G-M3 | PG-GM3-A | SPEC-VALIDATE | files: src/game/enemy_ai.h, src/game/enemy_ai.cpp. AI is ~20 lines — keep it simple per research R5; do **not** depend on engine E-M5 |
| G-018 | Implement laser fire path in `src/game/weapons.cpp`: when player active weapon is Laser AND right-mouse held AND cooldown allows, `engine_raycast(player_pos, player_forward, laser_max_distance)`; submit `renderer_enqueue_line_quad(muzzle_world, hit_or_max, width, color_with_alpha)` for ~0.5s with fade; queue damage event for `damage_resolve` (G-020); apply small impulse via `engine_apply_impulse_at_point` on small-asteroid hits. Set `WeaponState.laser_last_fire = engine_now()`. | HIGH | TODO | — | G-015 | G-M3 | SEQUENTIAL | SPEC-VALIDATE | files: src/game/weapons.cpp. Touches same file as G-015/G-019 — sequential, not parallel. SPEC-VALIDATE for FR-006/FR-007 (instant-hit + cooldown) |
| G-019 | Implement plasma fire path in `src/game/weapons.cpp`: when active weapon is Plasma AND right-mouse held AND cooldown allows, `spawn_projectile(muzzle, player_forward * plasma_speed, player_entity)` once per `plasma_cooldown` while held (auto-fire). Also called by enemy AI for enemy plasma. Set `last_fire = engine_now()`. | MED | TODO | — | G-018 | G-M3 | SEQUENTIAL | SPEC-VALIDATE | files: src/game/weapons.cpp. SPEC-VALIDATE for FR-006/FR-007 (rapid cadence + frame-rate independence) |
| G-020 | Extend damage pipeline in `src/game/damage.cpp`: handle projectile-entity collisions — if collision involves a `ProjectileTag` and the other entity has `Health` AND `other != ProjectileData.owner`, apply `ProjectileData.damage` to shield→HP and `DestroyPending` the projectile. Apply queued laser raycast damage with the same shield→HP cascade. Owner-self collisions are silently ignored. | HIGH | TODO | — | G-012, G-018, G-019 | G-M3 | SEQUENTIAL | SPEC-VALIDATE + REVIEW | files: src/game/damage.cpp. High risk — owner-exclusion + projectile despawn must be exact |
| G-021 | Add enemy spawn to `game_init` and implement death VFX + `enemies_remaining` tally. In `src/game/spawn.cpp`: `spawn_enemy` creates the enemy archetype. In `src/game/game.cpp`: `game_init` increments `MatchState.enemies_remaining` on every enemy spawn; `match_state_update` (placeholder until G-024) detects `EnemyTag` entities with `Health.current <= 0`, spawns a short-lived scaled sphere with alpha-blended material at the enemy's position (minimal explosion VFX), `engine_destroy_entity(enemy)`, decrements `enemies_remaining`. | MED | TODO | — | G-005 | G-M3 | SEQUENTIAL | SELF-CHECK | files: src/game/spawn.cpp, src/game/game.cpp |
| G-022 | Wire G-M3 systems into `game_tick(dt)` in `src/game/game.cpp`: insert `enemy_ai_update(dt)` (step 4), `weapon_update(dt)` (step 5), `projectile_update(dt)` (step 6) per the 11-step contract. Verify steps 1–10 are now wired (HUD step 11 lands in G-M4). | MED | TODO | — | G-015, G-016, G-017, G-018, G-019, G-020, G-021 | G-M3 | SEQUENTIAL | SPEC-VALIDATE | files: src/game/game.cpp. Tick-order contract |

**Checkpoint (G-M3 acceptance)**: Player can fire both weapons; weapons feel distinct; one enemy seeks and shoots; player and enemy can damage and destroy each other; explosion VFX appears on enemy death. Spec scenarios US2.1, US2.2, US2.3, US2.4. **Validation**: SPEC-VALIDATE + REVIEW on G-020 (damage routing risk). SPEC-VALIDATE on G-017/G-018/G-019/G-022.

---

## Milestone: G-M4 — HUD + Game Flow + Restart

**Expected outcome**: Full demo loop. ImGui HUD shows HP/Shield/Boost/crosshair/weapon/cooldown. 4-state match machine drives win/death overlays and auto- + manual-restart. Esc quits. **Game MVP complete.**

**Cross-workstream prerequisites**: Renderer R-M1 + renderer-owned `sokol_imgui` (delivered) + Engine E-M1.

| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
|---|---|---|---|---|---|---|---|---|---|
| G-023 | Implement ImGui HUD in `src/game/hud.h` and `src/game/hud.cpp`: `hud_render()` builds ImGui::Begin/End windows with HP bar (red, `Health.current/max`), Shield bar (blue), Boost bar (yellow), centered crosshair, active-weapon text, and per-weapon cooldown timer (`max(0, cooldown - (now - last_fire))`). Reads player + matchstate; never writes ECS. | MED | TODO | — | G-006 | G-M4 | PG-GM4-A | SELF-CHECK | files: src/game/hud.h, src/game/hud.cpp |
| G-024 | Implement 4-state match state machine in `src/game/game.cpp`: `match_state_update(dt)` evaluates **Victory before PlayerDead** (research R2 — simultaneous death yields Victory). Transitions: Playing → Victory when `enemies_remaining == 0`; Playing → PlayerDead when player Health == 0. PlayerDead → Restarting after `auto_restart_delay = 2.0s` OR `engine_key_down(ENTER)` edge. Victory → Restarting after `3.0s` OR Enter. Restarting clears game-tagged entities then re-runs spawn sequence and transitions to Playing. Tracks `phase_enter_time = engine_now()` on every transition. | HIGH | TODO | — | G-010 | G-M4 | PG-GM4-A | SPEC-VALIDATE + REVIEW | files: src/game/game.cpp. FR-010, FR-012; clarification 2026-04-23 (4 phases, simultaneous death) |
| G-025 | Implement win + death overlays in `src/game/hud.cpp`: when `MatchState.phase == Victory`, render large centered "YOU WIN" + "Press Enter to restart"; when `phase == PlayerDead`, render "YOU DIED" + same prompt. Overlays only render in terminal phases — Restarting is a single transition tick. | MED | TODO | — | G-023, G-024 | G-M4 | SEQUENTIAL | SELF-CHECK | files: src/game/hud.cpp |
| G-026 | Implement restart + quit flow in `src/game/game.cpp`: Restarting phase iterates and `engine_destroy_entity(e)` every entity tagged `PlayerTag`/`EnemyTag`/`AsteroidTag`/`ProjectileTag` (preserves camera + light entities); after engine deferred-destroy sweep next tick, re-run the `game_init` spawn sequence and set `phase = Playing`. Quit: edge-triggered `engine_key_down(ESCAPE)` calls `sapp_request_quit()` (declared in `<sokol_app.h>` via renderer transitive include — confirm available; otherwise add a renderer-side helper `renderer_request_quit()` and request a 2-minute renderer notice). | MED | TODO | — | G-024 | G-M4 | SEQUENTIAL | SPEC-VALIDATE | files: src/game/game.cpp. **Watchpoint**: `sapp_request_quit` direct-call is a renderer-internal symbol. If `renderer.h` does not transitively expose it, file a BLOCKER and request `renderer_request_quit()` per blueprint §8.5 rule 11. |
| G-027 | Wire G-M4 systems into `game_tick(dt)` in `src/game/game.cpp`: insert `match_state_update(dt)` (step 8) and `hud_render()` (step 11) per the 11-step contract. `hud_render` runs between `renderer_begin_frame` and `renderer_end_frame` so ImGui widget calls land in the renderer's `simgui` frame. | MED | TODO | — | G-023, G-024, G-025, G-026 | G-M4 | SEQUENTIAL | SPEC-VALIDATE | files: src/game/game.cpp. Tick-order contract — final wiring |

**Checkpoint (G-M4 acceptance — Game MVP complete)**: Full demo loop start → fly → fight → win or die → overlay → restart. HUD reflects all combat state. Esc quits. Spec scenarios US3.1, US3.2, US3.3, US3.4. **Validation**: milestone gate — SPEC-VALIDATE + REVIEW + human behavioral check on demo machine (`rtx3090`).

---

## Milestone: POLISH — Tuning + Acceptance + E2E

**Expected outcome**: Tuning pass; all spec acceptance scenarios validated; quickstart runbook end-to-end pass on demo machine.

| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
|---|---|---|---|---|---|---|---|---|---|
| G-028 | Tune gameplay constants in `src/game/constants.h`: player accel/turn/strafe rates, boost multiplier, damage scaling factor, weapon cadence, enemy pursuit_speed, field_radius, camera lag factor, asteroid density. Iterate against demo feel. | LOW | TODO | — | G-027 | POLISH | PG-POLISH-A | SELF-CHECK | files: src/game/constants.h. No new code; tuning only |
| G-029 | Validate full gameplay loop against all spec acceptance scenarios in `docs/planning/speckit/game/spec.md` (US1.1–US1.4, US2.1–US2.4, US3.1–US3.4) and edge cases (simultaneous death, cooldown overlap, fast object containment, restart-while-countdown). | MED | TODO | — | G-027 | POLISH | PG-POLISH-A | SPEC-VALIDATE | files: (no code) — produces a validation report under `_coordination/game/VALIDATION/` |
| G-030 | Run end-to-end quickstart validation per `docs/planning/speckit/game/quickstart.md` on the demo machine (`rtx3090`): build, launch, exercise all controls, complete a full win cycle and a full death cycle, confirm HUD elements update, confirm Esc quits cleanly. | MED | TODO | — | G-029 | POLISH | SEQUENTIAL | REVIEW | files: (no code) — produces a runbook execution report. Final pre-demo gate |

**Checkpoint**: All milestones merged to `main`. Demo-day ready.

---

## Parallel Group Summary

| Group | Milestone | Tasks | Disjoint file sets |
|---|---|---|---|
| PG-SETUP-A | SETUP | G-001, G-002 | `main.cpp` / `game.h, game.cpp` |
| PG-FOUND-A | FOUNDATION | G-003, G-004 | `components.h` / `constants.h` |
| PG-GM1-A | G-M1 | G-007, G-008, G-009 | `player.{h,cpp}` / `camera_rig.{h,cpp}` / `asteroid_field.{h,cpp}` |
| PG-GM2-A | G-M2 | G-011, G-012, G-013 | `asteroid_field.cpp` / `damage.{h,cpp}` / `player.cpp` |
| PG-GM3-A | G-M3 | G-015, G-016, G-017 | `weapons.{h,cpp}` / `projectile.{h,cpp}` / `enemy_ai.{h,cpp}` |
| PG-GM4-A | G-M4 | G-023, G-024 | `hud.{h,cpp}` / `game.cpp` |
| PG-POLISH-A | POLISH | G-028, G-029 | `constants.h` / (no code) |

**File-ownership rule reminder (blueprint §10.3)**: tasks in the same parallel group must touch **disjoint file sets**. If an agent discovers it must touch a file outside its declared `files:`, pause, update the row, check for conflicts, and file a `BLOCKERS.md` entry if another task in the same PG owns that file.

---

## Milestone Dependency Graph

```
SETUP (G-001..G-002) → FOUNDATION (G-003..G-006)
    └─► G-M1 (G-007..G-010)
            └─► G-M2 (G-011..G-014)
                    └─► G-M3 (G-015..G-022)
                            └─► G-M4 (G-023..G-027)
                                    └─► POLISH (G-028..G-030)
```

User-story to milestone mapping (from `docs/planning/speckit/game/tasks.md`):

| Phase | User Story | Maps to milestones |
|-------|------------|--------------------|
| Phase 1 | Setup | SETUP |
| Phase 2 | Foundational | FOUNDATION |
| Phase 3 | US1 — Fly and Survive | G-M1 + G-M2 |
| Phase 4 | US2 — Engage Enemies | G-M3 |
| Phase 5 | US3 — Match Flow | G-M4 |
| Phase 6 | Polish | POLISH |

---

## Cross-Workstream Dependencies

| Game Milestone | Requires Renderer | Requires Engine | Mock fallback |
|---|---|---|---|
| SETUP | R-M1 (or `USE_RENDERER_MOCKS=ON`) | E-M1 (or `USE_ENGINE_MOCKS=ON`) | both available |
| FOUNDATION | R-M1 | E-M1 | both available |
| G-M1 | R-M1 | E-M1 | both available |
| G-M2 | R-M1 | E-M4 | engine mock cannot simulate physics — must be real |
| G-M3 | R-M1 + R-M3 (line quads) | E-M3 (raycast) + E-M4 | renderer line-quad support is required for laser visual |
| G-M4 | R-M1 + renderer-owned `sokol_imgui` | E-M1 | renderer ImGui integration is required |
| POLISH | full real upstream | full real upstream | demo-fallback only |

At promotion (2026-04-26) every required upstream milestone has been delivered.

---

## Validation Levels — Summary

| Level | Tasks | Rationale |
|-------|-------|-----------|
| `NONE` | — | (none — every task has at least SELF-CHECK) |
| `SELF-CHECK` | G-001, G-002, G-004, G-005, G-006, G-008, G-009, G-013, G-015, G-016, G-021, G-023, G-025, G-028 | Trivial / mechanical; failure visible in build or smoke test |
| `SPEC-VALIDATE` | G-003, G-010, G-011, G-014, G-017, G-018, G-019, G-022, G-026, G-027, G-029 | Cross-cutting types, contract-bearing logic, spec FR coverage |
| `REVIEW` | G-030 | Final pre-demo runbook execution gate |
| `SPEC-VALIDATE + REVIEW` | G-012, G-020, G-024 | High-risk semantics: KE damage (FR-005), projectile damage routing, match state machine |

Per blueprint §8.6 every milestone merge requires Spec Validator + human behavioral check + lightweight Code Review independent of per-task validation.

---

## Notes for Agents

1. The **11-step `game_tick(dt)` order** (in `docs/interfaces/game-interface-spec.md`) is contractual. Reordering or skipping a step is a contract change — file a blocker, do not silently change.
2. **Tick step indices** are insert points, not a sequence to write top-to-bottom. Each milestone fills a few steps of the same `game_tick` body; G-014 / G-022 / G-027 are the wiring tasks.
3. **No hard-coded asset paths.** Compose all asset paths from `ASSET_ROOT` (generated `paths.h`).
4. **No magenta on success path.** Renderer falls back to magenta on shader failure; the game itself does not produce magenta — if you see it, it's an upstream regression.
5. **Game-local AI for MVP.** Do not add a dependency on engine E-M5 — even if the engine team ships steering before G-M3, the MVP enemy AI stays game-local. The G-M5 (Desirable) swap is a separate task pass.
6. **Match state evaluation order** is: enemies_remaining check (Victory) **before** player Health check (PlayerDead). This is non-negotiable per clarification 2026-04-23.
