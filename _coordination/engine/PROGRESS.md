# PROGRESS.md — Engine Workstream

> Updated by engine specialist as milestones complete. See `_coordination/overview/PROGRESS.md` for cross-workstream status.

## Milestones

| Milestone | Status | Owner | Notes |
|---|---|---|---|
| E-M1 Bootstrap + ECS + Scene API | DONE | — | Procedural scene rendered through renderer. SC-001, SC-006 met. |
| E-M2 Asset Import | DONE | — | glTF/OBJ loading through `ASSET_ROOT`. SC-002 met. |
| E-M3 Input + AABB Colliders + Raycasting | DONE | — | WASD+mouse FPS camera, 50+ collidable entities, crosshair raycast. Post-E-M3 fix: reversed glTF index winding in `asset_import.cpp` to resolve inside-out asteroid rendering (CW→CCW). SC-004, SC-005 met. |
| E-M4 Euler Physics + Collision Response | DONE | — | Engine MVP; 20+ rigid bodies with elastic collisions. Energy conservation within 5%. SC-003, SC-007, SC-008 met. |
| E-M5 Enemy Steering AI | TODO | — | Desirable |
| E-M6 Multiple Point Lights | TODO | — | Desirable |
| E-M7 Capsule Mesh Integration | TODO | — | Desirable; depends on renderer R-M5 |
| DEMO-SCENE Stress Test + Collision Viz | DONE | claude@laptopA | E-036: renderer_get_triangle_count(). E-037: CollisionFlash component. E-038: collision event hook. E-039: render-loop blending. E-040: 10 static glTF asteroids (4 models, varied scales/colors). E-041: ImGui HUD (FPS/draws/tris/entities). E-042: stress-test spawner (max 200 entities, ramps difficulty). |

## Recent Changes

| Date | Change |
|---|---|
| 2026-04-26 | E-M3 completed. Added glTF index winding reversal fix in `asset_import.cpp` (line 90: read indices in reverse order to convert CW→CCW). Updated TASKS.md E-023 notes. Clean build verified. |
| 2026-05-01 | DEMO-SCENE complete: E-037 (CollisionFlash component + engine_on_collision API), E-038 (collision event hook in collision_response.cpp with v_rel_n guard), E-039 (render-loop blending with 2-segment curve), E-040 (10 static glTF asteroids from 4 models with varied scales/colors), E-041 (ImGui HUD via sokol_imgui: FPS/draws/tris/entities), E-042 (stress-test spawner, max 200 entities, ramps difficulty). Build verified: engine_app + engine_tests compile clean, 118 assertions / 51 test cases pass. |
| 2026-05-01 | Crash fix: `engine_on_collision()` called `emplace<CollisionFlash>` which crashed when the same entity collided with multiple entities in one frame (double emplace assertion). Added `all_of<CollisionFlash>()` guard to skip if already flashed. Also removed duplicate simgui calls from engine_app (renderer already handles simgui_new_frame/render in begin/end_frame). |
