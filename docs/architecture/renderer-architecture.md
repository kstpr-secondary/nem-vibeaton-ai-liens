# Renderer Architecture

> **Status:** SpecKit plan phase output. Not frozen — becomes authoritative after human review and the interface freeze.

---

## Ownership and Boundaries

- **Renderer owns**: `sokol_app` init, the main frame callback, input forwarding, Dear ImGui initialization/shutdown/frame/render, all GPU pipeline creation, the public `renderer.h` API.
- **Renderer does not own**: a scene graph, entity lifecycle, physics, asset import (glTF/OBJ parsing lives in the engine).
- **Engine and game consume**: only `renderer.h` and the types it exposes. Internal renderer files are never included by downstream workstreams.
- **Procedural mesh builders** (`make_sphere_mesh`, `make_cube_mesh`) live in the renderer; engine wraps them into entity-creation helpers.

---

## Frame Lifecycle

```
sokol_app frame event
  └─► renderer internal frame_callback()
         ├─ simgui_new_frame()          (ImGui frame start)
         ├─ [engine tick() if registered]
         │    └─ consumer calls:
         │         renderer_set_camera(...)
         │         renderer_set_directional_light(...)
         │         renderer_set_skybox(...)
         │         renderer_enqueue_draw(...) × N
         │         renderer_enqueue_line_quad(...) × M
         │         ImGui::* widget calls
         └─ renderer_end_frame()
               ├─ pass 0: skybox (depth write OFF, drawn first)
               ├─ pass 1: opaque draws, sorted front-to-back (R-M6, Desirable)
               ├─ pass 2: transparent draws (alpha < 1, line quads — basic, unsorted in MVP)
               └─ pass 3: simgui_render() (ImGui UI overlay)
```

---

## Shader Pipeline

```
shaders/renderer/unlit.glsl       ──sokol-shdc──► build/generated/shaders/unlit.glsl.h
shaders/renderer/lambertian.glsl  ──sokol-shdc──► build/generated/shaders/lambertian.glsl.h
shaders/renderer/skybox.glsl      ──sokol-shdc──► build/generated/shaders/skybox.glsl.h
shaders/renderer/line_quad.glsl   ──sokol-shdc──► build/generated/shaders/line_quad.glsl.h
shaders/renderer/blinnphong.glsl  ──sokol-shdc──► build/generated/shaders/blinnphong.glsl.h  (R-M4)
```

CMake custom commands drive compilation; Ninja tracks `DEPENDS` on the `.glsl` source. Missing `sokol-shdc` on PATH is a configure-time error when `VIBEATON_REQUIRE_SOKOL_SHDC=ON` (default).

At runtime: `shd_<name>_shader_desc(sg_query_backend())` feeds the generated descriptor into `sg_make_shader()`. Any failure → magenta error pipeline (logged, no crash).

---

## Pipeline Inventory

| Pipeline | Milestone | Blend | Depth Write | Use |
|----------|-----------|-------|-------------|-----|
| `pipeline_unlit` | R-M1 | off | on | Flat-color opaque geometry |
| `pipeline_lambertian` | R-M2 | off | on | Diffuse-lit opaque geometry |
| `pipeline_blinnphong` | R-M4 | off | on | Specular+diffuse opaque (Desirable) |
| `pipeline_transparent` | R-M3 | alpha | off | Alpha-blended draws + line quads |
| `pipeline_skybox` | R-M3 | off | off | Cubemap background |
| `pipeline_line_quad` | R-M3 | alpha | off | Billboard laser quads |
| `pipeline_magenta` | R-M0 | off | on | Fallback on creation failure |

All pipelines share the same `Vertex` layout (Position + Normal + UV + Tangent). Shaders that don't use all attributes simply ignore them.

---

## Module Split

| File | Responsibility | Milestone |
|------|---------------|-----------|
| `renderer.h` | Public API types + declarations (frozen interface) | R-M0 |
| `renderer.cpp` | `init` / `shutdown` / `run` / `begin_frame` / `end_frame`; pipeline dispatch | R-M0 |
| `pipeline_unlit.cpp/h` | Unlit pipeline + shader creation | R-M1 |
| `pipeline_lambertian.cpp/h` | Lambertian pipeline + light uniform | R-M2 |
| `pipeline_blinnphong.cpp/h` | Blinn-Phong pipeline + texture sampler | R-M4 |
| `mesh_builders.cpp/h` | `make_sphere_mesh`, `make_cube_mesh`, upload helpers | R-M1 |
| `skybox.cpp/h` | Cubemap loading, skybox pass | R-M3 |
| `debug_draw.cpp/h` | Line-quad billboard generation + pass | R-M3 |
| `texture.cpp/h` | `upload_texture_2d`, `upload_texture_from_file`, `upload_cubemap` | R-M4 |
| `app/main.cpp` | `renderer_app` driver — updated at each milestone | R-M0+ |
| `mocks/renderer_mock.cpp` | No-op stubs for `USE_RENDERER_MOCKS=ON` | R-M0 |

---

## Asset and Path Rules

- **Shaders**: No runtime path needed — compiled into `${CMAKE_BINARY_DIR}/generated/shaders/*.glsl.h`.
- **Textures / meshes**: Runtime-loaded content uses `ASSET_ROOT` macro from `paths.h` (`cmake/paths.h.in` → `${CMAKE_BINARY_DIR}/generated/paths.h`). Hard-coded relative paths are prohibited.
- All GPU resources held for process lifetime; released collectively in `renderer_shutdown()`.

---

## Dear ImGui Integration

Renderer owns `util/sokol_imgui.h` from the pinned sokol repo. Integration flow:
1. `renderer_init()` → `simgui_setup()`
2. Each frame: `simgui_new_frame()` before 3D rendering, `simgui_render()` after 3D scene in the same render pass
3. `renderer_shutdown()` → `simgui_shutdown()`
4. sokol_app input events forwarded via `simgui_handle_event()` inside the renderer's event handler

Game code only emits `ImGui::*` widget calls. No second ImGui init/shutdown allowed.

---

## Open Questions (resolved during plan phase)

All questions from the pre-SpecKit scaffold have been resolved. See `specs/001-sokol-render-engine/research.md` for full Decision records.
