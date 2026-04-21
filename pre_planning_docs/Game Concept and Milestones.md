# Game — Concept, MVP, Milestones

Seed document for Game SpecKit. Concise but complete. Runs *after* the renderer and engine interface specs are frozen. Supersedes the Game Concept section of `Milestones_MVPs_Concepts.md` with the pre-cuts from `Milestones Feasibility Review.md` applied.

---

## Concept

A third-person 3D space shooter in a contained asteroid field. Freelancer (2003 game) inspired flight feel and shooting mechanics with Quake 3 inspired weapon and power-ups system, scaled to hackathon-MVP. Built on the game engine's public API and (transitively) the renderer's. The player pilots a ship through a tumbling asteroid field, fights a small number of enemy ships, and wins when all enemies are destroyed.

The game fully runs against engine + renderer mocks at T+0 so the game workstream never idles on upstream delivery. Each milestone leaves the game in a playable, demoable state.

**Core loop:** fly → shoot enemies → avoid asteroids & incoming fire → survive → win when field is cleared. Death restarts the match.

---

## Scope Summary

**MVP (must ship):**
- Player ship with third-person camera (first-person fallback if rig time overruns)
- Asteroid field (200 asteroids) inside a spherical containment boundary
- Physics-driven asteroid motion, ship-asteroid collisions, containment reflection
- Two weapons: **laser (railgun)** and **plasma gun**, switchable (Q/E)
- One enemy ship with simple "seek + shoot" behavior (AI lives in game code for MVP)
- HP + shield + boost resources on player; HP + shield on enemy
- **Minimal explosion VFX** (simple primitive flash or expanding sphere with basic alpha blending) on death. Shader-based VFX deferred to Desirable (requires renderer R-M5).
- ImGui HUD: HP / shield / boost bars + crosshair + weapon cooldowns
- Restart flow: Enter → reset, Esc → quit, death → auto-restart
- Win condition: all enemies destroyed

**Desirable (if time):**
- Asteroid-asteroid collisions (elastic response)
- Scale enemy count from 1 to 3 (up to 8 for demo)
- Swap game-local AI for engine E-M5 steering
- Explosion VFX via renderer R-M5 custom shader hook
- Laser hit-flash on asteroid surface
- Tuning pass on game feel (accelerations, damping, FOV, camera lag)

**Stretch (cut unless MVP lands with headroom):**
- Screen-space damage numbers (displayed over enemies)
- Power-ups (entire system)
- Rocket launcher (homing missiles)
- Multiple asteroid generation patterns / clusters
- Secondary weapons feedback (screenshake, hit markers)

**Cut entirely:**
- Damper objects (instead: cap asteroid speed at containment reflection to prevent energy growth)
- Audio (blueprint-wide cut)
- Networking, particles, post-processing (blueprint-wide cut)

---

## Player Ship

- Mesh: pre-verified glTF asset (or primitive placeholder if asset import slips).
- Starting state: center of field, max HP (100), max shield (100), full boost (100).
- **HP (0–100):** drops only when shield is 0. If HP hits 0 → death → auto-restart.
- **Shield (0–100):** drained first by damage. Very slow passive regen (e.g., +2/s after 3s of no damage).
- **Boost (0–100):** Space bar toggles 2× forward thrust. Drains over 5s when continuously used; regenerates over 20s.
- **Collision damage:** proportional to relative kinetic energy against asteroid mass; small taps do nothing, ramming a big rock hurts.

---

## Enemies (MVP)

- 1 enemy ship in scene for MVP (scale to 3-8 in Desirable/Polish).
- Same HP + shield as player; no boost.
- Simple game-local AI: steer toward player at fixed speed, fire plasma when within range + line-of-sight (raycast check).
- Enemies do not attack each other.
- Not tied to engine E-M5 for MVP — the homing vector is a trivial few lines of game code.
- On death: entity despawned + minimal explosion VFX.
- Win condition: enemy count == 0.

---

## Weapons

Only one weapon active at a time. Q switches to plasma, E to laser. Right mouse button fires.

### Laser / railgun (MVP)
- Instant hit via engine `raycast` from ship forward vector.
- Damage: 10. Deals damage to first hit entity (enemy or asteroid).
- Cooldown: 5s.
- Visual: `enqueue_line` from muzzle to hit point (or to max distance if miss), color-flashing + fading over ~0.5s. Depends on renderer R-M3 lines.
- Small asteroid hits add a modest impulse along the ray direction.
- Default load-out: 1 laser gun.

### Plasma gun (MVP)
- Projectile entity: small glowing sphere mesh, rigid body with forward velocity.
- Damage: 0.5 per hit. Travels fast but not instantaneous → fast enemies can sidestep.
- Cooldown: short (e.g., 0.1s between shots) — automatic fire while button held.
- Projectile lifetime: despawn after N seconds or on collision with any collider.
- Default load-out: 1 plasma gun (single barrel).

### Rocket launcher (stretch)
- Homing missile: projectile that steers toward player-or-enemy target. Mid cooldown, high damage, large AoE on detonation.
- Requires engine E-M5 (steering) and likely renderer R-M5 (explosion shader) — explicitly stretch.

---

## Asteroid Field Dynamics

- 200 asteroids (different sizes), procedurally placed inside the containment sphere.
- Size varies (small / medium / large); mass scales with size.
- Each asteroid has linear + angular velocity. Smaller asteroids start faster.
- Ship–asteroid collisions handled by engine E-M4. (Asteroid–asteroid collisions are Desirable).
- Weapons can push asteroids: laser imparts a small impulse; plasma tiny impulses. Large asteroids are ~unmovable by small weapons because mass dominates.
- **Energy containment rule:** cap asteroid linear speed at a configured maximum on each reflection event.

## Containment Field

- Spherical, radius ~1 km (SpecKit tunes).
- Visual: optional faint translucent sphere when near boundary (MVP can omit).
- Reflection math: velocity component along inward normal is inverted (asteroid) or inverted + damped slightly (ship, no damage).
- Implemented as game-layer constraint.

---

## Controls

Freelancer-style flight with strafe overlay.

- **Mouse move with left mouse hold:** rotate ship (pitch + yaw) — ship turns toward cursor direction.
- **Right mouse:** fire active weapon.
- **WASD:**
    - **W/S:** forward (acceleration) / back (deceleration/reverse)
    - **A/D:** strafe-left / strafe-right
- **Space:** boost (while held, doubles forward thrust until boost resource empties).
- **Q / E:** switch weapon (plasma / laser).
- **Enter:** restart match.
- **Esc:** quit.

---

## UI

- Dear ImGui overlay, rendered after the scene.
- **Health bar:** red, 0–100.
- **Shield bar:** blue, 0–100.
- **Boost bar:** yellow.
- **Crosshair:** centered reticle.
- **Weapon indicator:** current weapon + 5s/0.1s cooldown timers.
- **Win / death overlay:** "YOU WIN" / "YOU DIED" + Enter to restart.

---

## Milestone Group — MVP

### G-M1 — Flight + Scene + Camera
*Needs: R-M1 (unlit meshes, camera), E-M1 (ECS + scene). Works against engine/renderer mocks.*
*Target: ~45 min.*

- Player ship entity.
- Third-person camera rig (camera follows player position with offset and slight lag).
- Asteroid field: 200 static asteroids procedurally placed.
- Flight controls (W/S accel, A/D strafe, Mouse pitch/yaw).
- Boost control wired.
- `game` executable runs against real or mocked engine/renderer.

**Expected outcome:** Player can fly through a static field of 200 asteroids. Camera follows. Nothing collides, nothing shoots. Demoable only if renderer/engine milestones are synced. Most likely "headless" development during the first hour, asset pipeline most likely not working yet.

**Contingency:** If third-person camera rig stalls beyond T+1h, switch to first-person (camera entity as child of player entity with no offset). Document decision in TASKS.md and notify Person A. First-person requires ~5 min of code changes; third-person rig can take 30–45 min.

### G-M2 — Physics, Containment, Asteroid Dynamics
*Needs: E-M3 (colliders, raycast), E-M4 (physics). Swap mocks for real engine physics when E-M4 merges.*
*Target: ~45 min.*

- Rigid bodies on asteroids and player ship.
- Asteroids given initial linear + angular velocity.
- Ship–asteroid and containment reflection rule.
- Аsteroid–asteroid elastic collisions via engine only if almost free in game logic.
- Speed cap on asteroid reflections.
- Boost resource drain + regen wired.

**Expected outcome:** Field feels alive. Asteroids drift, ship collides with them and bounces off boundary. Player can ram asteroids and feel the deflection. Boost can be used up. Demo and test: most likely simple visualization works at this point but physics is not yet implemented in game engine.

### G-M3 — Weapons + Enemies + HP/Shield
*Needs: E-M3 raycast (laser), E-M4 physics (plasma projectiles), R-M3 lines (laser visual).*
*Target: ~1h 15 min.*

- HP + shield components on player and enemies; damage pipeline (10 for laser, 0.5 for plasma).
- Laser weapon: raycast, line visual with fade, 5s cooldown, damage on hit, impulse on small asteroids.
- Plasma weapon: projectile entity, with velocity + collider, 0.1s cooldown, spawns on fire, despawns on collision or lifetime, damages on impact.
- Weapon switch (Q/E).
- Enemy AI (game-local): 1 enemy, seek player, fire plasma.
- Enemy death despawns entity + **minimal explosion VFX**.

**Expected outcome:** Full combat loop with 1 enemy. Laser on 5s cooldown. Player can destroy enemies; enemies can damage and kill the player. Weapons feel distinct. Death is detected but not yet handled.

### G-M4 — HUD + Game Flow + Restart
*Needs: ImGui integration.*
*Target: ~45 min.*

- ImGui HUD: bars + weapon cooldown indicators.
- Crosshair (fixed center for MVP; enemy-highlight on lock is nice-to-have).

- Restart flow: on Enter (always), on death (auto after ~2s), on win (auto after ~3s).
- Win condition: enemy count reaches 0 → "YOU WIN" overlay.
- Death overlay: "YOU DIED" with Enter prompt.

- Esc quits.

**Expected outcome:** **Game MVP complete.** Full demo loop: start → fly → fight → win or die → restart. Playable for the rehearsal at T-1h.

---

## Milestone Group — Desirable

### G-M5 — AI Upgrade & Scaling
- Swap game-local seek AI for engine's E-M5 steering (seek + raycast obstacle avoidance). Behavior improves visibly: enemies stop ramming asteroids.
- Increase enemy count to 3 (up to 8).

### G-M6 — Visual Polish
- Explosion VFX via renderer R-M5 custom shader hook.
- Laser hit-flash on asteroid surface.

### G-M7 — Feel Tuning Pass
- Tune accelerations, damping, FOV, camera follow lag, weapon cadence, damage values, enemy count, asteroid density for a fun demo..

---

## Milestone Group — Stretch

- **Screen-space damage numbers:** Display dmg values over hit enemies.
- **Power-ups:** weapon upgrades (2× laser, 4× plasma), HP / shield / extra-variants, boost booster. Pickup sphere meshes with different colors. Multiplies UI and weapon state complexity — only land with clear headroom.
- **Rocket launcher:** homing projectile + AoE detonation.
- **Asteroid-asteroid collisions:** Elastic response via game-layer collision detection (engine provides AABB tests). O(n²) with 200 asteroids; only viable if agent finds a trivial optimization or reduces active asteroid count.

---

## Cross-Workstream Dependencies

| Game milestone | Requires renderer at | Requires engine at |
|---|---|---|
| G-M1 | R-M1 (unlit meshes, camera) | E-M1 (ECS + scene) |
| G-M2 | R-M1 | E-M4 (physics + collision response) |
| G-M3 | R-M1 + R-M3 (lines) | E-M3 (raycast) + E-M4 (physics) |
| G-M4 | R-M1 + ImGui integration | E-M1 |
| G-M5 (desirable) | — | E-M5 (steering) |
| G-M6 (desirable) | R-M5 (custom shader + alpha) | E-M4 |

**Before G-M2 merges**, engine E-M4 must be landed **or** the game uses engine mocks with canned physics (stationary asteroids, simple velocity integration). The game workstream is never blocked: mocks cover every dependency.

---


## Notes for SpecKit

- Game workstream starts at T+0 against mocks; swap to real engine/renderer interfaces at each milestone merge.
- Every milestone must leave the `game` executable in a demoable state — never a half-built intermediate.
- Parallel group hints:
  - G-M3: laser raycast+line rendering path and plasma projectile+collision path are file-disjoint → natural parallel group. Enemy AI is a third disjoint effort.
  - G-M4: ImGui HUD panel and restart/win-loss flow are file-disjoint.
- Acceptance per milestone: build passes, game runs end-to-end, human behavioral check confirms the milestone's expected outcome, no regressions in earlier milestones.
- Feature-freeze discipline (blueprint §13.2): at T-30 min, all game feature work stops; only bug fixes and asset tweaks beyond that point.
- Fallback plan (blueprint §13.2): if the game workstream is broken at T-30, re-enable engine+renderer mocks so `game` still launches and shows *something* — even if it's just a fly-through with no combat.


---
