# Game Workstream Design

> **Status:** Placeholder — to be populated during Game SpecKit planning cycle.  
> **Upstream**: Engine interface spec (`engine-interface-spec.md`, DRAFT v0.1 — must be frozen first). Renderer interface spec (`renderer-interface-spec.md`, FROZEN v1.1). Game design document (`GAME_DESIGN.md`).

---

## Overview

This document will describe the workstream-level design for the game executable: how the game organizes its ECS components, systems, and gameplay logic on top of the engine and renderer public APIs. Populated during the Game SpecKit planning cycle.

---

## Seed Topics (pre-SpecKit)

- Game-layer ECS components: PlayerController, WeaponState, ShieldState, EnemyAI, AsteroidTag, ProjectileTag
- System organization: flight controller, camera rig, weapon systems, damage pipeline, containment logic, HUD rendering
- Integration pattern: game tick within engine tick within renderer frame callback
- Asset requirements: ship model (glTF), asteroid models (glTF), skybox cubemap
- Mock strategy: game starts against `USE_ENGINE_MOCKS=ON` + `USE_RENDERER_MOCKS=ON` at T+0

---

## Cross-References

- Game milestones: `docs/game-design/GAME_DESIGN.md`
- Game architecture: `docs/architecture/game-architecture.md`
- Game interface spec: `docs/interfaces/game-interface-spec.md`
- Game task board: `_coordination/game/TASKS.md`
