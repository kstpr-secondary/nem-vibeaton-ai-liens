# Master Tasks (compact dashboard)

> Pre-SpecKit scaffold owned by the Systems Architect. Per-workstream `TASKS.md` files are authoritative once populated on their feature branches.

## Milestone graph (renderer -> engine -> game)

| Workstream | Milestone | Scope | Status | Unlocks / Notes |
|---|---|---|---|---|
| Renderer | R-M0 | Bootstrap | TODO | OpenGL 3.3 init, `renderer_app`, input bridge exposure |
| Renderer | R-M1 | Unlit Forward Rendering + Procedural Scene | TODO | First engine sync point; frozen renderer API usable by engine |
| Renderer | R-M2 | Directional Light + Lambertian | TODO | Enables lit engine/game scenes with no engine API change |
| Renderer | R-M3 | Skybox + Line-Quads + Alpha Blending | TODO | Completes renderer MVP; required for laser/plasma visuals |
| Renderer | R-M4 | Blinn-Phong + Diffuse Textures | TODO | Desirable |
| Renderer | R-M5 | Custom Shader Hook + Sorted Transparency | TODO | Desirable; unlocks shader-driven game VFX |
| Engine | E-M1 | Bootstrap + ECS + Scene API | TODO | First game sync point; game starts here against later mocks |
| Engine | E-M2 | Asset Import | TODO | glTF/OBJ -> renderer mesh upload bridge |
| Engine | E-M3 | Input + AABB Colliders + Raycasting | TODO | Required for laser, camera control, collision queries |
| Engine | E-M4 | Euler Physics + Collision Response | TODO | Completes engine MVP; required for moving asteroids/projectiles |
| Engine | E-M5 | Enemy Steering AI | TODO | Desirable |
| Engine | E-M6 | Multiple Point Lights | TODO | Desirable |
| Engine | E-M7 | Capsule Mesh Integration | TODO | Desirable; depends on renderer capsule builder |
| Game | G-M1 | Flight + Scene + Camera | TODO | Starts at T+0 on frozen interfaces + mocks |
| Game | G-M2 | Physics, Containment, Asteroid Dynamics | TODO | Swaps to real engine physics when E-M4 lands |
| Game | G-M3 | Weapons + Enemies + HP/Shield | TODO | Requires line quads, raycast, projectile physics |
| Game | G-M4 | HUD + Game Flow + Restart | TODO | Completes game MVP |
| Game | G-M5 | AI Upgrade & Scaling | TODO | Desirable |
| Game | G-M6 | Visual Polish | TODO | Desirable |
| Game | G-M7 | Feel Tuning Pass | TODO | Desirable |

## Cross-workstream dependencies

| Downstream milestone | Requires renderer at | Requires engine at |
|---|---|---|
| E-M1 | R-M1 | — |
| E-M2 | R-M1 | E-M1 |
| E-M3 | R-M1 | E-M1 |
| E-M4 | R-M1 | E-M3 |
| E-M5 | R-M2 | E-M4 |
| E-M6 | R-M2, ideally R-M5 | E-M1 |
| E-M7 | R-M5 | E-M1 |
| G-M1 | R-M1 | E-M1 |
| G-M2 | R-M1 | E-M4 or engine physics mocks |
| G-M3 | R-M1 + R-M3 | E-M3 + E-M4 |
| G-M4 | R-M1 + ImGui integration | E-M1 |
| G-M5 | — | E-M5 |
| G-M6 | R-M5 | E-M4 |

## Parallel groups and bottlenecks

| Milestone | Candidate split | Constraint | Notes |
|---|---|---|---|
| R-M2 | Shader work vs C++ uniform/pipeline plumbing | `sg_directional_light_t`-style uniform block is BOTTLENECK | Final PG rows belong in per-workstream `TASKS.md` after SpecKit |
| R-M3 | Skybox path vs line-quad path | File sets must stay disjoint | Two natural PG siblings once files are declared |
| E-M2 | glTF importer vs OBJ fallback | Shared asset bridge header can become BOTTLENECK | Use `files:` lists in `Notes` once tasks exist |
| E-M3 | Input plumbing vs collider/raycast math | Keep driver/controller files separate from query math | Natural PG candidate |
| G-M3 | Laser path vs plasma path vs enemy AI | Keep weapon/VFX/gameplay files disjoint | Natural three-way PG candidate |
| G-M4 | HUD vs restart/win-loss flow | UI files separate from state-flow files | Natural PG candidate |

## Mock-swap schedule

| Consumer | Mock source | CMake toggle | Swap to real impl when | Notes |
|---|---|---|---|---|
| Engine workstream | `src/renderer/mocks/` | `USE_RENDERER_MOCKS` | Renderer R-M1 is merged and renderer interface is frozen | Engine must not idle on renderer delivery |
| Game workstream | `src/renderer/mocks/` | `USE_RENDERER_MOCKS` | Renderer milestones merge incrementally | Game can begin with renderer mocks at T+0 |
| Game workstream | `src/engine/mocks/` | `USE_ENGINE_MOCKS` | Engine E-M1 lands for scene API, later milestones replace remaining mocks | Game never blocks on full engine delivery |
| Demo fallback on `main` | Re-enable broken upstream mock(s) | `USE_RENDERER_MOCKS` / `USE_ENGINE_MOCKS` | If any workstream is unstable at T-30 | Keeps demo launchable on `rtx3090` |

## Open questions / pre-merge blockers

| ID | Item | Status | Notes |
|---|---|---|---|
| OQ-1 | Final frozen interface docs under `docs/interfaces/` | OPEN | Must exist before downstream SpecKit runs |
| OQ-2 | Shared API headers and mock generation rehearsal | OPEN | Blueprint pre-hackathon requirement |
| OQ-3 | `quickstart.md` / runbook creation | OPEN | Required by blueprint checklist before start |
