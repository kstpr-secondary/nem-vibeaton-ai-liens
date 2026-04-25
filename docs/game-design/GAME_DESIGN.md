# Game Design Document

> **Status:** Updated 2026-04-26 with Game SpecKit clarifications.
> **Upstream**: Engine interface (`engine-interface-spec.md`, **FROZEN v1.2**). Renderer interface (`renderer-interface-spec.md`, **FROZEN v1.1**).
> **SpecKit source**: `docs/planning/speckit/game/spec.md` (clarifications session 2026-04-23).

---

## Concept

A third-person 3D space shooter in a contained asteroid field. Freelancer-inspired flight feel and shooting mechanics with Quake 3 inspired weapon and power-up system, scaled to hackathon MVP. Built on the game engine's public API and (transitively) the renderer's. The player pilots a ship through a tumbling asteroid field, fights a small number of enemy ships, and wins when all enemies are destroyed.

The game runs against engine + renderer mocks at T+0 so the game workstream never idles on upstream delivery. Each milestone leaves the game in a playable, demoable state.

**Core loop:** fly → shoot enemies → avoid asteroids & incoming fire → survive → win when field is cleared. Death restarts the match.

---

## SpecKit Clarifications (2026-04-23)

These three points override any earlier ambiguity in the seed concept and are part of the freeze contract:

1. **Match state machine** — Four states only: `Playing → PlayerDead/Victory → Restarting → Playing`. Terminal phases (`PlayerDead`, `Victory`) display an overlay and run an auto-restart countdown; `Restarting` clears entities and rebuilds the scene before transitioning back to `Playing`. Manual restart skips the countdown.
2. **Simultaneous death** — If the last enemy and the player both reach 0 HP in the same tick, the match transitions to **Victory**. Enemy deaths are evaluated before player death within `match_state_update` so the win condition wins.
3. **Collision damage threshold** — None. Every collision deals damage proportional to relative kinetic energy. Slow taps deal sub-1 damage (effectively masked by shield regen); hard rams deal significant damage. There is no minimum cutoff to tune.

---

## Scope

### MVP (must ship)

- Player ship with third-person camera (first-person fallback if rig time overruns)
- Asteroid field (200 asteroids) inside a spherical containment boundary
- Physics-driven asteroid motion, ship-asteroid collisions, containment reflection
- Two weapons: **laser (railgun)** and **plasma gun**, switchable (Q/E)
- One enemy ship with simple "seek + shoot" behavior (AI lives in game code for MVP)
- HP + shield + boost resources on player; HP + shield on enemy
- Minimal explosion VFX (simple primitive flash or expanding sphere with basic alpha blending) on death
- ImGui HUD: HP / shield / boost bars + crosshair + weapon cooldowns
- Restart flow: Enter → reset, Esc → quit, death → auto-restart
- Win condition: all enemies destroyed

### Desirable (if time)

- Asteroid-asteroid collisions (elastic response)
- Scale enemy count from 1 to 3 (up to 8 for demo)
- Swap game-local AI for engine E-M5 steering
- Explosion VFX via renderer R-M5 custom shader hook
- Laser hit-flash on asteroid surface
- Tuning pass on game feel (accelerations, damping, FOV, camera lag)

### Stretch (cut unless MVP lands with headroom)

- Screen-space damage numbers
- Power-ups (entire system)
- Rocket launcher (homing missiles)
- Multiple asteroid generation patterns / clusters

### Cut entirely

- Audio (blueprint-wide cut)
- Networking, particles, post-processing (blueprint-wide cut)

---

## Player Ship

- Mesh: pre-verified glTF asset (or primitive placeholder if asset import slips)
- Starting state: center of field, max HP (100), max shield (100), full boost (100)
- **HP (0–100):** drops only when shield is 0. If HP hits 0 → death → auto-restart
- **Shield (0–100):** drained first by damage. Slow passive regen (+2/s after 3s of no damage)
- **Boost (0–100):** Space bar toggles 2× forward thrust. Drains over 5s; regenerates over 20s
- **Collision damage:** proportional to relative kinetic energy against asteroid mass

---

## Enemies (MVP)

- 1 enemy ship in scene for MVP (scale to 3–8 in Desirable/Polish)
- Same HP + shield as player; no boost
- Simple game-local AI: steer toward player at fixed speed, fire plasma when within range + line-of-sight
- Enemies do not attack each other
- On death: entity despawned + minimal explosion VFX
- Win condition: enemy count == 0

---

## Weapons

Only one weapon active at a time. Q switches to plasma, E to laser. Right mouse button fires.

### Laser / railgun (MVP)

- Instant hit via engine `raycast` from ship forward vector
- Damage: 10. Deals damage to first hit entity (enemy or asteroid)
- Cooldown: 5s
- Visual: `enqueue_line_quad` from muzzle to hit point, color-flashing + fading over ~0.5s (requires R-M3 lines)
- Small asteroid hits add a modest impulse along the ray direction

### Plasma gun (MVP)

- Projectile entity: small glowing sphere mesh, rigid body with forward velocity
- Damage: 0.5 per hit. Travels fast but not instantaneous
- Cooldown: 0.1s between shots — automatic fire while button held
- Projectile lifetime: despawn after N seconds or on collision
- Default load-out: 1 plasma gun (single barrel)

---

## Asteroid Field Dynamics

- 200 asteroids (different sizes), procedurally placed inside the containment sphere
- Size varies (small / medium / large); mass scales with size
- Each asteroid has linear + angular velocity. Smaller asteroids start faster
- Ship–asteroid collisions handled by engine E-M4
- Weapons can push asteroids: laser imparts a small impulse; plasma tiny impulses
- **Energy containment rule:** cap asteroid linear speed at a configured maximum on each reflection event

## Containment Field

- Spherical, radius ~1 km
- Reflection math: velocity component along inward normal is inverted (asteroid) or inverted + damped slightly (ship, no damage)
- Implemented as game-layer constraint

---

## Controls

Freelancer-style flight with strafe overlay.

| Input | Action |
|---|---|
| Mouse move + left hold | Rotate ship (pitch + yaw) |
| Right mouse | Fire active weapon |
| W/S | Forward thrust / back |
| A/D | Strafe left / right |
| Space | Boost (while held) |
| Q/E | Switch weapon (plasma / laser) |
| Enter | Restart match |
| Esc | Quit |

---

## UI (Dear ImGui HUD)

- Health bar: red, 0–100
- Shield bar: blue, 0–100
- Boost bar: yellow
- Crosshair: centered reticle
- Weapon indicator: current weapon + cooldown timers
- Win / death overlay: "YOU WIN" / "YOU DIED" + Enter to restart

---

## Milestones

| ID | Name | Scope | Status |
|---|---|---|---|
| G-M1 | Flight + Scene + Camera | Player ship, camera rig, 200 static asteroids, flight controls | TODO |
| G-M2 | Physics, Containment, Asteroid Dynamics | Rigid bodies, initial velocities, containment reflection, boost | TODO |
| G-M3 | Weapons + Enemies + HP/Shield | Combat loop, 1 enemy, laser + plasma, damage pipeline | TODO |
| G-M4 | HUD + Game Flow + Restart | ImGui HUD, win/death, restart. **MVP complete** | TODO |
| G-M5 | AI Upgrade & Scaling | Swap to engine steering, scale enemies | Desirable |
| G-M6 | Visual Polish | Shader VFX, laser hit-flash | Desirable |
| G-M7 | Feel Tuning Pass | Accelerations, damping, FOV, camera lag | Desirable |

---

## Cross-Workstream Dependencies

| Game milestone | Requires renderer at | Requires engine at |
|---|---|---|
| G-M1 | R-M1 (unlit meshes, camera) | E-M1 (ECS + scene) |
| G-M2 | R-M1 | E-M4 (physics + collision response) |
| G-M3 | R-M1 + R-M3 (lines) | E-M3 (raycast) + E-M4 (physics) |
| G-M4 | R-M1 + renderer-owned `sokol_imgui` | E-M1 |
| G-M5 | — | E-M5 (steering) |
| G-M6 | R-M5 (custom shader + alpha) | E-M4 |

At Game SpecKit promotion (2026-04-26) renderer R-M1..R-M3 and engine E-M1..E-M4 are all delivered (renderer FROZEN v1.1; engine FROZEN v1.2), so the game runs end-to-end against real upstream for the entire MVP. Mocks remain available via `-DUSE_RENDERER_MOCKS=ON` / `-DUSE_ENGINE_MOCKS=ON` as a demo-day fallback if any upstream regresses.

---

## Cross-References

- Game architecture: `docs/architecture/game-architecture.md`
- Game interface (procedural API contract): `docs/interfaces/game-interface-spec.md`
- Game workstream design: `docs/game-design/game-workstream-design.md`
- Game tasks (live): `_coordination/game/TASKS.md`
- Game SpecKit artifacts: `docs/planning/speckit/game/`
