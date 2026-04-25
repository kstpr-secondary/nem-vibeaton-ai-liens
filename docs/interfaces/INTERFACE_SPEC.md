# Cross-Workstream Interface Specification

> **Index document.** Points to authoritative per-workstream interface specs.  
> Downstream workstreams MUST NOT begin SpecKit until the upstream interface they depend on is frozen.

---

## Freeze Status

| Interface | File | Status | Frozen By | Downstream May Start |
|---|---|---|---|---|
| Renderer public API | `docs/interfaces/renderer-interface-spec.md` | **FROZEN — v1.1** | Human supervisor (2026-04-23) | ✅ Engine + Game SpecKit complete |
| Engine public API | `docs/interfaces/engine-interface-spec.md` | **FROZEN — v1.2** | Human supervisor (post E-M4) | ✅ Game SpecKit complete |
| Game public API | `docs/interfaces/game-interface-spec.md` | **DRAFT — v0.1** (promoted from Game SpecKit, 2026-04-26) | — | N/A — game is final consumer |

---

## Freeze Protocol

1. Implementing workstream reaches its first sync-point milestone (R-M1, E-M1, etc.).
2. Human supervisor reviews the interface spec doc.
3. Human adds `**Status**: FROZEN — v<N>` at top of the file, adds freeze date, commits + pushes.
4. Human announces freeze to downstream workstream leads.
5. Any post-freeze change requires **explicit human approval** and a version bump.
6. Downstream workstreams that want a change must file a blocker in `_coordination/overview/BLOCKERS.md` — never modify the frozen spec to make their code compile.

---

## Renderer Interface — Summary

**File**: `docs/interfaces/renderer-interface-spec.md`  
**Status**: **FROZEN — v1.0** (2026-04-23, pre-implementation planning freeze)  
**Frozen after**: Renderer SpecKit completion; human-reviewed and approved

Key contracts:

- **Handle types**: `RendererMeshHandle`, `RendererTextureHandle` — opaque `uint32_t` structs; `id=0` is invalid.
- **Vertex layout**: `struct Vertex { float position[3]; float normal[3]; float uv[2]; float tangent[3]; }` — universal for all mesh types.
- **Shading models**: `Unlit` (R-M1), `Lambertian` (R-M2), `BlinnPhong` (R-M4 Desirable).
- **Lifecycle**: `renderer_init(config)` → `renderer_set_input_callback(cb, data)` → `renderer_run()` → (inside frame:) `begin_frame` / scene setup + enqueue / `end_frame` → `renderer_shutdown()`.
- **Draw limit**: 1024 draw calls per frame; overflow silently dropped + logged.
- **Resource lifetime**: all GPU resources live until `renderer_shutdown()`; no per-handle destroy.
- **Error behavior**: invalid handles skipped silently; shader/pipeline failure → magenta fallback; never crashes.
- **Mock surface**: `src/renderer/mocks/renderer_mock.cpp` toggled by `USE_RENDERER_MOCKS=ON`.

See `docs/interfaces/renderer-interface-spec.md` for the full C++ header.

---

## Engine Interface — Summary

**File**: `docs/interfaces/engine-interface-spec.md`  
**Status**: **FROZEN — v1.2** (post E-M4 — Phase 6 physics added: `ForceAccum`, `make_box_inv_inertia_body`, `make_sphere_inv_inertia_body`, `update_world_inertia`, fixed-timestep substep loop, impulse-based elastic response with Baumgarte correction)
**Frozen after**: E-M1–E-M4 implemented and human-reviewed

Key contracts:

- **Component types**: `Transform`, `Mesh`, `EntityMaterial`, `RigidBody`, `Collider`, `Camera`, `Light` + tag markers (`Static`, `Dynamic`, `Interactable`, `CameraActive`, `DestroyPending`).
- **Lifecycle**: `engine_init(config)` → `engine_tick(dt)` (inside renderer FrameCallback) → `engine_shutdown()`.
- **Scene API**: `engine_create_entity()`, `engine_destroy_entity(e)` (deferred), template `engine_add/get/remove/has_component<T>()`, `engine_registry()` for direct view access.
- **Procedural spawners**: `engine_spawn_sphere()`, `engine_spawn_cube()` — create entities with Transform + Mesh + EntityMaterial.
- **Asset loading**: `engine_load_gltf(path)` / `engine_load_obj(path)` → `RendererMeshHandle`; `engine_spawn_from_asset()` convenience.
- **Input**: `engine_key_down()`, `engine_mouse_delta()`, `engine_mouse_button()`, `engine_mouse_position()`.
- **Physics**: `engine_apply_force()`, `engine_apply_impulse()`, `engine_apply_impulse_at_point()`.
- **Queries**: `engine_raycast()` → `std::optional<RaycastHit>`, `engine_overlap_aabb()` → `std::vector<entt::entity>`.
- **Camera**: `engine_set_active_camera(e)` — computes view/projection from entity's Transform + Camera.
- **Time**: `engine_now()`, `engine_delta_time()`.
- **Error behavior**: invalid handles/entities silently ignored or logged; never crashes.
- **Mock surface**: `src/engine/mocks/engine_mock.cpp` toggled by `USE_ENGINE_MOCKS=ON`.

See `docs/interfaces/engine-interface-spec.md` for the full C++ header.

---

## Game Interface — Summary

**File**: `docs/interfaces/game-interface-spec.md`
**Status**: **DRAFT — v0.1** (promoted from Game SpecKit `contracts/game-api.md` on 2026-04-26)
**Will be frozen after**: G-M1 (Flight + Scene + Camera) lands and the lifecycle calling convention is validated end-to-end against real upstream

Key contracts:

- **Lifecycle**: `void game_init()` → register `FrameCallback` + `InputCallback` → `renderer_run()` → `game_shutdown()`. Game is the final consumer; nothing depends on its API.
- **Per-frame tick**: `game_tick(dt)` runs an 11-step system order — `engine_tick` → `containment_update` → `player_update` → `enemy_ai_update` → `weapon_update` → `projectile_update` → `damage_resolve` → `match_state_update` → `camera_rig_update` → `render_submit` → `hud_render`. Order is part of the contract.
- **Match state machine**: 4 states — `Playing → PlayerDead/Victory → Restarting → Playing`. Victory is checked before PlayerDead (simultaneous death → player wins).
- **Single registry**: engine owns it; game iterates via `engine_registry().view<...>()` and templated component helpers.
- **HUD**: renderer owns ImGui setup/event-forward/new_frame/render; game emits widgets only.
- **Mock surface**: game ships none; consumes `USE_RENDERER_MOCKS=ON` and/or `USE_ENGINE_MOCKS=ON` while upstream lands. At promotion time both upstream interfaces are frozen — mocks are demo-day fallback only.

---

## Shared Type Conventions

- All matrices: column-major `float[16]`, matching GLM's default layout.
- All vectors: `float[3]` or `float[4]` arrays in function signatures (no GLM types on public API boundaries).
- All colors: linear RGB or RGBA `float`, range [0,1].
- Handle validity: `id == 0` is always invalid; `id >= 1` is always valid.
