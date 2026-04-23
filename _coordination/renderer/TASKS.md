# TASKS.md — Renderer Workstream

> **Live operational task board.** Authoritative on `feature/renderer` branch.  
> Translated from `specs/001-sokol-render-engine/tasks.md`. Source spec: `specs/001-sokol-render-engine/`.  
> **Before claiming:** read the task row, then the spec section it references.  
> **To claim:** human sets `Owner = <agent>@<machine>`, `Status = CLAIMED`, commits + pushes, then triggers agent.  
> Related docs: `docs/interfaces/renderer-interface-spec.md` (frozen API), `docs/architecture/renderer-architecture.md` (frozen arch).

---

## Milestone: SETUP — Build Scaffold and Foundation

**Expected outcome**: `cmake --build build --target renderer_app` compiles with stub bodies; binary launches and exits cleanly. Renderer mock compiles under `USE_RENDERER_MOCKS=ON`.

| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
|---|---|---|---|---|---|---|---|---|---|
| R-001 | Verify existing `sokol-shdc` CMake infrastructure: confirm `vibeaton_attach_shader_headers()` glob picks up all expected shaders (`magenta.glsl`, `unlit.glsl`, `lambertian.glsl`, `skybox.glsl`, `line_quad.glsl`, `blinnphong.glsl`); confirm `VIBEATON_REQUIRE_SOKOL_SHDC=ON` guard exists; confirm generated dir in `renderer` PUBLIC include path; add `SOKOL_NO_ENTRY` compile definition on `renderer` target (required for `renderer_run()` → `sapp_run()` pattern); add `SOKOL_GLCORE33` compile definition on `renderer` target | HIGH | TODO | — | — | SETUP | BOTTLENECK | SELF-CHECK | files: CMakeLists.txt |
| R-002 | Create `src/renderer/renderer.h` with all public types and free-function declarations from `docs/interfaces/renderer-interface-spec.md`: handles, RendererConfig, Vertex, ShadingModel, Material, DirectionalLight, RendererCamera, InputCallback, all free-function declarations; bodies are empty stubs | MED | TODO | — | R-001 | SETUP | PG-SETUP-A | SPEC-VALIDATE | files: src/renderer/renderer.h |
| R-003 | Verify `cmake/paths.h.in` exists with `PROJECT_SOURCE_ROOT` and `ASSET_ROOT` macros; verify `CMakeLists.txt` generates `${CMAKE_BINARY_DIR}/generated/paths.h`; confirm `renderer` target exposes generated dir via PUBLIC include so downstream targets inherit it — **NOTE: already implemented in CMakeLists.txt, this is verification only** | LOW | TODO | — | R-001 | SETUP | PG-SETUP-A | SELF-CHECK | files: cmake/paths.h.in, CMakeLists.txt |
| R-004 | Confirm `stb_image` FetchContent entry exists in `CMakeLists.txt` with pinned version; verify PUBLIC include path propagation to `engine` and `game` targets (renderer PUBLIC → engine PUBLIC → game PRIVATE); confirm `STB_IMAGE_IMPLEMENTATION` is NOT defined in any header file — **NOTE: already implemented in CMakeLists.txt, this is verification only** | LOW | TODO | — | R-001 | SETUP | PG-SETUP-A | SELF-CHECK | files: CMakeLists.txt |
| R-005 | Create `src/renderer/mocks/renderer_mock.cpp`: no-op implementations for every void function in `renderer.h`; handle-returning functions return `{1}` (valid sentinel); verify `USE_RENDERER_MOCKS=ON` CMake flag swaps this in as the renderer static lib | MED | TODO | — | R-002 | SETUP | PG-SETUP-B | SELF-CHECK | files: src/renderer/mocks/renderer_mock.cpp |
| R-006 | Create empty `.cpp`/`.h` file pairs for all modules: `renderer.cpp`, `pipeline_unlit.cpp/h`, `pipeline_lambertian.cpp/h`, `pipeline_blinnphong.cpp/h`, `mesh_builders.cpp/h`, `skybox.cpp/h`, `debug_draw.cpp/h`, `texture.cpp/h`; also create `shaders/renderer/magenta.glsl` (pass-through vs + solid magenta fs) | LOW | TODO | — | R-002 | SETUP | PG-SETUP-B | NONE | files: src/renderer/renderer.cpp, src/renderer/pipeline_unlit.cpp, src/renderer/pipeline_unlit.h, src/renderer/pipeline_lambertian.cpp, src/renderer/pipeline_lambertian.h, src/renderer/pipeline_blinnphong.cpp, src/renderer/pipeline_blinnphong.h, src/renderer/mesh_builders.cpp, src/renderer/mesh_builders.h, src/renderer/skybox.cpp, src/renderer/skybox.h, src/renderer/debug_draw.cpp, src/renderer/debug_draw.h, src/renderer/texture.cpp, src/renderer/texture.h, shaders/renderer/magenta.glsl |
| R-007 | Define renderer internal state struct (file-private in `renderer.cpp`): `DrawCommand draw_queue[1024]`, `LineQuadCommand line_quad_queue[256]`, counters, `RendererCamera camera`, `DirectionalLight light`, `RendererTextureHandle skybox_handle`, pipeline handles (unlit/lambertian/blinnphong/transparent/line_quad/skybox/magenta), `sg_buffer mesh_vbufs[512]/mesh_ibufs[512]`, `sg_image texture_table[256]`, next-id counters, `bool frame_active`, `bool initialized`, input callback + userdata, stored `RendererConfig`. **This file also owns the sokol IMPL macros**: `#define SOKOL_GFX_IMPL`, `#define SOKOL_APP_IMPL`, `#define SOKOL_GLUE_IMPL`, `#define SOKOL_TIME_IMPL`, `#define SOKOL_LOG_IMPL` before including the respective sokol headers (once only, all other TUs include without IMPL). Also `#define SIMGUI_IMPL` before including `util/sokol_imgui.h` | HIGH | TODO | — | R-002, R-006 | SETUP | SEQUENTIAL | SELF-CHECK | files: src/renderer/renderer.cpp |
| R-008 | Implement `renderer_begin_frame()` skeleton: assert `!frame_active`; set `frame_active=true`; reset draw/line-quad counts to 0; call `sg_begin_pass(&(sg_pass){.action = pass_action, .swapchain = sglue_swapchain()})` with stored clear color in `pass_action` (modern sokol API — `sg_begin_default_pass` is removed) | MED | TODO | — | R-007 | SETUP | PG-FOUND-A | SELF-CHECK | files: src/renderer/renderer.cpp |
| R-009 | Implement `renderer_enqueue_draw()` and `renderer_enqueue_line_quad()` guard logic: silently return if `!frame_active`; drop + log if at capacity (1024 / 256); skip invalid mesh handle; skip zero-length or zero-width line quad | MED | TODO | — | R-007 | SETUP | PG-FOUND-A | SELF-CHECK | files: src/renderer/renderer.cpp |
| R-010 | Implement magenta error pipeline helper (file-private in `renderer.cpp`): `#include <generated/shaders/magenta.glsl.h>`; call `shd_magenta_shader_desc(sg_query_backend())`; `sg_make_pipeline` with pass-through MVP vertex + solid magenta fragment; on failure log fatal; store result; called from the **internal init callback** (not `renderer_init()` directly — GL context only exists after `sapp_run()`) | HIGH | TODO | — | R-001, R-007 | SETUP | SEQUENTIAL | SELF-CHECK | files: src/renderer/renderer.cpp, shaders/renderer/magenta.glsl |
| R-011 | Create `src/renderer/app/main.cpp` driver stub: regular `int main()` that creates a `RendererConfig` and calls `renderer_init(cfg)` then `renderer_run()` (both stubs at this stage); **do NOT use `sokol_main()`** — the renderer uses `SOKOL_NO_ENTRY` + `sapp_run()` inside `renderer_run()` to match the public API lifecycle | MED | TODO | — | R-002 | SETUP | SEQUENTIAL | NONE | files: src/renderer/app/main.cpp |

---

## Milestone: R-M0 — Bootstrap (≤ 45 min gate)

**Expected outcome**: `renderer_app` opens a window, clears to configured color, prints keyboard/mouse events to console, exits cleanly. OpenGL 3.3 verified on all target machines.

| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
|---|---|---|---|---|---|---|---|---|---|
| R-012 | Implement `renderer_init()` and the **internal init callback** in `renderer.cpp`: `renderer_init()` stores `RendererConfig` only (no GL calls — context doesn't exist yet). The internal init callback (wired via `sapp_desc.init_cb` in R-014) calls `sg_setup({.environment = sglue_environment(), .logger.func = slog_func})` (modern sokol API per sokol-api SKILL §3), then calls magenta pipeline helper (R-010), then `simgui_setup({.sample_count = sapp_sample_count()})`. `SOKOL_GLCORE33` compile definition is added in R-001 | HIGH | TODO | — | R-007, R-010, R-011 | R-M0 | PG-RM0-A | SELF-CHECK | files: src/renderer/renderer.cpp |
| R-013 | Implement `renderer_set_input_callback()` in `renderer.cpp`: store cb + userdata; implement sokol_app event_cb that calls `simgui_handle_event(e)` then dispatches to `input_cb` if non-null | MED | TODO | — | R-007 | R-M0 | PG-RM0-A | SELF-CHECK | files: src/renderer/renderer.cpp |
| R-014 | Implement `renderer_run()` in `renderer.cpp`: build `sapp_desc` from stored config wiring `init_cb` → internal init (R-012), `frame_cb` → internal frame, `cleanup_cb` → internal cleanup, `event_cb` → internal event; set `.gl = {.major_version=3, .minor_version=3}`; call `sapp_run(&desc)` (blocking — requires `SOKOL_NO_ENTRY` from R-001); populate width/height/title from stored config | MED | TODO | — | R-007 | R-M0 | PG-RM0-A | SELF-CHECK | files: src/renderer/renderer.cpp |
| R-015 | Implement `renderer_shutdown()` in `renderer.cpp`: call `simgui_shutdown()`, `sg_shutdown()`, `sapp_quit()`; set `frame_active=false`; zero all pipeline handles | MED | TODO | — | R-012 | R-M0 | SEQUENTIAL | SELF-CHECK | files: src/renderer/renderer.cpp |
| R-016 | Update `src/renderer/app/main.cpp` for R-M0 demo: create RendererConfig (title "R-M0 Bootstrap", clear color `0.05,0.05,0.10,1.0`); register input callback printing event type via `printf`; call `renderer_init`, `renderer_set_input_callback`, `renderer_run` | LOW | TODO | — | R-012, R-013, R-014 | R-M0 | SEQUENTIAL | SELF-CHECK | files: src/renderer/app/main.cpp |
| R-017 | Build `renderer_app`; verify: window opens to clear color, input events logged, clean exit; verify mock compiles: `cmake -S . -B build-mock -G Ninja -DUSE_RENDERER_MOCKS=ON && cmake --build build-mock --target engine_app` and confirm exit 0; **human sign-off required (SC-001)** | MED | TODO | — | R-015, R-016 | R-M0 | SEQUENTIAL | SPEC-VALIDATE + REVIEW | files: (verification only) |

---

## Milestone: R-M1 — Unlit Forward Rendering + Procedural Scene (~1 h) 🔑 ENGINE SYNC POINT

**Expected outcome**: `renderer_app` displays ≥10 colored unlit 3D objects at correct positions under a perspective camera. Interface frozen after human sign-off → engine workstream may begin.

| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
|---|---|---|---|---|---|---|---|---|---|
| R-018 | Author `shaders/renderer/unlit.glsl`: `@vs unlit_vs` — inputs `position(vec3)`, `normal(vec3)`, `uv(vec2)`, `tangent(vec3)`; uniform `vs_params { mat4 mvp; }`; `@fs unlit_fs` — uniform `fs_params { vec4 base_color; }`; output `base_color`; `@program unlit unlit_vs unlit_fs` | MED | TODO | — | R-017 | R-M1 | PG-RM1-A | SELF-CHECK | files: shaders/renderer/unlit.glsl |
| R-019 | Create `src/renderer/pipeline_unlit.cpp/h`: `sg_pipeline create_pipeline_unlit()` — include generated `unlit.glsl.h`; `shd_unlit_shader_desc(sg_query_backend())`; configure pipeline with full Vertex layout (pos float3, normal float3, uv float2, tangent float3), depth test ON + write ON, cull back; on `sg_make_pipeline` failure return magenta handle | MED | TODO | — | R-017 | R-M1 | PG-RM1-B | SELF-CHECK | files: src/renderer/pipeline_unlit.cpp, src/renderer/pipeline_unlit.h |
| R-020 | Implement `renderer_upload_mesh()` in `mesh_builders.cpp`: create `sg_buffer` for vertices (VERTEXBUFFER, IMMUTABLE) and indices (INDEXBUFFER, IMMUTABLE); on failure return `{0}`; store in `mesh_vbufs[next_mesh_id]` / `mesh_ibufs[next_mesh_id]`; return `{next_mesh_id++}` | MED | TODO | — | R-017 | R-M1 | PG-RM1-B | SELF-CHECK | files: src/renderer/mesh_builders.cpp, src/renderer/mesh_builders.h |
| R-021 | Implement `renderer_make_sphere_mesh()` in `mesh_builders.cpp`: UV sphere with lat/lon rings; compute Position, Normal (=normalize(pos)), UV (lon/lat→[0,1]), Tangent (=normalize(cross(up,normal))); pack into `Vertex[]`; triangle-strip index buffer; call `renderer_upload_mesh()` | MED | TODO | — | R-020 | R-M1 | PG-RM1-B | SELF-CHECK | files: src/renderer/mesh_builders.cpp |
| R-022 | Implement `renderer_make_cube_mesh()` in `mesh_builders.cpp`: 24 vertices (4 per face × 6), correct per-face normals, UVs, tangents (face local X); 36 indices; call `renderer_upload_mesh()` | MED | TODO | — | R-020 | R-M1 | PG-RM1-B | SELF-CHECK | files: src/renderer/mesh_builders.cpp |
| R-023 | Implement `renderer_set_camera()` in `renderer.cpp`: store RendererCamera in internal state; compute `vp = projection × view` each frame using `glm::make_mat4`; if not called before `end_frame()`, use identity + emit debug warning | MED | TODO | — | R-017 | R-M1 | SEQUENTIAL | SELF-CHECK | files: src/renderer/renderer.cpp |
| R-024 | Implement opaque draw dispatch in `renderer_end_frame()` in `renderer.cpp`: iterate `draw_queue[0..draw_count-1]`; for each: select `pipeline_unlit`; compute `mvp = vp × model`; upload `vs_params` + `fs_params`; bind `mesh_vbufs/ibufs[cmd.mesh.id]`; call `sg_draw` | HIGH | TODO | — | R-018, R-019, R-020, R-023 | R-M1 | SEQUENTIAL | SELF-CHECK | files: src/renderer/renderer.cpp |
| R-025 | Implement `renderer_make_unlit_material()`, stub `renderer_make_lambertian_material()`, stub `renderer_make_blinnphong_material()` in `renderer.cpp`; Lambertian and BlinnPhong stubs return `Material{ShadingModel::Unlit}` at this stage; **all three must exist before interface freeze** | LOW | TODO | — | R-017 | R-M1 | SEQUENTIAL | SELF-CHECK | files: src/renderer/renderer.cpp |
| R-026 | Update `src/renderer/app/main.cpp` for R-M1 demo: call `renderer_make_sphere_mesh` + `renderer_make_cube_mesh`; set fixed perspective camera (45° FOV, near=0.1, far=100); enqueue ≥10 colored unlit draw calls at varied positions/rotations/scales | MED | TODO | — | R-024, R-021, R-022 | R-M1 | SEQUENTIAL | SELF-CHECK | files: src/renderer/app/main.cpp |
| R-027 | Build + run `renderer_app`; verify ≥10 primitives render without artifacts; after human sign-off: verify `docs/interfaces/renderer-interface-spec.md` FROZEN v1.0 matches as-built code (spec already frozen from SpecKit phase); announce freeze confirmation to engine workstream (Person B / laptopB); **SC-002 + SC-007 gate** | HIGH | TODO | — | R-026 | R-M1 | SEQUENTIAL | SPEC-VALIDATE + REVIEW | files: docs/interfaces/renderer-interface-spec.md |

---

## Milestone: R-M2 — Directional Light + Lambertian Shading (~45 min)

**Expected outcome**: Procedural scene lit by a single directional light with Lambertian diffuse shading. Camera orbit shows correct bright/dark shading response. Debug toggle cycles Unlit ↔ Lambertian.

| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
|---|---|---|---|---|---|---|---|---|---|
| R-028 | Author `shaders/renderer/lambertian.glsl`: `@vs lambertian_vs` — uniform `vs_params { mat4 mvp; mat4 model; }`; pass `v_normal = normalize(mat3(model) * normal)`; `@fs lambertian_fs` — uniform `fs_params { vec3 light_dir; vec3 light_color; float light_intensity; vec4 base_color; }`; `ndotl = max(dot(v_normal, normalize(light_dir)), 0.0)`; output `base_color.rgb * light_color * intensity * ndotl`; `@program lambertian` | MED | TODO | — | R-027 | R-M2 | PG-RM2-A | SELF-CHECK | files: shaders/renderer/lambertian.glsl |
| R-029 | Create `src/renderer/pipeline_lambertian.cpp/h`: `sg_pipeline create_pipeline_lambertian()` — include `lambertian.glsl.h`; same Vertex layout and depth state as unlit; on failure return magenta; expose getter `get_lambertian_pipeline()` | MED | TODO | — | R-027 | R-M2 | PG-RM2-B | SELF-CHECK | files: src/renderer/pipeline_lambertian.cpp, src/renderer/pipeline_lambertian.h |
| R-030 | Implement `renderer_set_directional_light()` in `renderer.cpp`: store DirectionalLight in state; normalize direction on store (zero-vector → `{0,-1,0}` fallback); augment `end_frame()` to upload light uniforms when Lambertian pipeline is active | MED | TODO | — | R-027 | R-M2 | PG-RM2-B | SELF-CHECK | files: src/renderer/renderer.cpp |
| R-031 | Extend pipeline dispatch in `renderer_end_frame()` in `renderer.cpp`: if `material.shading_model == Lambertian` bind `pipeline_lambertian` and upload `vs_params` (mvp+model) + `fs_params` (light+base_color); else bind `pipeline_unlit`; implement `renderer_make_lambertian_material()` fully | MED | TODO | — | R-028, R-029, R-030 | R-M2 | SEQUENTIAL | SELF-CHECK | files: src/renderer/renderer.cpp |
| R-032 | Add runtime debug toggle in `app/main.cpp`: pressing `L` cycles materials between Unlit/Lambertian shading models for milestone verification | LOW | TODO | — | R-031 | R-M2 | SEQUENTIAL | NONE | files: src/renderer/app/main.cpp |
| R-033 | Update `app/main.cpp` for R-M2 demo: set directional light `dir={1,-1,-0.5}`, white, intensity=1.5; animate camera orbiting scene; render mix of Unlit and Lambertian objects; verify shading response — **human sign-off SC-003** | MED | TODO | — | R-031, R-032 | R-M2 | SEQUENTIAL | SPEC-VALIDATE | files: src/renderer/app/main.cpp |

---

## Milestone: R-M3 — Skybox + Line-Quads + Alpha Blending (~45 min) 🏁 MVP COMPLETE

**Expected outcome**: Starfield skybox behind opaque geometry; camera-facing billboard laser quads alpha-composite correctly; ImGui HUD renders on top. Renderer MVP complete.

| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
|---|---|---|---|---|---|---|---|---|---|
| R-034 | Author `shaders/renderer/skybox.glsl`: `@vs skybox_vs` — input `vec3 position`; uniform `vs_params { mat4 view_proj; }`; `gl_Position = (view_proj * vec4(position,1)).xyww` (w=w trick, depth=1.0); output `v_texcoord=position`; `@fs skybox_fs` — `uniform samplerCube skybox_tex`; `frag_color = texture(skybox_tex, v_texcoord)`; `@program skybox` | MED | TODO | — | R-027 | R-M3 | PG-RM3-A | SELF-CHECK | files: shaders/renderer/skybox.glsl |
| R-035 | Author `shaders/renderer/line_quad.glsl`: `@vs line_quad_vs` — inputs `vec3 position`, `vec4 color`; uniform `vs_params { mat4 vp; }`; `@fs line_quad_fs` — input `vec4 v_color`; `frag_color = v_color`; `@program line_quad`; note: corner positions pre-computed on CPU — no geometry shader | MED | TODO | — | R-027 | R-M3 | PG-RM3-A | SELF-CHECK | files: shaders/renderer/line_quad.glsl |
| R-036 | Create `src/renderer/skybox.cpp/h`: implement `renderer_upload_cubemap(faces,w,h,ch)` — `sg_image` with `SG_IMAGETYPE_CUBE`, 6 face subimage data; return `RendererTextureHandle`; implement `renderer_set_skybox(handle)`; implement `draw_skybox_pass()` for `end_frame()`: 1-unit cube (36 indices, 24 vertices), depth write OFF, depth compare `SG_COMPAREFUNC_LESS_EQUAL`, cull front, view-proj uses rotation-only view matrix (zero translation column) | HIGH | TODO | — | R-027 | R-M3 | PG-RM3-B | SELF-CHECK | files: src/renderer/skybox.cpp, src/renderer/skybox.h |
| R-037 | Create `src/renderer/debug_draw.cpp/h`: implement `renderer_enqueue_line_quad()` CPU billboard — `dir=normalize(p1-p0)`; extract `cam_pos` from camera view matrix inverse (last column); `right = normalize(cross(dir, to_cam)) * width * 0.5`; generate 4 corners + 6 indices; append to `line_quad_queue` | MED | TODO | — | R-027 | R-M3 | PG-RM3-B | SELF-CHECK | files: src/renderer/debug_draw.cpp, src/renderer/debug_draw.h |
| R-038 | Create transparent pipeline and line-quad pipeline in `renderer.cpp`: `pipeline_transparent` — full Vertex layout, `SG_BLENDFACTOR_SRC_ALPHA / ONE_MINUS_SRC_ALPHA`, depth write OFF, depth test ON; `pipeline_line_quad` — position+color layout, same blend state; `pipeline_skybox` — position only, depth write OFF, front-face cull reversed; on any failure fall back to magenta | MED | TODO | — | R-027 | R-M3 | PG-RM3-C | SELF-CHECK | files: src/renderer/renderer.cpp |
| R-039 | Extend `renderer_end_frame()` pass order in `renderer.cpp`: (1) skybox pass if `skybox_handle.id != 0`; (2) opaque draws (Unlit+Lambertian); (3) transparent draws where `material.alpha < 1.0`; (4) line quad draws from `line_quad_queue`; (5) `simgui_render()` | HIGH | TODO | — | R-036, R-037, R-038 | R-M3 | SEQUENTIAL | SELF-CHECK | files: src/renderer/renderer.cpp |
| R-040 | Integrate `simgui_new_frame()` into `renderer_begin_frame()` in `renderer.cpp`: call `simgui_new_frame({.width=sapp_width(), .height=sapp_height(), .delta_time=sapp_frame_duration(), .dpi_scale=sapp_dpi_scale()})` | MED | TODO | — | R-038 | R-M3 | SEQUENTIAL | SELF-CHECK | files: src/renderer/renderer.cpp |
| R-041 | Update `app/main.cpp` for R-M3 MVP demo: load cubemap from `ASSET_ROOT "/skybox/"` (6 PNG faces); add 3 laser line-quads with `alpha=0.6`; add ImGui HUD showing FPS + draw count; render opaque+transparent geometry mix | MED | TODO | — | R-039, R-040 | R-M3 | SEQUENTIAL | SELF-CHECK | files: src/renderer/app/main.cpp |
| R-042 | Build `renderer_app`; human behavioral check: skybox as background, laser quads alpha-composite, ImGui HUD on top, no depth/blend artifacts — **SC-004 sign-off; Renderer MVP complete** | MED | TODO | — | R-041 | R-M3 | SEQUENTIAL | SPEC-VALIDATE + REVIEW | files: (verification only) |

---

## Milestone: R-M4 — Blinn-Phong + Diffuse Textures ✨ Desirable (only after MVP ≤ 3 h mark)

**Expected outcome**: Textured mesh and specular-highlighted surface render correctly side-by-side under the directional light (SC-008).

| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
|---|---|---|---|---|---|---|---|---|---|
| R-043 | Author `shaders/renderer/blinnphong.glsl`: `@vs blinnphong_vs` — uniform `vs_params { mat4 mvp; mat4 model; mat4 view; }`; output `v_normal`, `v_uv`, `v_world_pos`; `@fs blinnphong_fs` — `uniform sampler2D albedo_tex`; uniform `fs_params { vec3 light_dir; vec3 light_color; float light_intensity; vec4 base_color; float shininess; float use_texture; }`; Blinn-Phong: diffuse + `pow(max(dot(v_normal,half),0), shininess)` specular; `@program blinnphong` | MED | TODO | — | R-042 | R-M4 | PG-RM4-A | SELF-CHECK | files: shaders/renderer/blinnphong.glsl |
| R-044 | Create `src/renderer/pipeline_blinnphong.cpp/h`: `sg_pipeline create_pipeline_blinnphong()` — include `blinnphong.glsl.h`; full Vertex layout; depth test ON + write ON, cull back; on failure return magenta | MED | TODO | — | R-042 | R-M4 | PG-RM4-B | SELF-CHECK | files: src/renderer/pipeline_blinnphong.cpp, src/renderer/pipeline_blinnphong.h |
| R-045 | Create `src/renderer/texture.cpp/h`: `renderer_upload_texture_2d(pixels,w,h,ch)` — `sg_image` with `SG_PIXELFORMAT_RGBA8` (convert RGB→RGBA if ch==3); on failure return `{0}`; store in `texture_table[next_texture_id++]`; `renderer_upload_texture_from_file(path)` — `#define STB_IMAGE_IMPLEMENTATION` (one TU only) + `stbi_load(path,&w,&h,&ch,4)`; call `renderer_upload_texture_2d`; `stbi_image_free` | MED | TODO | — | R-042 | R-M4 | PG-RM4-C | SELF-CHECK | files: src/renderer/texture.cpp, src/renderer/texture.h |
| R-046 | Extend opaque draw dispatch in `renderer.cpp` for `ShadingModel::BlinnPhong`: bind `pipeline_blinnphong`; if `material.texture.id != 0` bind `texture_table[material.texture.id]` to sampler slot 0 and set `use_texture=1.0`, else `use_texture=0.0`; upload `shininess`; implement `renderer_make_blinnphong_material()` fully | MED | TODO | — | R-043, R-044, R-045 | R-M4 | SEQUENTIAL | SELF-CHECK | files: src/renderer/renderer.cpp |
| R-047 | Update `app/main.cpp` for R-M4 demo: `renderer_upload_texture_from_file(ASSET_ROOT "/textures/asteroid.png")`; one Blinn-Phong material with texture, one without; two spheres side-by-side under directional light; verify texture mapping + specular highlight — **SC-008 sign-off** | MED | TODO | — | R-046 | R-M4 | SEQUENTIAL | SPEC-VALIDATE | files: src/renderer/app/main.cpp |

---

## Milestone: R-M5 — Sorted Transparency + Capsule Mesh ✨ Desirable

**Expected outcome**: Transparent objects composite without ordering artifacts; capsule mesh builder available for ship/enemy shapes.  
**Activation**: Only after R-M3 MVP merged and time permits. Lower priority than R-M6.

| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
|---|---|---|---|---|---|---|---|---|---|
| R-058 | Implement `renderer_make_capsule_mesh(radius, height, subdivisions)` in `mesh_builders.cpp`: cylinder body + two hemisphere caps; compute Position, Normal, UV (longitudinal), Tangent; call `renderer_upload_mesh()`; target subdivisions=8 | MED | TODO | — | R-042 | R-M5 | PG-RM5-A | SELF-CHECK | files: src/renderer/mesh_builders.cpp |
| R-059 | Implement back-to-front sort of transparent draw queue in `renderer_end_frame()` in `renderer.cpp`: after opaque/transparent split, sort transparent subset by **descending** view-space Z (extract world pos from transform column 3, transform to view space with `view × pos`, sort farthest-first) | MED | TODO | — | R-042 | R-M5 | PG-RM5-B | SELF-CHECK | files: src/renderer/renderer.cpp |

---

## Milestone: R-M6 — Frustum Culling + Front-to-Back Sort ✨ Desirable (conditional)

**Expected outcome**: GPU draw call count drops measurably for off-frustum objects. Opaque queue sorted front-to-back.  
**Activation**: Only if visible lag/judder observed during R-M3 human behavioral check.

| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
|---|---|---|---|---|---|---|---|---|---|
| R-048 | Implement frustum plane extraction in `renderer.cpp`: from combined VP matrix extract 6 planes via Gribb-Hartmann method (`plane[i] = row3 ± row_i` for left/right/bottom/top/near/far); store as 6 `glm::vec4` (xyz=normal, w=d); call once per `end_frame()` after camera is set | MED | TODO | — | R-042 | R-M6 | SEQUENTIAL | SELF-CHECK | files: src/renderer/renderer.cpp |
| R-049 | Implement AABB-vs-frustum culling in `end_frame()` opaque loop in `renderer.cpp`: compute world-space AABB from mesh bounds (store at upload in R-020) transformed by world matrix; test AABB against all 6 planes; skip `sg_draw` if fully outside; log cull count once per 60 frames | HIGH | TODO | — | R-048 | R-M6 | SEQUENTIAL | SELF-CHECK | files: src/renderer/renderer.cpp |
| R-050 | Implement front-to-back sort of opaque draw_queue in `renderer.cpp` after frustum cull: extract world-space pos from column 3 of world matrix; transform to view space; `std::sort` opaque entries by ascending view-space Z (nearest first); do NOT sort transparent queue (that is R-059) | MED | TODO | — | R-048 | R-M6 | SEQUENTIAL | SELF-CHECK | files: src/renderer/renderer.cpp |
| R-051 | Update `app/main.cpp` stress mode: 50 objects inside + outside frustum at random sphere positions; ImGui window showing `draws submitted / draws culled / total enqueued`; `C` key toggles culling on/off | LOW | TODO | — | R-049, R-050 | R-M6 | SEQUENTIAL | SELF-CHECK | files: src/renderer/app/main.cpp |

---

## Cross-Cutting: Tests, Docs, Interface Freeze

| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
|---|---|---|---|---|---|---|---|---|---|
| R-052 | Populate `_coordination/renderer/TASKS.md` with all milestone rows in blueprint schema (this task) | LOW | DONE | copilot@laptopA | — | SETUP | SEQUENTIAL | NONE | files: _coordination/renderer/TASKS.md |
| R-053 | Write Catch2 tests in `src/renderer/tests/test_mesh_builders.cpp`: sphere subdivisions=4 → expected vertex count `(4+1)*(4+1)`, index count `4*4*6`; cube → 24 vertices, 36 indices; all sphere normals unit-length; all cube positions within `[-half_extent, half_extent]` | MED | TODO | — | R-021, R-022 | R-M1 | PG-FINAL-A | SELF-CHECK | files: src/renderer/tests/test_mesh_builders.cpp |
| R-054 | Write Catch2 tests in `src/renderer/tests/test_handles.cpp`: `renderer_handle_valid({0})` returns false; `renderer_handle_valid({1})` returns true; enqueue with invalid handle does not increment `draw_count`; zero-length line quad does not increment `line_quad_count` | MED | TODO | — | R-009 | SETUP | PG-FINAL-A | SELF-CHECK | files: src/renderer/tests/test_handles.cpp |
| R-055 | Verify `docs/interfaces/renderer-interface-spec.md` FROZEN v1.0 status matches as-built code after R-M1; confirm no signature drift; add implementation-verified note if needed; announce to engine workstream so engine SpecKit can begin — **NOTE: spec is already FROZEN v1.0 from SpecKit phase; this is post-implementation verification, not initial freeze** | HIGH | TODO | — | R-027 | R-M1 | SEQUENTIAL | SPEC-VALIDATE + REVIEW | files: docs/interfaces/renderer-interface-spec.md |
| R-056 | Finalize `docs/architecture/renderer-architecture.md` after R-M3: fill pipeline inventory table with measured creation results; confirm frame sequence diagram matches as-built code; update module-to-file mapping if any files changed | LOW | TODO | — | R-042 | R-M3 | SEQUENTIAL | SELF-CHECK | files: docs/architecture/renderer-architecture.md |
| R-057 | Update `docs/planning/speckit/renderer/tasks.md`: add pointer to `_coordination/renderer/TASKS.md` as live board and to `specs/001-sokol-render-engine/tasks.md` as SpecKit source; mark planning outputs complete | LOW | TODO | — | R-052 | SETUP | SEQUENTIAL | NONE | files: docs/planning/speckit/renderer/tasks.md |
