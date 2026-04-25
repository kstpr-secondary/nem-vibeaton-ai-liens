# PROGRESS.md — Engine Workstream

> Updated by engine specialist as milestones complete. See `_coordination/overview/PROGRESS.md` for cross-workstream status.

## Milestones

| Milestone | Status | Owner | Notes |
|---|---|---|---|
| E-M1 Bootstrap + ECS + Scene API | DONE | — | Procedural scene rendered through renderer. SC-001, SC-006 met. |
| E-M2 Asset Import | DONE | — | glTF/OBJ loading through `ASSET_ROOT`. SC-002 met. |
| E-M3 Input + AABB Colliders + Raycasting | DONE | — | WASD+mouse FPS camera, 50+ collidable entities, crosshair raycast. Post-E-M3 fix: reversed glTF index winding in `asset_import.cpp` to resolve inside-out asteroid rendering (CW→CCW). SC-004, SC-005 met. |
| E-M4 Euler Physics + Collision Response | TODO | — | Engine MVP; 20+ rigid bodies with elastic collisions |
| E-M5 Enemy Steering AI | TODO | — | Desirable |
| E-M6 Multiple Point Lights | TODO | — | Desirable |
| E-M7 Capsule Mesh Integration | TODO | — | Desirable; depends on renderer R-M5 |

## Recent Changes

| Date | Change |
|---|---|
| 2026-04-26 | E-M3 completed. Added glTF index winding reversal fix in `asset_import.cpp` (line 90: read indices in reverse order to convert CW→CCW). Updated TASKS.md E-023 notes. Clean build verified. |
