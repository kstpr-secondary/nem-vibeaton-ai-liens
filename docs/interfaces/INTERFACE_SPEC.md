# Cross-Workstream Interface Specification

> **Index document.** Points to authoritative per-workstream interface specs.  
> Downstream workstreams MUST NOT begin SpecKit until the upstream interface they depend on is frozen.

---

## Freeze Status

| Interface | File | Status | Frozen By | Engine SpecKit May Start |
|---|---|---|---|---|
| Renderer public API | `docs/interfaces/renderer-interface-spec.md` | Draft — frozen after R-M1 sign-off (task R-055) | Human supervisor after R-M1 | ❌ Not yet — wait for R-M1 |
| Engine public API | `docs/interfaces/engine-interface-spec.md` | Draft — pending Engine SpecKit | — | ❌ No |
| Game public API | `docs/interfaces/game-interface-spec.md` | Draft — pending Game SpecKit | — | ❌ No |

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
**Status**: Draft — frozen after R-M1 human sign-off (task R-055)  
**Frozen after**: R-M1 (Unlit Forward Rendering + Procedural Scene milestone)

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
**Status**: Draft — to be populated during Engine SpecKit  
**Will be frozen after**: E-M1 (Bootstrap + ECS + Scene API milestone)

Expected contracts (pre-SpecKit sketch — not binding):

- ECS scene API: entity create/destroy, component add/remove/get, transform management.
- Asset bridge: `engine_load_mesh(path)` → `RendererMeshHandle` (delegates to renderer upload).
- Game loop: `engine_tick(dt)` called from inside renderer frame callback.
- Input: engine registers callback via `renderer_set_input_callback`; engine routes to game layer.
- Physics queries: AABB overlap, raycast returning hit entity + distance.
- Collision: Euler rigid-body integration, AABB-vs-AABB response.

---

## Game Interface — Summary

**File**: `docs/interfaces/game-interface-spec.md`  
**Status**: Draft — to be populated during Game SpecKit  
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
