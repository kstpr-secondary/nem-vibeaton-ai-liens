# Implementation Plan: Space Shooter MVP

**Branch**: `003-create-feature-branch` | **Date**: 2026-04-23 | **Spec**: [specs/003-space-shooter-mvp/spec.md](./spec.md)
**Input**: Feature specification from `/specs/003-space-shooter-mvp/spec.md`

## Summary

Build a C++17 game executable (`game`) providing a third-person 3D space shooter in a contained asteroid field. The game consumes the engine public API (`engine.h`, frozen v1.1) and transitively the renderer public API (`renderer.h`, frozen v1.1). It owns all gameplay systems: player flight controller, third-person camera rig, 200-asteroid procedural field with spherical containment, two weapons (laser raycast + plasma projectile), HP/shield/boost resources, 1 enemy with seek+shoot AI, minimal explosion VFX, Dear ImGui HUD, and a 4-state match flow (Playing → PlayerDead/Victory → Restarting → Playing). MVP milestones G-M1 through G-M4 must land within ~3.5 hours; desirable milestones G-M5 through G-M7 fill remaining hackathon time. The game starts against engine + renderer mocks at T+0 and swaps to real upstream at each milestone merge.

## Technical Context

**Language/Version**: C++17 — `-Wall -Wextra -Wpedantic`, `-Werror` off
**Primary Dependencies**: engine static lib (frozen v1.1), renderer static lib (frozen v1.1), GLM 1.x (vec/mat/quat math), Dear ImGui (renderer-owned `util/sokol_imgui.h` — game only builds widgets), entt v3.x (registry access via `engine_registry()`)
**Storage**: N/A — all state lives in ECS component pools (via engine) and game-local structs; no disk persistence
**Testing**: Human behavioral check on `game` executable for all gameplay correctness; no Catch2 `game_tests` target (game layer is primarily integration/behavioral, not unit-testable math)
**Target Platform**: Ubuntu 24.04 LTS, desktop Linux; OpenGL 3.3 Core via renderer
**Project Type**: Executable (`game`) — consumes engine + renderer static libs
**Performance Goals**: ≥30 FPS with 200 asteroids + 1 enemy + active combat on target hardware; plasma projectile spawn/despawn must not cause visible frame hitches
**Constraints**: ~3.5h MVP window (G-M1→G-M4); game-local AI (no engine E-M5 steering for MVP); single directional light; no audio/networking/particles/post-processing
**Scale/Scope**: 200 asteroids, 1 player ship, 1 enemy ship (MVP), ≤50 active projectiles, 1 camera, 1 directional light, 7 HUD elements

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Gate | Status | Evidence |
|------|--------|---------|
| **I. Behavioral Correctness** | ✅ PASS | All 14 FRs map to acceptance scenarios in 3 prioritized user stories; each milestone gated by human behavioral check on `game` executable; edge cases (simultaneous death, cooldown overlap, containment fast objects) explicitly resolved |
| **II. Milestone-Driven Integration** | ✅ PASS | G-M1→G-M4 are discrete sync points; game starts against mocks (`USE_ENGINE_MOCKS=ON`, `USE_RENDERER_MOCKS=ON`) and swaps at milestone merges; frozen interfaces consumed as-is |
| **III. Speed Over Maintainability** | ✅ PASS | Game-local AI (trivial seek vector, no behavior tree), inline damage pipeline, no component inheritance, no event bus, no state-machine framework — raw `if/else` match flow |
| **IV. AI-Generated Under Human Supervision** | ✅ PASS | All C++ AI-generated; human approves milestones, runs behavioral checks, signs off on each merge to `main` |
| **V. Minimal Fixed Stack** | ✅ PASS | Zero new libraries; game consumes only engine + renderer public APIs + Dear ImGui widgets (renderer-owned); all deps already in FetchContent |

## Project Structure

### Documentation (this feature)

```text
specs/003-space-shooter-mvp/
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output
├── contracts/           # Phase 1 output → promoted to docs/interfaces/game-interface-spec.md
└── tasks.md             # Phase 2 output (speckit.tasks command)
```

### Source Code (repository root)

```text
src/game/
├── main.cpp                  # Entry point: renderer_init → set frame/input callbacks → renderer_run
├── game.h                    # Public game API: game_init(), game_tick(dt), game_shutdown()
├── game.cpp                  # Game lifecycle, match state machine (Playing/PlayerDead/Victory/Restarting)
├── player.h                  # Player flight controller, boost, resource state
├── player.cpp
├── camera_rig.h              # Third-person camera follow with offset + lag
├── camera_rig.cpp
├── weapons.h                 # Weapon definitions (laser/plasma), cooldown tracking, fire logic
├── weapons.cpp
├── projectile.h              # Plasma projectile spawn, lifetime, collision despawn
├── projectile.cpp
├── damage.h                  # Damage pipeline: collision damage, weapon damage, shield/HP resolution
├── damage.cpp
├── enemy_ai.h                # Game-local seek + shoot AI for enemy ships
├── enemy_ai.cpp
├── asteroid_field.h          # Procedural asteroid placement, containment reflection, speed capping
├── asteroid_field.cpp
├── hud.h                     # Dear ImGui HUD: bars, crosshair, weapon indicator, overlays
├── hud.cpp
├── components.h              # Game-layer ECS components (PlayerTag, EnemyTag, ProjectileTag, etc.)
├── spawn.h                   # Entity factory: spawn_player, spawn_enemy, spawn_asteroid, spawn_projectile
├── spawn.cpp
└── constants.h               # Tuning constants: speeds, damage values, cooldowns, field radius
```

**Structure Decision**: Single C++17 executable workstream following the existing repo layout at `src/game/`. The game depends on engine + renderer only through their frozen public headers. No engine or renderer source is touched by this workstream. Game-layer ECS components are defined in `components.h` and attached to entities owned by the engine registry.

## Complexity Tracking

> No constitution violations — table omitted.
