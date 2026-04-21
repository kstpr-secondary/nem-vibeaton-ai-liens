# Milestones & MVP Feasibility Review

Companion to `Milestones_MVPs_Concepts.md`. Scope: renderer + game engine only (game stays deferred until these two are frozen). Judged against the blueprint's 5–6 hour budget, three workstreams, AI-assisted implementation, and the stated target of **~1 milestone merge/hour/workstream (≈5 milestones total per workstream)**.

Reality check first: at T+0 the game workstream is blocked on renderer M1 *and* engine M1; if those slip past T+2h, the game has <3h left. Every unnecessary item in an MVP milestone is a direct tax on the game's survival.

---

## 1. Renderer — recommended re-slicing

### Problems in the current draft

- **M2 is overloaded.** Frustum culling + front-to-back sorting + directional light + Lambertian + line rendering + skybox + stress test is 3–4 milestones packed as one. Under the ~1-per-hour target, M2 alone would consume 2–3 hours and block the engine from ever seeing lit output.
- **Culling and sorting are not MVP.** With the scene sizes the game actually needs (a few hundred asteroids + ships + projectiles), unsorted forward rendering with no culling runs fine on an RTX 3090. Culling is a performance concern, not a visual one — demote it.
- **Line rendering *is* MVP** — the game design relies on laser beams. Must stay.
- **Skybox is MVP** — a space shooter without a starfield background looks broken.
- **Unlit → Lambertian should be two visible increments, not one jump** — unlit in M1 lets the engine wire meshes in against something working; Lambertian comes right after.

### Proposed renderer milestones

**MVP group**

- **R-M0 — Bootstrap.** Sokol init (Vulkan primary, GL 3.3 fallback decision **must** happen here — do not let it slip to M1), `sokol_app` window, input plumbing, clear-color frame. Skippable as a sync point. *Target: ≤45 min.*
- **R-M1 — Unlit forward rendering + procedural scene.** Forward pass, perspective camera, shader compile pipeline, opaque queue, unlit solid-color shading, public API for the engine to enqueue meshes + transforms + color, hardcoded procedural scene of spheres/cubes/capsules in `renderer_app`. **Sync point for engine.** *Target: ~1h.*
- **R-M2 — Directional light + Lambertian.** Single directional light, Lambertian fragment shader, base-color material parameter, toggleable vs unlit. *Target: ~45 min.*
- **R-M3 — Skybox + line rendering.** Cubemap or equirectangular skybox pass, depth-tested line primitives for laser beams. Both are MVP-critical for the game; grouped because they are independent and file-disjoint → natural parallel group. *Target: ~45 min.*

MVP stops here. After R-M3 the game has: lit shaded meshes, starfield background, laser beam visuals. That is sufficient for a demoable shooter.

**Desirable group**

- **R-M4 — Blinn-Phong + diffuse textures.** Specular material, albedo texture sampling. (Blinn-Phong is a small delta over Lambertian — keep.)
- **R-M5 — Custom shader hook + alpha-blended queue.** Engine-supplied fragment shaders applied to a quad/sphere (explosions, plasma, fresnel) *and* alpha-blended transparency queue. Grouped because explosions and plasma need both. Promoting alpha-blended from old-M4 to here — the game design genuinely needs it for VFX.
- **R-M6 — Frustum culling + front-to-back sort + stress test.** Promoted out of old-M2. Pure optimization; land only if the stress test actually shows a problem.

**Stretch group** (unchanged in spirit, trimmed)

- **R-M7 — Normal maps + procedural sky shader.** Human comment on the draft already flags procedural sky as optional — keep that. Normal maps cost tangent-space bookkeeping and an asset pipeline bump; stretch.
- **R-M8 — Shadows (adapted from Sokol sample) + PBR Cook-Torrance + IBL.** Last-hour exploration only, consistent with the original human comment.
- **Cut entirely from planning:** MSAA, deferred/clustered, AO, GPU-driven culling, MDI. Note the human's suggestion to slip **GPU instancing and draw-call batching** into earlier milestones is sensible only if trivially available; otherwise skip.

### Net effect on renderer
5 MVP+desirable milestones (M0–M5) instead of the previous 4, but each is sized to fit the hourly cadence, and MVP (M0–M3) is now reachable in ~3.5h with headroom.

---

## 2. Game Engine — recommended re-slicing

### Problems in the current draft

- **M1 is a mega-milestone.** Game loop + procedural shapes + asset import + ECS + camera + lights + component schemas + scene API + asset-to-entity pipeline + procedural scene building. This is at least three milestones. If GE-M1 consumes the first 2+ hours, the engine never reaches physics.
- **Component schema design is spec work, not a milestone deliverable.** It belongs in the Engine SpecKit output (frozen before T+0), not in a coding milestone.
- **Procedural shape duplication with the renderer is avoidable.** The renderer already owns mesh creation for `renderer_app`; it should expose `make_sphere()` / `make_cube()` / `make_capsule()` mesh builders on its public API. Engine calls them and wraps the mesh handles in ECS entities. No duplication; no friction — same decision the blueprint's "frozen interface + mock" pattern already pushes us toward.
- **"Pathfinding" is misnamed.** For a space shooter in an open asteroid field there are no navmeshes — just steering (seek, avoid). A* / navmesh pathfinding is stretch at best; call the MVP piece "enemy steering."
- **Raycasting scope.** Hit-testing with mouse click is game-layer concern. The engine only needs the raycast primitive; *using* it for mouse-pick is a game task. Keep the primitive in engine MVP, drop the mouse-pick acceptance criterion.

### Proposed engine milestones

**MVP group**

- **E-M1 — Engine bootstrap + ECS + scene API.** Update loop driven from renderer frame callback, `entt` registry, camera + directional-light entities, public scene API (create entity, attach transform/mesh/material, iterate), consume renderer's procedural shape builders. No asset import yet. *Target: ~1h.* **Sync point: game workstream can start against this + mocks.**
- **E-M2 — Asset import.** glTF via `cgltf`, OBJ via `tinyobjloader`, pre-verified `/assets` folder, asset-handle → renderer mesh-upload bridge, asset-to-entity helper. *Target: ~45 min.*
- **E-M3 — Input + AABB colliders + raycasting.** Input routing from `sokol_app`, WASD/mouse → camera/player transform, AABB colliders on entities, ray-vs-AABB and AABB-vs-AABB tests (brute force is fine at expected scale). *Target: ~1h.*
- **E-M4 — Euler physics + collision response.** Linear/angular velocity, mass, impulse-based elastic collision response, time access API (wrap `sokol_time`), fixed timestep or capped dt. *Target: ~1h.*

MVP stops here. The game has: scene building, loaded assets, moving player, collidable asteroids, physics. Enough for a playable space shooter core.

**Desirable group**

- **E-M5 — Enemy steering AI.** Seek + basic obstacle avoidance (raycast-ahead) — *not* pathfinding. Renamed from the draft's "pathfinding."
- **E-M6 — Multiple point lights.** For explosions/weapon glow. Small if renderer already supports a light array; depends on renderer M2/M5.

**Stretch group**

- Convex-hull colliders, spatial partitioning (BVH/grid), advanced avoidance — all stretch as originally marked.
- **Cut from planning:** true pathfinding, animation, audio, networking, editor (already cut in blueprint §5).

### Shared concern: procedural shapes

**Decision to freeze in Renderer SpecKit:** procedural shape generation lives in the **renderer** public API (`make_sphere_mesh`, `make_cube_mesh`, `make_capsule_mesh`) and returns mesh handles. Engine and game consume them. `renderer_app` also consumes them for its driver scene. One implementation, zero duplication, no friction.

---

## 3. Implications for the game concept (informational — no milestones yet)

The game concept as written is sized for a weekend, not the trailing 2–3 hours that will actually remain. Pre-cuts to agree on before Game SpecKit:

- **Power-ups system → cut entirely from MVP.** It multiplies weapon states, visualization work, collision-pickup code paths, and UI complexity. Game design already marks it optional — keep it firmly stretch.
- **Rocket launcher → stretch.** Already marked optional. Homing missiles = steering AI + new projectile type + new VFX; too much for MVP.
- **Damper objects that absorb kinetic energy → cut.** Interesting physics but an MVP distraction. If total-energy growth becomes a demo problem, cap asteroid speed at reflection instead.
- **Containment field reflection** → keep, cheap (sphere-point distance + velocity reflect).
- **Third-person camera** → keep but budget for camera rig complexity. First-person is a safer fallback if rig work overruns.
- **Shield + health as separate resources** → keep; two bars, trivially more code than one.
- **Boost speed** → keep; single float + timer.
- **Two weapons at MVP (laser + plasma)** → keep. Both are cheap: laser = ray+line, plasma = projectile entity + sphere mesh + hit test.
- **Restart flow (enter / on death)** → keep; must work for demo.

Net effect: the game MVP is ship movement + two weapons + asteroids with physics + containment + HP/shield/boost + HUD bars + crosshair. Everything else moves to stretch and is only touched after T-60 min if the MVP is already solid.

---

## 4. Summary table — proposed MVP scope

| Workstream | MVP milestones | Est. time to MVP | Promoted from draft | Demoted from draft |
|---|---|---|---|---|
| Renderer | M0 bootstrap → M1 unlit/API → M2 Lambertian → M3 skybox+lines | ~3.5h | skybox & lines now MVP (were bundled in M2) | culling, sorting, stress test → desirable |
| Engine | M1 ECS/scene → M2 asset import → M3 input+collision → M4 physics | ~3.75h | asset import split out | pathfinding → desirable (as "steering"); mouse-pick → game layer |
| Game | deferred | — | — | power-ups, rockets, dampers → stretch |

---

## 5. Open decisions to make before SpecKit freeze

1. **Vulkan-vs-GL fallback gate.** Must be a hard checkpoint *inside* R-M0, not R-M1 — if Vulkan experimental backend misbehaves we need to know before any rendering code is written against it.
2. **Procedural shape ownership.** Confirm renderer owns it and engine consumes (per §2 above).
3. **Alpha-blended queue placement.** Keep in desirable (R-M5) or pull into MVP for explosion/plasma VFX? Recommend desirable — MVP laser and plasma can both be opaque and still look fine.
4. **Camera perspective.** Confirm third-person as intended; agree first-person fallback if rig work balloons.
5. **Precompiled-shader decision** (already flagged in blueprint §3.3). If adopted, affects R-M1 shader pipeline scope.
