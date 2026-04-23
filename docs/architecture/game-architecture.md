# Game Architecture

> **Status:** Placeholder — to be populated after Game SpecKit completion.  
> **Upstream**: Engine architecture (`engine-architecture.md`), Renderer architecture (`renderer-architecture.md`, FROZEN).

---

## Overview

The game is a C++17 executable providing a 3D space shooter built on the engine and renderer public APIs. It owns all gameplay systems: player flight controller, third-person camera rig, asteroid field + containment, weapon systems (laser raycast + plasma projectile), HP/shields/boost resources, enemy AI, Dear ImGui HUD, and restart/win/quit flow.

---

## Ownership and Boundaries

- **Game owns**: player flight controller, third-person camera rig, asteroid field + containment, weapon systems, HP/shields/boost, enemy AI, Dear ImGui HUD widgets, restart/win/quit flow, game-layer ECS components.
- **Game does not own**: renderer internals (shaders, sokol_app init, pipeline creation), engine ECS internals (registry ownership, physics stepping, collision detection).
- **Game consumes**: `engine.h` and `renderer.h` public APIs only.

---

## Module Map

> To be populated during Game SpecKit planning cycle.

---

## Milestones

| ID | Name | Expected Outcome |
|----|------|-----------------|
| G-M1 | Flight + Scene + Camera | Player can fly through 200 static asteroids; camera follows |
| G-M2 | Physics, Containment, Asteroid Dynamics | Field feels alive; asteroids drift; ship collides and bounces |
| G-M3 | Weapons + Enemies + HP/Shield | Full combat loop with 1 enemy; weapons feel distinct |
| G-M4 | HUD + Game Flow + Restart | **Game MVP complete**; full demo loop |
| G-M5 | AI Upgrade & Scaling | Improved enemy AI; scale to 3–8 enemies |
| G-M6 | Visual Polish | Shader VFX, laser hit-flash |
| G-M7 | Feel Tuning Pass | Accelerations, damping, FOV, camera lag tuning |
