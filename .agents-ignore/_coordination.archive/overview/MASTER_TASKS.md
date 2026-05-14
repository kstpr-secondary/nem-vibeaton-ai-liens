# Master Tasks (compact dashboard)

> Systems Architect's integration view. Per-workstream `TASKS.md` files are authoritative on their feature branches. Updated 2026-04-26 after Game SpecKit promotion.

## Milestone graph (renderer → engine → game)

| Workstream | Milestone | Scope | Status | Unlocks / Notes |
|---|---|---|---|---|
| Renderer | R-M0 | Bootstrap | DONE | Window + clear color + input pump |
| Renderer | R-M1 | Unlit Forward Rendering + Procedural Scene | DONE | Engine + game sync point; renderer FROZEN v1.1 |
| Renderer | R-M2 | Directional Light + Lambertian | DONE | Lit scenes available with no engine API change |
| Renderer | R-M3 | Skybox + Line-Quads + Alpha Blending | DONE | Renderer MVP complete; required for laser visual + plasma alpha |
| Renderer | R-M4 | Blinn-Phong + Diffuse Textures | TODO | Desirable |
| Renderer | R-M5 | Custom Shader Hook + Sorted Transparency | TODO | Desirable; unlocks game shader VFX (G-M6) |
| Engine | E-M1 | Bootstrap + ECS + Scene API | DONE | Game sync point; engine FROZEN |
| Engine | E-M2 | Asset Import (glTF + OBJ) | DONE | Mesh upload bridge |
| Engine | E-M3 | Input + AABB Colliders + Raycasting | DONE | Required for laser, camera control, AI LOS |
| Engine | E-M4 | Euler Physics + Collision Response | DONE | Engine MVP complete (FROZEN v1.2); required for asteroids/projectiles |
| Engine | E-M5 | Enemy Steering AI | TODO | Desirable; G-M5 swap target |
| Engine | E-M6 | Multiple Point Lights | TODO | Desirable |
| Engine | E-M7 | Capsule Mesh Integration | TODO | Desirable; depends on R-M5 |
| Game | SETUP | main.cpp + game.h/.cpp skeleton | TODO | Game workstream entry — G-001..G-002 |
| Game | FOUNDATION | components / constants / spawn / render_submit | TODO | Blocks all G-M*; G-003..G-006 |
| Game | G-M1 | Flight + Scene + Camera | TODO | First demoable scene; G-007..G-010 |
| Game | G-M2 | Physics + Containment + Asteroid Dynamics | TODO | Field comes alive; G-011..G-014 |
| Game | G-M3 | Weapons + Enemies + HP/Shield | TODO | Combat loop; G-015..G-022 |
| Game | G-M4 | HUD + Game Flow + Restart | TODO | Game MVP complete; G-023..G-027 |
| Game | POLISH | Tuning + acceptance + e2e | TODO | G-028..G-030 |
| Game | G-M5 | AI Upgrade & Scaling | TODO | Desirable |
| Game | G-M6 | Visual Polish | TODO | Desirable (depends R-M5) |
| Game | G-M7 | Feel Tuning Pass | TODO | Desirable |

## Cross-workstream dependencies

| Downstream milestone | Requires renderer at | Requires engine at | Status |
|---|---|---|---|
| E-M1 | R-M1 | — | UNBLOCKED |
| E-M2 | R-M1 | E-M1 | UNBLOCKED |
| E-M3 | R-M1 | E-M1 | UNBLOCKED |
| E-M4 | R-M1 | E-M3 | UNBLOCKED |
| G-M1 | R-M1 | E-M1 | UNBLOCKED |
| G-M2 | R-M1 | E-M4 | UNBLOCKED |
| G-M3 | R-M1 + R-M3 | E-M3 + E-M4 | UNBLOCKED |
| G-M4 | R-M1 + renderer-owned `sokol_imgui` | E-M1 | UNBLOCKED |
| G-M5 | — | E-M5 | BLOCKED on engine E-M5 (Desirable) |
| G-M6 | R-M5 | E-M4 | BLOCKED on renderer R-M5 (Desirable) |

## Parallel groups and bottlenecks (game workstream)

| Group | Milestone | Tasks | File sets |
|---|---|---|---|
| PG-SETUP-A | SETUP | G-001, G-002 | `main.cpp` / `game.h`+`game.cpp` |
| PG-FOUND-A | FOUNDATION | G-003, G-004 | `components.h` / `constants.h` |
| PG-GM1-A | G-M1 | G-007, G-008, G-009 | `player.{h,cpp}` / `camera_rig.{h,cpp}` / `asteroid_field.{h,cpp}` |
| PG-GM2-A | G-M2 | G-011, G-012, G-013 | `asteroid_field.cpp` / `damage.{h,cpp}` / `player.cpp` |
| PG-GM3-A | G-M3 | G-015, G-016, G-017 | `weapons.{h,cpp}` / `projectile.{h,cpp}` / `enemy_ai.{h,cpp}` |
| PG-GM4-A | G-M4 | G-023, G-024 | `hud.{h,cpp}` / `game.cpp` |
| PG-POLISH-A | POLISH | G-028, G-029 | `constants.h` / (no code) |

No `BOTTLENECK` markers in the game workstream — `game_tick` wiring tasks (G-014, G-022, G-027) are SEQUENTIAL gates within their milestones, not BOTTLENECKs that block other workstreams.

## Mock-swap schedule

| Consumer | Mock source | CMake toggle | Swap to real impl when | Status |
|---|---|---|---|---|
| Engine workstream | `src/renderer/mocks/` | `USE_RENDERER_MOCKS` | R-M1 merged + interface frozen | DONE |
| Game workstream | `src/renderer/mocks/` | `USE_RENDERER_MOCKS` | Renderer milestones merge | DONE — real renderer used by default |
| Game workstream | `src/engine/mocks/` | `USE_ENGINE_MOCKS` | Engine E-M1 lands; later milestones replace remaining mocks | DONE — real engine used by default |
| Demo fallback on `main` | Re-enable broken upstream mock(s) | `USE_RENDERER_MOCKS` / `USE_ENGINE_MOCKS` | If any workstream is unstable at T-30 | Standby |

## Open questions / pre-merge blockers

| ID | Item | Status | Notes |
|---|---|---|---|
| OQ-1 | Final frozen interface docs under `docs/interfaces/` | RESOLVED | renderer FROZEN v1.1; engine FROZEN v1.2; game DRAFT v0.1 (final consumer — frozen on first behavioral validation) |
| OQ-2 | Shared API headers and mock generation rehearsal | RESOLVED | Both upstream mocks compile and link; toggled via `-DUSE_*_MOCKS=ON` |
| OQ-3 | `quickstart.md` / runbook creation | PARTIAL | `docs/planning/speckit/game/quickstart.md` exists; needs end-to-end pass on `rtx3090` (G-030) |
| OQ-4 | `sapp_request_quit` exposure to game (G-026 watchpoint) | OPEN | Game's Esc-to-quit flow may need a renderer-side helper if `renderer.h` does not transitively expose `sokol_app.h`. Filed as a watchpoint on G-026; flag a BLOCKER if discovered during implementation per blueprint §8.5 rule 11 (CMakeLists / renderer ownership). |

## SpecKit promotion log

| Workstream | Promoted to docs at | Source SpecKit dir |
|---|---|---|
| Renderer | docs/architecture/renderer-architecture.md, docs/interfaces/renderer-interface-spec.md, docs/planning/speckit/renderer/ | `specs/001-sokol-render-engine/` |
| Engine | docs/architecture/engine-architecture.md, docs/interfaces/engine-interface-spec.md, docs/planning/speckit/engine/ | `specs/002-ecs-physics-engine/` |
| Game | docs/architecture/game-architecture.md, docs/interfaces/game-interface-spec.md, docs/game-design/{GAME_DESIGN.md, game-workstream-design.md}, docs/planning/speckit/game/ | `specs/003-space-shooter-mvp/` (promoted 2026-04-26) |
