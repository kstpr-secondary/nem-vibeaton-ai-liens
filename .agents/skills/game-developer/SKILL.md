---
name: game-developer
description: Activate inside the game workstream (`src/game/`) or on anything that builds the `game` executable — player flight controller, third-person camera rig, asteroid field + containment, laser + plasma weapons, game-local enemy AI, HP/shield/boost resources, explosion VFX, Dear ImGui HUD, match state machine, restart/win/quit flow, game-layer ECS components. Do NOT use for renderer internals (shaders, sokol_app, pipeline creation → renderer-specialist), engine internals (entt registry, Euler integrator, AABB/ray math, asset import → engine-specialist), or cross-workstream planning (→ systems-architect).
---

# Game Developer

Owns the 3D space-shooter `game` executable: player experience, gameplay rules, combat loop, HUD, game-layer ECS components. Implementer for the game workstream. Operates under `AGENTS.md` global rules.

**C++ reminders for Unity/C# devs:** `struct` members default public (opposite of C#); use `std::optional<T>` for nullable returns; prefer `{}` init over `()`; no GC — destroy entities explicitly at end-of-tick; lifetime is manual.

---

## 1. Objective

Maintain and extend the `game` executable: player controls, combat loop, asteroid field, enemy AI, HUD, match state machine, and game-layer ECS components. Build against the frozen engine + renderer interfaces. Keep `game` demoable at all times — every change should leave a runnable, visually correct binary.

Priorities: behavioral correctness → scope discipline → integration → speed. Elegance and extensibility are deprioritized.

---

## 2. Scope

**In scope**
- C++ under `src/game/`, including `src/game/main.cpp` (shipped executable entry).
- Game-layer ECS components: `Player`, `Enemy`, `Projectile`, `Weapon`, `Health`, `Shield`, `Boost`, `Containment`, `CameraRig`, `ExplosionVFX`.
- Player controller: flight integration, strafe basis, pitch/yaw from mouse, boost drain/regen.
- Third-person camera rig: follow offset + positional damping + rotational slerp; first-person fallback.
- Asteroid field (procedural placement, size/mass tiers, spherical containment, reflection + speed-cap).
- Weapons: laser (raycast + line visual), plasma (projectile), Q/E switch, damage pipeline.
- Game-local enemy AI: seek + fire with LOS.
- HP/shield/boost on player and enemy; damage pipeline.
- Explosion VFX (expanding sphere + alpha fade).
- ImGui HUD: bars, crosshair, weapon + cooldown indicator, win/death overlays.
- Game flow: Enter restart, auto-restart after death/win, Esc quit, win = enemy count == 0.
- `docs/architecture/game-architecture.md`; `docs/game-design/GAME_DESIGN.md`.
- Top-level `CMakeLists.txt` game target (game is an executable, not a static lib).

**Out of scope** (hand off)
- Pipeline creation, shaders, sokol_app init, line-quad impl, skybox → **renderer-specialist**.
- `entt::registry` ownership, Euler integrator, AABB/ray math, asset import, mesh-upload bridge, camera matrix computation → **engine-specialist**.
- Cross-workstream planning, build-topology, top-level CMake changes → **systems-architect**.
- Unit-test authoring → **test-author**. Diff reviews → **code-reviewer**. Spec audits → **spec-validator**.

---

## 3. Confirmed facts

**Build and entry**
- `game` is an **executable**; no static-lib version. `src/game/main.cpp` is the shipped entry.
- Game does **not** own `sokol_app` or drive the loop. Renderer owns `sokol_app`; engine's `tick(dt)` runs inside the renderer's per-frame callback; game logic runs inside engine's tick (via engine interface).
- Input reaches the game via engine: polled (`key_down`, `mouse_delta`, `mouse_button`) and event queue. Never register directly with `sokol_app`.

**Game-layer components live in game code**
- `Player`, `Enemy`, `Projectile`, `Weapon`, `Health`, `Shield`, `Boost`, `Containment`, `CameraRig`, `ExplosionVFX`. Do **not** add them to the engine schema.
- Game reads/writes engine-owned components (`Transform`, `RigidBody`, `Collider`, `Mesh`, `Material`, `Camera`, `Light`) only through the engine's public API.

**Controls**
- **LMB hold + mouse move:** pitch + yaw. Mouse rotation is gated by LMB — moving without LMB does nothing.
- **RMB:** fire active weapon.
- **WASD:** W forward accel, S decelerate/reverse, A/D strafe.
- **Space:** boost (2x forward thrust while held, until boost resource empties).
- **Q / E:** switch to plasma / laser.
- **Enter:** restart. **Esc:** quit.

**Resources**
- HP 0–100: drops only when shield is 0. HP=0 → death → auto-restart (~2 s).
- Shield 0–100: drained first by damage; slow passive regen after ~3 s of no damage.
- Boost 0–100: drains over ~5 s continuous use; regenerates over ~20 s.
- Collision damage scales with relative kinetic energy vs asteroid mass.

**Weapons**
- **Laser (railgun):** instant hit via engine `raycast(origin, dir, max_dist)` from ship forward. Damage 10. Cooldown 5 s. Line visual fades over ~0.5 s. Small-asteroid impulse on hit.
- **Plasma gun:** projectile entity (sphere mesh + `RigidBody` + `Collider` + `Projectile` tag). Damage 0.5 per hit. Cooldown ~0.1 s (automatic fire). Despawn on collision or lifetime.
- Only one weapon active at a time.

**Asteroid field**
- Procedurally placed inside containment sphere. Size tiers (small/medium/large); mass scales with size; smaller start faster.
- Containment sphere reflection: inward-normal velocity component inverted. **Speed cap on asteroid reflections is not optional** — prevents numerical energy growth.
- Ship–asteroid collisions via engine. Asteroid–asteroid collisions are Desirable, not MVP.

**Enemies**
- 1 enemy at MVP (scaling to 3+ is Desirable). Same HP + shield as player; no boost. Game-local AI: seek at fixed speed, fire plasma when within range + LOS raycast. Enemies don't attack each other. Death → despawn + minimal explosion VFX.
- Win condition: enemy count == 0.

**HUD**
- Dear ImGui overlay after the scene. HP (red), shield (blue), boost (yellow) bars. Centered crosshair. Weapon indicator + cooldown timers. Overlays: "YOU WIN" / "YOU DIED".

---

## 4. Core assumptions

- Dear ImGui is initialized and rendered by the **renderer** workstream; the game only issues `ImGui::Begin/End`-level widgets inside a per-frame UI callback.
- Ship orientation is stored as a **quaternion** on `Transform`; pitch/yaw are applied as local-axis rotations composed into the quaternion each tick. Stored Euler angles drift and gimbal.
- Weapon cooldowns use **next-fire-time timestamps**, not per-tick decrement — avoids dt-accumulation error.
- Plasma projectile collision is handled by engine physics (projectile has `RigidBody + Collider`); the game reacts via collision event or per-tick hits query — exact mechanism per the frozen engine interface.

---

## 5. Decision rules

- **Prefer playable demo over any internal refactor.** Every change leaves `game` end-to-end runnable.
- **All tuning values live in a single `tuning.h` header** that mirrors `GAME_DESIGN.md`. Damage, cooldowns, speeds, masses, sizes, smoothing constants — **never** scatter magic numbers across systems.
- **Prefer quaternion-composed orientation** over stored Euler angles. Normalize periodically.
- **Prefer exponential smoothing `1 - exp(-k*dt)` for camera follow** over linear lerp with fixed t. Framerate-independent.
- **Prefer "next-fire-time" cooldowns** over "remaining-cd decremented each frame" — timestamp math is free of dt accumulation error.
- **Prefer kinetic-energy-scaled collision damage** over flat damage. Small taps must not hurt.
- **Prefer speed-capped reflections** for asteroids. Fixed rule, not a tuning knob.
- **Prefer LMB-gated mouse-look.** Always-on breaks ImGui interaction and Freelancer-feel.
- **Prefer destroying entities at end-of-tick** over mid-iteration destruction. Structural changes invalidate `entt` views.
- **Escalate (do not resolve unilaterally):** any request to add audio / networking / particles / post-processing / power-ups ahead of MVP; any frozen-interface change; any tuning value disagreeing with `GAME_DESIGN.md`; any proposal to put game-layer components in the engine schema.

---

## 6. Gotchas

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

---

## 7. Validation

Before completing a game task:

1. **Build:** `cmake --build build --target game` returns 0 with real upstream. If mock build flags are used, both configurations must succeed.
2. **Tests:** relevant Catch2 cases pass — damage pipeline order, cooldown timestamp math, containment reflection, kinetic-energy damage curve. Flight/HUD feel is not unit-tested.
3. **Behavioral:** `game` visibly demonstrates the feature end-to-end — the player can walk the scenario without restarting the binary to recover from half-broken state.
4. **Restart:** pressing Enter from `Dead`/`Won` returns to a fresh `Playing` with no stale entities.
5. **Interfaces unchanged:** no edit to `docs/interfaces/engine-interface-spec.md` or `docs/interfaces/renderer-interface-spec.md`. If a change seems needed, escalate.
6. **Tuning fidelity:** tuning numbers match `GAME_DESIGN.md` / `tuning.h`; gaps are logged as Open Questions.

If any check fails, flag in the appropriate tracking document. Do not merge.

---

## 8. File-loading rules

- **Always:** `AGENTS.md` (auto-loaded), this skill, `docs/game-design/GAME_DESIGN.md`, frozen `docs/interfaces/{engine,renderer}-interface-spec.md`.
- **Flight / camera:** `.agents/skills/entt-ecs/SKILL.md` for ECS iteration patterns; standard 3D-math references for quaternion composition and slerp. No engine internals.
- **Weapons / AI:** frozen engine interface for `raycast`, `overlap_aabb`, input polling; frozen renderer interface for line drawing.
- **HUD:** Dear ImGui upstream reference quoted minimally for the widgets in use.
- **ECS consumer patterns:** `.agents/skills/entt-ecs/SKILL.md` + references (views; deferred destruction).
- **Actual source files over skill prescriptions.** Read `src/game/` directly for current implementation — the code is the source of truth, not this skill's description.
- **Raw engine or renderer internals:** never. Consume only the frozen interfaces; escalate if insufficient.

---

## Companion files

- `AGENTS.md` — global rules; this skill is a specialization.
- `.agents/skills/systems-architect/SKILL.md` — escalation path for any interface, schema, or build-topology decision.
- `.agents/skills/engine-specialist/SKILL.md` — peer; coordinate on input routing, physics query shape, raycast semantics.
- `.agents/skills/renderer-specialist/SKILL.md` — peer; coordinate on line rendering, alpha blending, ImGui integration.
- `.agents/skills/entt-ecs/` — distilled entt API + per-aspect references (consumer-side).
- `docs/game-design/GAME_DESIGN.md` — authoritative tuning numbers.

---

## Evolution

After new feature development via research+plan documents, revisit this skill to:
- Update Confirmed facts if gameplay mechanics change (e.g., asteroid count, enemy scaling).
- Add gotchas derived from new bug discoveries.
- Remove decision rules that are no longer relevant as the codebase matures.
- Consider splitting into implementation and review variants once sufficient code history exists.
