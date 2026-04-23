# Cross-Workstream Interface Specification

> **Index document.** Points to authoritative per-workstream interface specs.  
> Downstream workstreams MUST NOT begin SpecKit until the upstream interface they depend on is frozen.

---

## Freeze Status

| Interface | File | Status | Frozen By | Downstream May Start |
|---|---|---|---|---|
| Renderer public API | `docs/interfaces/renderer-interface-spec.md` | **FROZEN — v1.1** | Human supervisor (2026-04-23) | ✅ Engine SpecKit complete |
| Engine public API | `docs/interfaces/engine-interface-spec.md` | **DRAFT — v0.1** (promoted from Engine SpecKit) | — | ❌ Game SpecKit waits for engine freeze |
| Game public API | `docs/interfaces/game-interface-spec.md` | Placeholder — pending Game SpecKit | — | N/A |

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
**Status**: **DRAFT — v0.1** (promoted from Engine SpecKit `contracts/engine-api.md`)  
**Will be frozen after**: E-M1 (Bootstrap + ECS + Scene API) implemented and human-reviewed

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
**Status**: Placeholder — to be populated during Game SpecKit  
**Will be frozen after**: G-M1 (Flight Controller + Scene + Camera milestone)

Expected contracts (pre-SpecKit sketch — not binding):

- `game_init(engine_handle, renderer_handle)`
- `game_tick(dt)` — update all gameplay systems
- `game_render()` — emit draw calls to renderer
- HUD: uses renderer-owned Dear ImGui path; no second ImGui context.

---

## Shared Type Conventions

- All matrices: column-major `float[16]`, matching GLM's default layout.
- All vectors: `float[3]` or `float[4]` arrays in function signatures (no GLM types on public API boundaries).
- All colors: linear RGB or RGBA `float`, range [0,1].
- Handle validity: `id == 0` is always invalid; `id >= 1` is always valid.
