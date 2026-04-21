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
- Asteroid field inside a spherical containment boundary
- Physics-driven asteroid motion, collisions, containment reflection
- Two weapons: **laser (railgun)** and **plasma gun**, switchable
- Small number of enemy ships with simple "seek + shoot" behavior (AI lives in game code for MVP; engine's E-M5 steering is a later swap-in)
- HP + shield + boost resources on player; HP + shield on enemies
- ImGui HUD: HP / shield / boost bars + crosshair
- Restart flow: Enter → reset, Esc → quit, death → auto-restart
- Win condition: all enemies destroyed

**Desirable (if time):**
- Swap game-local AI for engine E-M5 steering
- Explosion VFX via renderer R-M5 custom shader hook
- Laser hit-flash on asteroid surface
- Secondary enemy ship variant
- Tuning pass on game feel (accelerations, damping, FOV, camera lag)

**Stretch (cut unless MVP lands with headroom):**
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
- **Shield (0–100):** drained first by damage. Very slow passive regen (e.g., +2/s after 3s of no damage — SpecKit tunes).
- **Boost (0–100):** Space bar toggles 2× forward thrust. Drains over 5s when continuously used; regenerates over 20s (2× regen rate relative to drain is the design intent — see open decision below if numbers conflict).
- **Collision damage:** proportional to relative kinetic energy against asteroid mass; small taps do nothing, ramming a big rock hurts.

---

## Enemies (MVP)

- 1–3 enemy ships in scene (start with 1 for T+early; bump to 3 when stable).
- Same HP + shield as player; no boost.
- Simple game-local AI: steer toward player at fixed speed, fire plasma when within range + line-of-sight (raycast check).
- Not tied to engine E-M5 for MVP — the homing vector is a trivial few lines of game code; engine's steering API becomes a drop-in replacement when available.
- On death: entity despawned. Explosion VFX is desirable, not MVP (MVP = just "disappear").
- Win condition: enemy count == 0.

---

## Weapons

Only one weapon active at a time. Q switches to plasma, E to laser. Right mouse button fires.

### Laser / railgun (MVP)
- Instant hit via engine `raycast` from ship forward vector.
- Damage: medium. Deals damage to first hit entity (enemy or asteroid).
- Cooldown: 10s.
- Visual: `enqueue_line` from muzzle to hit point (or to max distance if miss), color-flashing + fading over ~0.5s. Depends on renderer R-M3 lines.
- Small asteroid hits add a modest impulse along the ray direction.
- Default load-out: 1 laser gun.

### Plasma gun (MVP)
- Projectile entity: small glowing sphere mesh, rigid body with forward velocity.
- Damage: low per hit. Travels fast but not instantaneous → fast enemies can sidestep.
- Cooldown: short (e.g., 0.1s between shots) — automatic fire while button held.
- Projectile lifetime: despawn after N seconds or on collision with any collider.
- Default load-out: 1 plasma gun (single barrel).

### Rocket launcher (stretch)
- Homing missile: projectile that steers toward player-or-enemy target. Mid cooldown, high damage, large AoE on detonation.
- Requires engine E-M5 (steering) and likely renderer R-M5 (explosion shader) — explicitly stretch.

---

## Asteroid Field Dynamics

- N asteroids (MVP target: 50–150, SpecKit tunes for frame budget), procedurally placed inside the containment sphere.
- Size varies (small / medium / large); mass scales with size.
- Each asteroid has linear + angular velocity. Smaller asteroids start faster.
- Collisions between asteroids and between asteroids+ship are handled by engine E-M4 elastic response.
- Weapons can push asteroids: laser imparts a small impulse along the ray; plasma hits impart tiny impulses; explosions (stretch) apply radial impulses. Large asteroids are ~unmovable by small weapons because mass dominates.
- **Energy containment rule:** to prevent unbounded speed growth from reflections, cap asteroid linear speed at a configured maximum on each reflection event (replaces the "damper objects" idea from the original draft). Simple, cheap, invisible.

## Containment Field

- Spherical, radius ~1 km (SpecKit tunes).
- Visual: optional faint translucent sphere when the ship is near the boundary; MVP can omit visualization entirely — reflections are felt, not seen.
- Reflection math: on contact, velocity component along the inward normal is inverted (asteroid) or inverted + damped slightly (ship, no damage).
- Implemented as a game-layer constraint running each physics step over entities with a `ContainedByField` tag. Lives in game code; no special engine support needed.

---

## Controls

Freelancer-style flight with strafe overlay.

- **Mouse move with left mouse hold:** rotate ship (pitch + yaw) — ship turns toward cursor direction. (SpecKit confirms whether this is "cursor-on-screen drags aim" or "mouse delta directly controls rate" — open decision below.)
- **Right mouse:** fire active weapon.
- **WASD:** move+strafe (relative to ship local axes: A/D = left/right strafe, W/S = forward/back, W = forward thrust, accelerate, S = backwards thrust, decelerate, reverse).
- **Space:** boost (while held, doubles forward thrust until boost resource empties).
- **Q / E:** switch weapon (plasma / laser). R is reserved for rocket launcher (stretch).
- **Enter:** restart match.
- **Esc:** quit.

---

## UI

- Dear ImGui overlay, rendered after the scene.
- **Health bar:** red, 0–100 (extendable to 200 if extra-health power-up, stretch).
- **Shield bar:** blue, 0–100.
- **Boost bar:** yellow (light-blue tint if boost power-up active — stretch).
- **Crosshair:** centered reticle; becomes a thicker/colored variant when a raycast from the ship forward intersects an enemy within laser range (nice-to-have, not MVP).
- **Weapon indicator:** small text showing current weapon + cooldown timer.
- **Win / death overlay:** "YOU WIN — press Enter to restart" / "YOU DIED — press Enter to restart".

---

## Milestone Group — MVP

### G-M1 — Flight + Scene + Camera
*Needs: R-M1 (unlit meshes, camera), E-M1 (ECS + scene). Works against engine/renderer mocks.*
*Target: ~45 min.*

- Player ship entity (placeholder primitive or loaded asset if E-M2 is ready).
- Third-person camera rig (camera follows player position with offset and slight lag).
- Asteroid field: static N asteroids procedurally placed (no velocity yet).
- Containment sphere defined (no visual, no enforcement yet).
- Flight controls: mouse-aim, left-click thrust, WASD strafe. Pure kinematic — position += velocity × dt, no collisions yet.
- Boost control wired (2× thrust, no resource drain yet).
- `game` executable runs against real or mocked engine/renderer.

**Expected outcome:** Player can fly through a static asteroid field. Camera follows. Nothing collides, nothing shoots. Demoable only if renderer/engine milestones are synced. Most likely "headless" development during the first hour, asset pipeline most likely not working yet.

### G-M2 — Physics, Containment, Asteroid Dynamics
*Needs: E-M3 (colliders, raycast), E-M4 (physics). Swap mocks for real engine physics when E-M4 merges.*
*Target: ~45 min.*

- Rigid bodies on asteroids and player ship.
- Asteroids given initial linear + angular velocity (size-scaled).
- Ship–asteroid and asteroid–asteroid elastic collisions via engine.
- Containment reflection rule for asteroids and ship.
- Speed cap on asteroid reflections (energy containment).
- Boost resource drain + regen wired; cooldown timing tuned.

**Expected outcome:** Field feels alive — asteroids drift and tumble, bounce off each other and off the containment boundary. Player can ram asteroids and feel the deflection. Boost can be used up. Demo and test: most likely simple visualization works at this point but physics is not yet implemented in game engine.

### G-M3 — Weapons + Enemies + HP/Shield
*Needs: E-M3 raycast (laser), E-M4 physics (plasma projectiles), R-M3 lines (laser visual).*
*Target: ~1h 15 min.*

- HP + shield components on player and enemies; damage pipeline (damage → shield → HP → death).
- Laser weapon: raycast, line visual with fade, cooldown, damage on hit, impulse on small asteroids.
- Plasma weapon: projectile entity with velocity + collider, spawns on fire, despawns on collision or lifetime, damages on impact.
- Weapon switch (Q/E).
- Enemy AI (game-local): seek player, fire plasma when in range and line-of-sight (raycast check). 1 enemy initially; scale to 3 if stable.
- Enemy death despawns the entity.
- Player death flag set when HP hits 0 (restart flow in G-M4).

**Expected outcome:** Full combat loop. Player can destroy enemies; enemies can damage and kill the player. Weapons feel distinct. Death is detected but not yet handled.

### G-M4 — HUD + Game Flow + Restart
*Needs: ImGui integration (confirm owner in cross-workstream deps below).*
*Target: ~45 min.*

- ImGui HUD: HP bar, shield bar, boost bar, weapon indicator with cooldown.
- Crosshair (fixed center for MVP; enemy-highlight on lock is nice-to-have).
- Restart flow: on Enter (always), on death (auto after ~2s), on win (auto after ~3s).
- Win condition: enemy count reaches 0 → "YOU WIN" overlay.
- Death overlay: "YOU DIED" with Enter prompt.
- Esc quits.

**Expected outcome:** **Game MVP complete.** Full demo loop: start → fly → fight → win or die → restart. Playable for the rehearsal at T-1h.

---

## Milestone Group — Desirable

### G-M5 — AI Upgrade
- Swap game-local seek AI for engine's E-M5 steering (seek + raycast obstacle avoidance). Behavior improves visibly: enemies stop ramming asteroids.

### G-M6 — Explosion VFX
- Requires renderer R-M5 (custom shader + alpha-blended queue).
- Enemy / asteroid destruction triggers an expanding transparent sphere with a plasma-like shader, short lifetime.
- Small radial impulse on nearby asteroids at detonation.

### G-M7 — Feel Tuning Pass
- Tune accelerations, damping, FOV, camera follow lag, weapon cadence, damage values, enemy count, asteroid density for a fun demo.
- Not a coding milestone — a human-driven polish window.

---

## Milestone Group — Stretch

- **Power-ups:** weapon upgrades (2× laser, 4× plasma), HP / shield / extra-variants, boost booster. Pickup sphere meshes with different colors. Multiplies UI and weapon state complexity — only land with clear headroom.
- **Rocket launcher:** homing projectile + AoE detonation. Requires E-M5 and R-M5.
- **Extra enemy variant:** different mesh, different weapon mix.
- **Damper objects:** original design had kinetic-energy-absorbing static objects; replaced in MVP by a simpler speed cap. Re-introduce only if the MVP is solid and there's a visual/design reason.

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

## Shared Ownership Decisions

- **ImGui integration ownership (OPEN):** recommended decision — the **renderer** wires ImGui (init, per-frame begin/end, input bridge) as part of R-M1 or a small side task, and exposes it via its public API. The game then just calls `ImGui::...` inside its tick. This avoids duplicating platform wiring in every consumer. SpecKit to confirm and assign.
- **Ship and asteroid assets:** pre-verified glTF in `/assets/`. Responsibility: human supervisor, pre-hackathon. Fallback: primitives from renderer's mesh builders.
- **Game-local AI vs engine steering:** game owns a minimal seek implementation for MVP; swap to engine's in G-M5.
- **Component schema extensions:** the game adds game-layer components (`WeaponState`, `HealthShield`, `AIController`, `ProjectileTag`, `PlayerTag`, `EnemyTag`, `ContainedByField`) in game code; engine-owned components are not modified.

---

## Resolved Open Decisions

- **Power-ups:** cut from MVP. Stretch only.
- **Rocket launcher:** cut from MVP. Stretch only.
- **Damper objects:** cut. Replaced by asteroid-speed cap on containment reflection.
- **Camera:** third-person; first-person is a fallback if the follow-rig costs too much time.
- **Game-local AI in MVP** (not engine E-M5).
- **Containment visualization:** optional / omitted for MVP.

---

## Open Decisions Still to Make (SpecKit / pre-hackathon)

- **Mouse flight model:** direct-delta control vs cursor-on-screen drag-aim (Freelancer-style). Pick one before G-M1 coding starts.
- **Strafe axis mapping:** A/D = roll vs yaw vs strafe-left/right; W/S = pitch vs forward/back vs strafe-up/down. Pick a specific scheme before G-M1.
> **Human comment** - A/D = strafe-left/right; W/S = forward(acceleration)/back(deceleration/reverse)
- **ImGui ownership:** renderer vs game (recommended: renderer — see above).
- **Numerical tuning targets:** asteroid count, enemy count, damage values, cooldowns, boost timings — tune during G-M7 or earlier if demo looks bad.
> **Human comment** - asteroid count (default to 200, different sizes, will experiment so the field is denser but not tank performance), enemy count (default to 3, don't attack each other, up to 8), damage values (laser 10 dmg, plasma bolt 0.5 dmg, displayed over enemy in screen space, if too hard to do - skip), cooldowns (displayed in HUD), boost timings — OK
- **Asset slate:** which ship mesh(es) + asteroid mesh(es) to pre-verify. Humans decide before T+0.

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
