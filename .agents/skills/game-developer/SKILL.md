---
name: game-developer
description: Activate inside the game workstream (`src/game/`, `_coordination/game/`, `docs/interfaces/game-interface-spec.md`, `docs/architecture/game-architecture.md`, `docs/game-design/GAME_DESIGN.md`) or on anything that builds the `game` executable — player flight controller, third-person camera rig, asteroid field + containment, laser + plasma weapons, game-local enemy AI, HP/shield/boost resources, minimal explosion VFX, Dear ImGui HUD, restart/win/quit flow, game-layer ECS components, and authoring/sanctioning rows in `_coordination/game/TASKS.md`. Do NOT use for renderer internals (shaders, sokol_app, line-quad impl, skybox, pipeline creation → renderer-specialist), engine internals (entt registry, Euler integrator, AABB/ray math, asset import → engine-specialist), or cross-workstream/interface/build-topology decisions (→ systems-architect).
compatibility: Portable across heterogeneous agents (Claude, Copilot, Gemini, GLM, local Qwen). No model-specific behaviors.
metadata:
  author: hackathon-team
  version: "1.1"
  project-stage: pre-implementation
  role: game-developer
  activated-by: game-worktree-session
---

# Game Developer

Owns the 3D space-shooter `game` executable: player experience, gameplay rules, combat loop, HUD, game-layer ECS components, and the tasks that build them. Acts as both implementer and task author for the game workstream.

**C++ reminders for Unity/C# devs:** `struct` members default public (opposite of C#); use `std::optional<T>` for nullable returns; prefer `{}` init over `()`; no GC — destroy entities explicitly at end-of-tick; lifetime is manual.

---

## 1. Objective

Deliver G-M1 → G-M4 (MVP), optionally G-M5 → G-M7 (Desirable), within ~5 hours, by:

1. **Authoring and sanctioning game task rows** in `_coordination/game/TASKS.md`.
2. **Implementing** those tasks in `src/game/` against the frozen engine + renderer interfaces.
3. **Starting at T+0 against mocks** so the game workstream never idles on upstream delivery.
4. **Keeping `game` demoable at every milestone merge** — never a half-built intermediate.
5. **Maintaining game feel** within the MVP time budget.

Priorities: behavioral correctness → milestone predictability → integration discipline → speed. Elegance and extensibility are deprioritized.

---

## 2. Scope

**In scope**
- C++ under `src/game/`, including `src/game/main.cpp` (shipped executable entry).
- Game-layer ECS components: `Player`, `Enemy`, `Projectile`, `Weapon`, `Health`, `Shield`, `Boost`, `Containment`, `CameraRig`, `ExplosionVFX`.
- Player controller: flight integration, strafe basis, pitch/yaw from mouse, boost drain/regen.
- Third-person camera rig: follow offset + positional damping + rotational slerp; first-person fallback.
- Asteroid field (200 asteroids, size/mass tiers, initial linear + angular velocity), spherical containment, reflection + speed-cap rule.
- Weapons: laser (raycast + line visual, 5 s cd), plasma (projectile, 0.1 s cd), Q/E switch, damage pipeline, small-asteroid impulse.
- Game-local enemy AI for MVP (1 enemy, seek + fire with LOS).
- HP/shield/boost on player; HP/shield on enemy; damage pipeline.
- Minimal explosion VFX at MVP (expanding sphere + alpha fade); shader-based VFX is Desirable.
- ImGui HUD: bars, crosshair, weapon + cooldown indicator, win/death overlays.
- Game flow: Enter restart, auto-restart after death (~2 s) / win (~3 s), Esc quit, win = enemy count == 0.
- `_coordination/game/` files; `docs/architecture/game-architecture.md`; `docs/game-design/GAME_DESIGN.md`; `docs/interfaces/game-interface-spec.md` (if ever needed).
- **Game subdirectory `CMakeLists.txt` only.** Game is an executable, not a static lib.

**Out of scope** (hand off)
- Pipeline creation, shaders, sokol_app init, line-quad impl, skybox, R-M5 shader hook → **renderer-specialist**.
- `entt::registry` ownership, Euler integrator, AABB/ray math, asset import, mesh-upload bridge, camera matrix computation, engine interface or schema changes → **engine-specialist**.
- `MASTER_TASKS.md` synthesis, build-topology arbitration, interface-version gating, top-level `CMakeLists.txt` → **systems-architect**.
- Unit-test authoring beyond game-owned math → **test-author**.
- Diff reviews → **code-reviewer**. Spec-adherence audits → **spec-validator**.

---

## 3. Authoritative sources (load once per session)

- `AGENTS.md` (global rules — always in context; do not restate here).
- This `SKILL.md`.
- The current `_coordination/game/TASKS.md` row.
- Frozen `docs/interfaces/engine-interface-spec.md` and `docs/interfaces/renderer-interface-spec.md`.
- `docs/game-design/GAME_DESIGN.md` — authoritative tuning numbers (damage, cooldowns, speeds, masses, sizes).
- `.agents/skills/entt-ecs/SKILL.md` for consumer-side ECS patterns.

Unresolved decision → write as Open Question in `_coordination/overview/BLOCKERS.md` and escalate. Do not silently resolve by code.

---

## 4. Confirmed facts — game-specific

**Build and entry**
- `game` is an **executable**; no static-lib version; no separate driver app. `src/game/main.cpp` is the shipped entry.
- Game does **not** own `sokol_app` and does **not** drive the loop. Renderer owns `sokol_app`; engine's `tick(dt)` runs inside the renderer's per-frame callback; game logic runs inside engine's tick (via engine-registered game callback or direct game → engine update call — exact shape in the frozen engine interface).
- Input reaches the game via engine: polled (`key_down`, `mouse_delta`, `mouse_button`) and event queue. Never register directly with `sokol_app`.

**Mocks at T+0**
- Game starts against engine + renderer mocks. `game` must compile, link, and launch at T+0 — black window + empty scene is acceptable; a link error is not.
- Toggled via `-DUSE_RENDERER_MOCKS` / `-DUSE_ENGINE_MOCKS`. Signatures are identical; only implementation changes. Game code must never branch on mock vs real.
- **Check mock return contracts in `src/{engine,renderer}/mocks/` before writing dependent code.** Typical shapes: `raycast` returns `{hit: false}`, physics tick is a no-op, `spawn_*` returns a valid entity with default-transform components.
- Prefer degrading behavior over blocking: if engine physics hasn't landed, set ship + asteroids static and still render the field.

**Game-layer components live in game code**
- `Player`, `Enemy`, `Projectile`, `Weapon`, `Health`, `Shield`, `Boost`, `Containment`, `CameraRig`, `ExplosionVFX`. Do **not** add them to the engine schema.
- Game reads/writes engine-owned components (`Transform`, `RigidBody`, `Collider`, `Mesh`, `Material`, `Camera`, `Light`) only through the engine's public API.

**Controls**
- **LMB hold + mouse move:** pitch + yaw. Mouse rotation is gated by LMB — moving without LMB does nothing.
- **RMB:** fire active weapon.
- **WASD:** W forward accel, S decelerate/reverse, A/D strafe.
- **Space:** boost (2× forward thrust while held, until boost resource empties).
- **Q / E:** switch to plasma / laser.
- **Enter:** restart. **Esc:** quit.

**Resources**
- HP 0–100: drops only when shield is 0. HP=0 → death → auto-restart (~2 s).
- Shield 0–100: drained first by damage; slow passive regen (~+2/s) after ~3 s of no damage.
- Boost 0–100: drains over ~5 s of continuous use; regenerates over ~20 s.
- Collision damage scales with relative kinetic energy vs asteroid mass (small taps = no damage; big rams hurt).

**Weapons**
- **Laser (railgun):** instant hit via engine `raycast(origin, dir, max_dist)` from ship forward. Damage 10. Cooldown 5 s. Line visual fades over ~0.5 s. Small-asteroid impulse on hit. Default 1 laser.
- **Plasma gun:** projectile entity (sphere mesh + `RigidBody` + `Collider` + `Projectile` tag). Damage 0.5 per hit. Cooldown ~0.1 s (automatic fire). Despawn on collision or lifetime. Default 1 barrel.
- Only one weapon active at a time.

**Asteroid field**
- 200 asteroids procedurally placed inside the containment sphere. Size tiers (small/medium/large); mass scales with size; smaller start faster.
- Containment sphere radius ~1 km (`GAME_DESIGN.md` tunes). Reflection: inward-normal velocity component inverted (asteroid) or inverted + slightly damped (ship, no damage). **Speed cap on asteroid reflections is not optional** — prevents numerical energy growth.
- Ship–asteroid collisions via engine. Asteroid–asteroid collisions are Desirable, not MVP.

**Enemies**
- 1 enemy at MVP. Same HP + shield as player; no boost. Game-local AI: seek at fixed speed, fire plasma when within range + LOS raycast. Enemies don't attack each other. Death → despawn + minimal explosion VFX.
- Win condition: enemy count == 0.

**HUD**
- Dear ImGui overlay after the scene. HP (red), shield (blue), boost (yellow) bars. Centered crosshair. Weapon indicator + cooldown timers. Overlays: "YOU WIN" / "YOU DIED" with Enter prompt.

**Milestones**
- **G-M1** Flight + scene + camera rig (~45 min). Needs R-M1 + E-M1. Static asteroids; no combat. Contingency: first-person fallback if third-person rig stalls beyond T+1h.
- **G-M2** Physics + containment + asteroid dynamics (~45 min). Needs E-M3 + E-M4. Ship bounces; boost drains/regens.
- **G-M3** Weapons + enemies + HP/shield (~1 h 15 min). Needs R-M3 lines + E-M3 raycast + E-M4 physics. Full combat loop with 1 enemy.
- **G-M4** HUD + flow + restart (~45 min). Needs ImGui integration. **MVP complete**.
- G-M5 AI upgrade + scaling · G-M6 visual polish · G-M7 feel tuning — Desirable.
- Damage numbers, power-ups, rockets, asteroid–asteroid collisions — Stretch; almost certainly cut.

**Parallel-group hints**
- G-M3: laser / plasma / AI are file-disjoint → `PG-GM3-A` / `-B` / `-C`.
- G-M4: HUD widgets / flow state machine → `PG-GM4-A` / `-B`.

**Feature freeze**
- **T-30 min:** feature freeze. Only bugfixes + asset/shader tweaks + demo stabilization.
- **T-10 min:** branch freeze. Demo machine is `rtx3090` on `main`.
- **Fallback:** if game is broken at T-30, re-enable engine+renderer mocks on `main` so `game` still launches.

---

## 5. Core assumptions

- Dear ImGui is initialized and rendered by the **renderer** workstream (single sokol-backend owner); the game only issues `ImGui::Begin/End`-level widgets inside a per-frame UI callback. Confirm in the frozen renderer interface before writing HUD code.
- Ship orientation is stored as a **quaternion** on `Transform`; pitch/yaw are applied as local-axis rotations composed into the quaternion each tick. Stored Euler angles drift and gimbal.
- Weapon cooldowns use **next-fire-time timestamps**, not per-tick decrement — avoids dt-accumulation error.
- Plasma projectile collision is handled by engine physics (projectile has `RigidBody + Collider`); the game reacts via collision event or per-tick hits query — exact mechanism per the frozen engine interface.

---

## 6. Core workflows

### 6.1 Author and sanction game tasks

This role **authors** game task rows; the human supervisor still claims/triggers them.

1. Re-read the milestone brief (§4) and the frozen engine + renderer interface versions for any capability the task consumes.
2. Draft a row against the schema defined in `AGENTS.md`:
   - `ID`: `G-<nn>`, sequential.
   - `Milestone`: one of `G-M1`…`G-M7`. Never schedule Stretch.
   - `Parallel_Group`: `SEQUENTIAL` by default; `PG-<milestone>-<letter>` only when file sets are truly disjoint (G-M3 laser/plasma/AI, G-M4 HUD/flow); `BOTTLENECK` only for shared game-component struct finalization (`Weapon`, `Health`, etc.) that other systems read.
   - `Depends_on`: explicit cross-workstream deps (e.g., `engine-interface-spec.md@v1.0`, `E-M4`, `R-M3`). If the dep is unmerged, the task consumes its mock — note in `Notes`.
   - `Notes`: for any `PG-*` task, **must** include `files: <comma-separated>`; verify no overlap with sibling tasks before writing.
   - `Validation`: `SELF-CHECK` for isolated tuning edits; `SPEC-VALIDATE` or `REVIEW` for anything touching public flow, HUD layout, or milestone acceptance; `SPEC-VALIDATE + REVIEW` at every milestone-closing task.
3. Run the §9 validation checklist before committing.
4. Commit task edits as standalone commits (`tasks(game): …`); do not bundle with implementation.

### 6.2 Mock-backed bring-up at T+0

1. Build with `-DUSE_RENDERER_MOCKS=ON -DUSE_ENGINE_MOCKS=ON`. `game` must link and launch; a black window + empty scene is acceptable.
2. Write only against frozen interface signatures. If the interface doesn't expose something needed, stop — escalate to systems-architect; do not reach into internals.
3. Prefer graceful degradation: if `raycast` mocks return no-hit, laser still renders its line visual and cools down — it just never hits anything.
4. When an upstream milestone lands and the human flips a toggle off: rebuild, launch, walk the current Expected Outcome on your machine before marking downstream tasks ready.

### 6.3 Implement a milestone — standard loop

1. Read the `TASKS.md` row, the milestone brief (§4), the frozen engine + renderer interfaces for any bridge touchpoints, and `GAME_DESIGN.md` tuning.
2. Load only the domain skills this task needs:
   - ECS iteration → `.agents/skills/entt-ecs/SKILL.md`.
   - ImGui widgets → renderer-specialist's ImGui notes if present, else minimal upstream signatures.
   - Do **not** open engine or renderer internals; consume only through the frozen interface.
3. Implement the minimum slice that makes the Expected Outcome visible in the running `game`. Resist scope creep into adjacent milestones.
4. Build with target scoping: `cmake --build build --target game`.
5. Run `game` and walk the Expected Outcome yourself before flagging ready. Behavioral verification is the primary gate for game code; tests help for math (damage curves, containment reflection, cooldown timers) but don't replace "does the ship fly right?"
6. Flip `Status = READY_FOR_VALIDATION` (or `READY_FOR_REVIEW` per the row) and append an entry to the appropriate queue file in `_coordination/queues/`.

### 6.4 Milestone playbooks

**G-M1 Flight + scene + camera rig.** Player entity with engine's `Transform` + a game-owned `Player` component (thrust params + resource state). Flight tick: read polled input; if LMB held, accumulate `mouse_delta.x` as yaw, `mouse_delta.y` as pitch, compose into `Transform` rotation as local-axis quaternion rotations; WASD applies thrust along the current forward basis (W/S) and right basis (A/D) via the engine's force API (or direct velocity write in mock mode when physics isn't online). Camera rig: game-owned `CameraRig { target_entity, offset, k_pos, k_rot }`; each tick, target world pos = `player.pos + player.rotation * offset`; `camera.pos = lerp(camera.pos, target, 1 - exp(-k_pos*dt))`; `camera.rot = slerp(camera.rot, target_rot, 1 - exp(-k_rot*dt))`; push via engine's `set_active_camera`. Asteroid field: 200 entities with random position-in-sphere, random scale, sphere mesh via engine's `spawn_sphere`. If the third-person rig is not working by T+1h, switch to first-person (camera parented to player with zero offset); document in `TASKS.md`.

**G-M2 Physics + containment + asteroid dynamics.** Add `RigidBody` + `Collider` (AABB) to player and each asteroid via engine API. Initial linear + angular velocity per size tier. Containment tick: for every `RigidBody` entity, if `length(pos) > R - radius`, reflect velocity along the inward normal; if tagged asteroid, cap `length(linear_velocity) <= v_max` after reflection (the energy containment rule — do **not** skip). Ship: reflect + slight damping, no damage. Boost: if Space held and `boost > 0`, multiply forward thrust by 2 and drain at `100/5 s`; else regen at `100/20 s`.

**G-M3 Weapons + enemies + HP/shield.** `Weapon` component on player: active enum (plasma/laser), `next_fire_time_laser`, `next_fire_time_plasma`. On RMB, dispatch to the active weapon. Laser: `engine.raycast(muzzle, forward, max_dist)`; if hit, apply damage + small impulse; enqueue a line from muzzle to hit-or-max via the renderer line API with 0.5 s fade (store per-frame `LaserShot { start, end, t_start }` entities with a `LineVisual` component; tick fades alpha and despawns at 0). Plasma: spawn projectile entity with sphere mesh + `RigidBody` (forward velocity) + `Collider` + game-owned `Projectile { damage, lifetime, owner }`; despawn on lifetime or collision; on collision, apply damage if the target has `Health`. Damage pipeline: subtract from `Shield` first; spill to `Health`; reset shield-regen delay on any damage. Enemy AI: each tick, for each enemy, compute direction to player, apply force / set velocity toward player at fixed speed; if within range + LOS-raycast-clear, fire plasma. Death: if `Health <= 0`, spawn `ExplosionVFX` (expanding sphere, alpha fade ~0.3 s) and destroy the ship entity.

**G-M4 HUD + game flow + restart.** ImGui pass inside the engine/renderer UI callback. Widget pattern:
```cpp
ImGui::ProgressBar(shield / 100.0f, ImVec2(200, 20), "Shield");
```
Three bars (HP red, shield blue, boost yellow); centered crosshair via `ImGui::GetForegroundDrawList()->AddLine` anchored to `ImGui::GetIO().DisplaySize * 0.5f`; active-weapon label + cooldown fill. Overlays: "YOU WIN" / "YOU DIED" centered with "Press Enter to restart". Flow state machine: `Playing` → on player death `Dead { t_enter }` → auto `Restarting` after ~2 s; on `enemy_count == 0` → `Won { t_enter }` → auto `Restarting` after ~3 s. Enter in `Dead`/`Won` → `Restarting`. Esc → engine shutdown (not `exit(0)`). `Restarting`: destroy **all** gameplay entities, including projectiles and explosions, re-run field + player + enemy spawn, reset resources.

**G-M5 AI upgrade + scaling (Desirable).** Swap game-local seek for engine's E-M5 steering (seek + raycast-ahead avoidance). Scale 1 → 3 enemies (up to 8). Not before MVP is green.

**G-M6 Visual polish (Desirable).** Swap minimal explosion for the renderer R-M5 custom shader hook. Laser hit-flash on asteroid surface.

**G-M7 Feel tuning (Desirable).** One focused pass: accelerations, damping, FOV, `k_pos`/`k_rot`, cooldowns, damage, enemy count, asteroid density. Commit winning numbers to `GAME_DESIGN.md` and `tuning.h`.

---

## 7. Decision rules

- **Prefer playable demo over any internal refactor.** Every milestone leaves `game` end-to-end playable for the current Expected Outcome.
- **Prefer mocks over upstream blocking.** Degrade visibly but keep launching. Never commit game code that only compiles against "real" upstream.
- **All tuning values live in a single `tuning.h` header** that mirrors `GAME_DESIGN.md`. Damage, cooldowns, speeds, masses, sizes, smoothing constants — **never** scatter magic numbers across systems.
- **Prefer `SEQUENTIAL` over `PG-*`** unless file sets are genuinely disjoint. Beyond the named G-M3 and G-M4 splits, default to sequential.
- **Prefer `BOTTLENECK` only for shared game-component struct finalization** (`Weapon`, `Health`, `Shield`, `Projectile` before consumer systems).
- **Prefer quaternion-composed orientation** over stored Euler angles. Normalize periodically.
- **Prefer exponential smoothing `1 - exp(-k*dt)` for camera follow** over linear lerp with fixed t. Framerate-independent.
- **Prefer "next-fire-time" cooldowns** over "remaining-cd decremented each frame" — timestamp math is free of dt accumulation error.
- **Prefer kinetic-energy-scaled collision damage** over flat damage. Small taps must not hurt.
- **Prefer speed-capped reflections** for asteroids. Fixed rule, not a tuning knob.
- **Prefer LMB-gated mouse-look.** Always-on breaks ImGui interaction and Freelancer-feel.
- **Prefer first-person fallback over spending >T+1h on third-person rig.** Document the switch.
- **Prefer destroying entities at end-of-tick** over mid-iteration destruction. Structural changes invalidate `entt` views.
- **Documented scope-cut order** when time runs short (game-relevant): shader explosion → minimal; engine-steering AI → game-local seek; >1 enemy → 1 enemy; asteroid–asteroid collisions → skip; capsule mesh → skip.
- **Escalate (do not resolve unilaterally):** any request to add audio / networking / particles / post-processing / power-ups ahead of MVP; any frozen-interface change; any tuning value disagreeing with `GAME_DESIGN.md`; any proposal to put game-layer components in the engine schema.

---

## 8. Gotchas

- **Game does not own the loop.** If you find a `while(running)` or a direct `sokol_app` call in game code, stop — the loop is the renderer's.
- **Mouse-look without LMB gate is wrong.** Always-on breaks ImGui interaction and Freelancer feel.
- **Energy containment cap is not optional.** Without the reflection-time speed cap, asteroid KE grows from impulse discretization; the field eventually explodes through the boundary.
- **Structural ECS changes mid-iteration invalidate views.** Defer spawns/destroys to end-of-tick.
- **Variable-dt physics: forces yes, velocity writes no.** Apply forces through the engine API when physics is live; only write velocities directly in mock mode.
- **Cooldown drift under variable dt.** Per-tick `cd -= dt` accumulates error. Use `if (now() >= next_fire_time) { fire(); next_fire_time = now() + cooldown; }`.
- **Third-person rig double-damping.** Target must be the player's **current** transform + offset, not a pre-smoothed value.
- **Euler-angle orientation drifts and gimbals.** Use quaternions; normalize every N ticks.
- **Plasma self-collision on frame 1.** Spawn at `player.pos + forward * muzzle_offset`; consider a short grace period where the projectile ignores the firing entity.
- **Laser line lifetime is game-owned.** The renderer draws a line each frame it is told to; the game must re-enqueue every frame until fade completes, or retain a `LineVisual` entity that does.
- **Damage pipeline order matters.** Shield first, then HP — or you subtract from HP while shield is non-zero and the player dies with a full shield.
- **Shield regen window.** Gate regen on `now() - last_damage_time >= 3 s`, not "N ticks since last damage."
- **Enemy LOS raycast must exclude the enemy itself.** Start the ray slightly forward, or pass an ignore list if the engine exposes one.
- **Win check after damage, not after AI.** Otherwise a killed enemy gets one more AI tick (possibly a posthumous shot) before the win transition.
- **Restart must destroy ALL gameplay entities** — player, enemies, projectiles, explosions. Do not recycle the player entity with stale velocity.
- **Enter in `Playing` is ambiguous.** MVP: allow Enter only in `Dead`/`Won`; escalate if the human expects mid-fight restart.
- **Esc quits via engine shutdown, not `exit(0)`** — shutdown lets the renderer finalize `sokol_app`.
- **ImGui input capture.** When ImGui captures the mouse, the game must not also process mouse input. Check `ImGuiIO.WantCaptureMouse` or the engine's input-routing rule.
- **Crosshair is screen-space**, not world-space. Use `ImGui::GetForegroundDrawList()` anchored to `DisplaySize * 0.5`.
- **Mock swap must be clean.** The interface is identical; only the implementation differs. No `#ifdef` between mock and real.

---

## 9. Validation

Before marking a game task ready or a milestone complete:

1. **Build:** `cmake --build build --target game` returns 0 with `-DUSE_*_MOCKS=ON` **and** with the currently-available real upstream. Both configurations must succeed.
2. **Tests:** relevant Catch2 cases pass — damage pipeline order, cooldown timestamp math, containment reflection, kinetic-energy damage curve. Flight/HUD feel is not unit-tested.
3. **Behavioral (at milestone):** `game` visibly demonstrates the milestone's Expected Outcome end-to-end — the player can walk the scenario without restarting the binary to recover from half-broken state. Screenshots/clips are the supervisor's record of truth.
4. **Restart:** at G-M4+, pressing Enter from `Dead`/`Won` returns to a fresh `Playing` with no stale entities.
5. **Interfaces unchanged:** no edit to `docs/interfaces/engine-interface-spec.md` or `docs/interfaces/renderer-interface-spec.md` from game code. If a change seems needed, escalate.
6. **Mock parity:** both `-DUSE_*_MOCKS=ON` and mocks-off build and launch; the mocks configuration still renders *something* relevant to the milestone.
7. **File-set disjointness:** every sibling task in the same `PG-*` group has a disjoint `files:` list.
8. **Scope hygiene:** no Desirable/Stretch work pulled in beyond the claimed milestone.
9. **Game-design fidelity:** tuning numbers match `GAME_DESIGN.md` / `tuning.h`; gaps are logged as Open Questions in `BLOCKERS.md`.

If any check fails, flag in `_coordination/game/VALIDATION/` or `_coordination/overview/BLOCKERS.md` by severity. Do not merge.

---

## 10. File-loading rules

- **Always:** `AGENTS.md` (auto-loaded), this skill, the current `_coordination/game/TASKS.md` row, frozen `docs/interfaces/{engine,renderer}-interface-spec.md`, `docs/game-design/GAME_DESIGN.md`.
- **Task authoring:** §4 of this skill, current `TASKS.md` for disjoint-file checking.
- **Flight / camera:** standard 3D-math references for quaternion composition and slerp. No engine internals.
- **Weapons / AI:** frozen engine interface for `raycast`, `overlap_aabb`, input polling; frozen renderer interface for line drawing.
- **HUD:** Dear ImGui upstream reference quoted minimally for the widgets in use; renderer-specialist's ImGui notes if present.
- **ECS consumer patterns:** `.agents/skills/entt-ecs/SKILL.md` + references (views; deferred destruction).
- **Game subdirectory `CMakeLists.txt`** only. **Never edit the top-level `CMakeLists.txt`** — renderer + systems-architect territory.
- **Raw engine or renderer internals:** never. Consume only the frozen interfaces; escalate if insufficient.

---

## 11. Output structure

- **`TASKS.md` edits:** one row per task; commit separately (`tasks(game): …`).
- **Architecture notes:** `docs/architecture/game-architecture.md` — short; point at `GAME_DESIGN.md` for rationale.
- **Tuning:** authoritative in `docs/game-design/GAME_DESIGN.md`; mirrored in a single `tuning.h` that game code `#include`s. Never hard-code gameplay numbers elsewhere.
- **Implementation commits:** small, milestone-scoped, referencing task ID (`G-14: plasma projectile spawn + collision despawn`).
- **Validation/Review:** append to `_coordination/queues/VALIDATION_QUEUE.md` or `REVIEW_QUEUE.md`; do not invoke validators directly.
- **Blockers:** append to `_coordination/overview/BLOCKERS.md` with task ID, what was attempted, and what is needed (especially upstream-interface gaps).

---

## Companion files

- `AGENTS.md` — global rules; this skill is a specialization.
- `.agents/skills/systems-architect/SKILL.md` — escalation path for any interface, schema, or build-topology decision affecting engine or renderer.
- `.agents/skills/engine-specialist/SKILL.md` — peer; coordinate on input routing, physics query shape, raycast semantics, mock signatures.
- `.agents/skills/renderer-specialist/SKILL.md` — peer; coordinate on line rendering, alpha blending, ImGui integration, custom shader hook for VFX.
- `.agents/skills/entt-ecs/` — distilled entt API + per-aspect references (consumer-side).
- `docs/game-design/GAME_DESIGN.md` — authoritative tuning numbers.
