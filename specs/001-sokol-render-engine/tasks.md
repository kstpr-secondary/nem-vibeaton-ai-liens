# Tasks: Sokol Render Engine

**Input**: Design documents from `/specs/001-sokol-render-engine/`
**Prerequisites**: plan.md ✅, spec.md ✅, research.md ✅, data-model.md ✅, contracts/renderer-api.md ✅

**Organization**: Tasks are grouped by user story / milestone to enable independent implementation and testing of each increment. Each phase maps to one renderer milestone (R-M0 through R-M6).

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (touches disjoint files, no unmet dependencies within the same group)
- **[US#]**: Which user story / milestone this task belongs to
- Include exact file paths in descriptions

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: CMake wiring, FetchContent deps, generated headers, public API scaffold, mock stubs. Must complete before any milestone work begins.

**⚠️ CRITICAL**: No user story work can begin until this phase is complete.

- [ ] T001 Wire `sokol-shdc` CMake custom command in `CMakeLists.txt`: add `add_custom_command` per shader file routing `shaders/renderer/<name>.glsl` → `${CMAKE_BINARY_DIR}/generated/shaders/<name>.glsl.h`; add `VIBEATON_REQUIRE_SOKOL_SHDC` ON-default guard that treats missing `sokol-shdc` on PATH as configure error; add generated dir to `renderer` target include path
- [ ] T002 [P] Create `src/renderer/renderer.h` public API scaffold with all types and function declarations from `specs/001-sokol-render-engine/contracts/renderer-api.md` (handles, RendererConfig, Vertex, ShadingModel, Material, DirectionalLight, RendererCamera, InputCallback, all free-function declarations) — bodies are empty stubs at this stage
- [ ] T003 [P] Verify `cmake/paths.h.in` exists and `CMakeLists.txt` generates `${CMAKE_BINARY_DIR}/generated/paths.h` with `PROJECT_SOURCE_ROOT` and `ASSET_ROOT` macros; confirm `renderer` target has PUBLIC include path to generated dir so downstream workstreams inherit it
- [ ] T004 [P] Confirm `stb_image` FetchContent entry exists in `CMakeLists.txt` with pinned version; verify `STB_IMAGE_IMPLEMENTATION` is NOT defined in any header — it will be owned by one `.cpp` only (T046); confirm PUBLIC include path propagation to `engine` and `game` targets
- [ ] T005 [P] Create `src/renderer/mocks/renderer_mock.cpp`: no-op implementations of every free function in `renderer.h`; handle-returning functions return `{1}` (valid sentinel); void functions are empty bodies; confirm `USE_RENDERER_MOCKS=ON` CMake flag swaps this in place of the real `renderer` static lib
- [ ] T006 Create empty `.cpp`/`.h` file pairs per the module split in `plan.md` to establish file ownership for parallel work: `src/renderer/renderer.cpp`, `src/renderer/pipeline_unlit.cpp`, `src/renderer/pipeline_unlit.h`, `src/renderer/pipeline_lambertian.cpp`, `src/renderer/pipeline_lambertian.h`, `src/renderer/pipeline_blinnphong.cpp`, `src/renderer/pipeline_blinnphong.h`, `src/renderer/mesh_builders.cpp`, `src/renderer/mesh_builders.h`, `src/renderer/skybox.cpp`, `src/renderer/skybox.h`, `src/renderer/debug_draw.cpp`, `src/renderer/debug_draw.h`, `src/renderer/texture.cpp`, `src/renderer/texture.h`, `src/renderer/app/main.cpp`

**Checkpoint**: `cmake --build build --target renderer_app` compiles (stub bodies, no rendering yet).

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Internal state machine, draw queue infrastructure, magenta error pipeline helper. Shared by all milestone phases.

**⚠️ CRITICAL**: All US phases depend on this foundation.

- [ ] T007 Define renderer internal state struct in `src/renderer/renderer.cpp` (file-private, not in header): `DrawCommand draw_queue[1024]`, `int draw_count`, `LineQuadCommand line_quad_queue[256]`, `int line_quad_count`, `RendererCamera camera`, `DirectionalLight light`, `RendererTextureHandle skybox_handle`, `sg_pipeline pipeline_unlit/lambertian/blinnphong/transparent/line_quad/skybox/magenta`, `sg_buffer mesh_vbufs[512]`, `sg_buffer mesh_ibufs[512]`, `sg_image texture_table[256]`, `uint32_t next_mesh_id`, `uint32_t next_texture_id`, `bool frame_active`, `InputCallback input_cb`, `void* input_cb_userdata`
- [ ] T008 [P] Implement `renderer_begin_frame()` skeleton in `src/renderer/renderer.cpp`: assert `!frame_active`; set `frame_active = true`; reset `draw_count = 0` and `line_quad_count = 0`; begin sokol render pass with configured clear color
- [ ] T009 [P] Implement `renderer_enqueue_draw()` guard logic in `src/renderer/renderer.cpp`: if `!frame_active` silently return; if `draw_count >= 1024` emit debug log and return; if `!renderer_handle_valid(mesh)` silently return; else store DrawCommand at `draw_queue[draw_count++]`; similarly implement `renderer_enqueue_line_quad()` guard (skip zero-length, skip width ≤ 0, store in `line_quad_queue`)
- [ ] T010 Implement magenta error pipeline creation helper (file-private) in `src/renderer/renderer.cpp`: hardcoded inline GLSL (solid `vec4(1,0,1,1)` fragment output) via `sg_make_shader` with embedded source strings for GL 3.3; if even this fails, log fatal and store invalid handle; called from `renderer_init()` before any other pipeline
- [ ] T011 Create `src/renderer/app/main.cpp` `renderer_app` driver stub: `sapp_desc sokol_main()` returning a descriptor with empty `init_cb`, `frame_cb`, `cleanup_cb`, `event_cb`; title from RendererConfig; this is the acceptance vehicle updated at each milestone

**Checkpoint**: `cmake --build build --target renderer_app` links; binary launches and immediately exits cleanly.

---

## Phase 3: User Story 1 — Window Init + Input Pump (Priority: P1, R-M0) 🎯

**Goal**: Open a configured window, clear to a color, forward input events, shut down cleanly.

**Independent Test (SC-001)**: Launch `renderer_app`; window opens to configured clear color; keyboard/mouse events print to console log; process exits cleanly on window close.

**FR coverage**: FR-001, FR-002, FR-003, FR-017 (magenta), FR-018

- [ ] T012 [US1] Implement `renderer_init()` in `src/renderer/renderer.cpp`: call `sg_setup({.context = sapp_sgcontext()})` with `SOKOL_GLCORE33` backend; store `RendererConfig` clear color; call magenta pipeline helper (T010); call `simgui_setup({.sample_count = sapp_sample_count()})` for Dear ImGui backend
- [ ] T013 [P] [US1] Implement `renderer_set_input_callback()` in `src/renderer/renderer.cpp`: store `cb` and `user_data`; implement sokol_app `event_cb` that calls `simgui_handle_event(e)` then, if `input_cb != nullptr`, calls `input_cb(e, input_cb_userdata)`
- [ ] T014 [P] [US1] Implement `renderer_run()` in `src/renderer/renderer.cpp`: call `sapp_run(desc)` where desc wires `init_cb = renderer_internal_init`, `frame_cb = renderer_internal_frame`, `cleanup_cb = renderer_internal_cleanup`, `event_cb = renderer_internal_event`, `width/height/title` from stored config
- [ ] T015 [US1] Implement `renderer_shutdown()` in `src/renderer/renderer.cpp`: call `simgui_shutdown()`, `sg_shutdown()`, `sapp_quit()`; set `frame_active = false`; zero all pipeline handles
- [ ] T016 [US1] Update `src/renderer/app/main.cpp`: create `RendererConfig` with title "R-M0 Bootstrap", clear color `(0.05, 0.05, 0.10, 1.0)`; register an input callback that prints event type to `printf`; call `renderer_init(cfg)`, `renderer_set_input_callback(cb, nullptr)`, `renderer_run()`
- [ ] T017 [US1] `cmake --build build --target renderer_app`; launch; verify: window opens, framebuffer clears to configured color, keyboard/mouse events print to console, process exits cleanly — human sign-off required for SC-001

---

## Phase 4: User Story 2 — Unlit 3D Scene Rendering (Priority: P1, R-M1) 🎯 Sync Point

**Goal**: Upload procedural geometry, set perspective camera, enqueue flat-color draw calls, render 10+ colored objects.

**Independent Test (SC-002)**: `renderer_app` displays at least 10 colored spheres/cubes at varied positions under a fixed perspective camera with no visual artifacts.

**FR coverage**: FR-004, FR-005, FR-006, FR-007, FR-008

- [ ] T018 [US2] Author `shaders/renderer/unlit.glsl` with sokol-shdc annotations: `@vs unlit_vs` — inputs: `in vec3 position, normal; in vec2 uv; in vec3 tangent`; uniform block `vs_params { mat4 mvp; }`; `@fs unlit_fs` — uniform block `fs_params { vec4 base_color; }`; outputs `out vec4 frag_color = base_color`; `@program unlit unlit_vs unlit_fs`
- [ ] T019 [P] [US2] Create `src/renderer/pipeline_unlit.cpp/h`: `sg_pipeline create_pipeline_unlit()` — include generated `unlit.glsl.h`; call `shd_unlit_shader_desc(sg_query_backend())`; configure `sg_pipeline_desc` with `Vertex` layout (position=float3, normal=float3, uv=float2, tangent=float3), depth test ON, depth write ON, cull back; on `sg_make_pipeline` failure log error and return magenta pipeline handle
- [ ] T020 [P] [US2] Implement `renderer_upload_mesh()` in `src/renderer/mesh_builders.cpp`: create `sg_buffer` for vertices (`SG_BUFFERTYPE_VERTEXBUFFER`) and indices (`SG_BUFFERTYPE_INDEXBUFFER`) with `SG_USAGE_IMMUTABLE`; on failure return `{0}`; store in `mesh_vbufs[next_mesh_id]` / `mesh_ibufs[next_mesh_id]`; return `{next_mesh_id++}`
- [ ] T021 [P] [US2] Implement `renderer_make_sphere_mesh()` in `src/renderer/mesh_builders.cpp`: UV sphere with `subdivisions` lat/lon rings; compute Position, Normal (=normalize(position)), UV (lon/lat mapped to [0,1]), Tangent (= normalize(cross(up, normal))); pack into `Vertex[]`; build index buffer for triangle strips; call `renderer_upload_mesh()`
- [ ] T022 [P] [US2] Implement `renderer_make_cube_mesh()` in `src/renderer/mesh_builders.cpp`: 24 vertices (4 per face × 6 faces) with correct per-face normals, UVs, and tangents (tangent = face's local X axis); 36 indices (2 triangles per face); call `renderer_upload_mesh()`
- [ ] T023 [US2] Implement `renderer_set_camera()` in `src/renderer/renderer.cpp`: store `RendererCamera` in internal state; compute `vp = projection × view` as `glm::mat4` (using `glm::make_mat4`) each frame; if not called before `end_frame()`, use identity + log debug warning
- [ ] T024 [US2] Implement opaque draw dispatch in `renderer_end_frame()` in `src/renderer/renderer.cpp`: after skybox placeholder (none yet), iterate `draw_queue[0..draw_count-1]`; for each: select `pipeline_unlit` (only model at this stage); compute `mvp = vp × glm::make_mat4(cmd.transform)`; upload `vs_params` uniform; upload `fs_params` with `base_color`; bind vertex + index buffers from `mesh_vbufs/ibufs[cmd.mesh.id]`; call `sg_draw`
- [ ] T025 [P] [US2] Implement `renderer_make_unlit_material()` and `renderer_make_lambertian_material()` helper bodies in `src/renderer/renderer.cpp` (Lambertian body is stub returning Unlit at this stage)
- [ ] T026 [US2] Update `src/renderer/app/main.cpp` for R-M1 demo: call `renderer_make_sphere_mesh` + `renderer_make_cube_mesh`; in frame callback set a fixed perspective camera (45° FOV, aspect from config, near=0.1, far=100); enqueue 10+ colored draw calls at varied positions, rotations, scales using `renderer_enqueue_draw`; run
- [ ] T027 [US2] `cmake --build build --target renderer_app renderer_tests`; verify 10+ colored primitives render correctly; after human sign-off: update `docs/interfaces/renderer-interface-spec.md` status to `FROZEN — v1.0`; announce freeze to engine workstream

**Checkpoint (Milestone Sync)**: Interface frozen. Engine workstream may now begin against `renderer.h` + mocks.

---

## Phase 5: User Story 3 — Directional Light + Lambertian Shading (Priority: P2, R-M2)

**Goal**: Add directional light uniform and Lambertian diffuse shader; pipeline selector switches between Unlit and Lambertian per-material.

**Independent Test (SC-003)**: `renderer_app` procedural scene under one directional light; camera orbit shows correct shading response (bright facing, dark away from light).

**FR coverage**: FR-009, FR-010

- [ ] T028 [US3] Author `shaders/renderer/lambertian.glsl`: `@vs lambertian_vs` — uniform block `vs_params { mat4 mvp; mat4 model; }`; pass `v_normal = normalize(mat3(model) * normal)` to fragment; `@fs lambertian_fs` — uniform block `fs_params { vec3 light_dir; vec3 light_color; float light_intensity; vec4 base_color; }`; compute `float ndotl = max(dot(v_normal, normalize(light_dir)), 0.0)`; output `frag_color = vec4(base_color.rgb * light_color * light_intensity * ndotl, base_color.a)`; `@program lambertian lambertian_vs lambertian_fs`
- [ ] T029 [P] [US3] Create `src/renderer/pipeline_lambertian.cpp/h`: `sg_pipeline create_pipeline_lambertian()` — include generated `lambertian.glsl.h`; same `Vertex` layout and depth state as unlit pipeline; on failure return magenta pipeline; expose `get_lambertian_pipeline()` getter
- [ ] T030 [P] [US3] Implement `renderer_set_directional_light()` in `src/renderer/renderer.cpp`: store `DirectionalLight` in internal state; normalize direction on store (zero-vector falls back to `{0,-1,0}`); add to `end_frame()` upload: pass `light_dir`, `light_color`, `light_intensity` into `fs_params` when Lambertian pipeline is active
- [ ] T031 [US3] Extend pipeline dispatch in `renderer_end_frame()` in `src/renderer/renderer.cpp`: if `material.shading_model == ShadingModel::Lambertian` bind `pipeline_lambertian` and upload both `vs_params` (mvp + model) and `fs_params` (light + base_color); else bind `pipeline_unlit` as before; implement `renderer_make_lambertian_material()` fully
- [ ] T032 [P] [US3] Add runtime debug toggle in `src/renderer/app/main.cpp`: pressing `L` key cycles between Unlit/Lambertian shading model on all materials; useful for milestone verification
- [ ] T033 [US3] Update `src/renderer/app/main.cpp` for R-M2 demo: set directional light direction `{1,-1,-0.5}`, white color, intensity 1.5; animate camera orbiting scene; render mix of unlit and Lambertian objects; verify shading response — human sign-off for SC-003

---

## Phase 6: User Story 4 — Skybox + Laser Line-Quads + Alpha Blending (Priority: P2, R-M3) 🏁 MVP

**Goal**: Cubemap skybox behind all geometry, camera-facing billboard line-quads for lasers, basic alpha blending for transparent draws, Dear ImGui HUD pass.

**Independent Test (SC-004)**: Starfield skybox behind colored opaque objects; laser line-quads with alpha composite in front of background but behind nearer solid objects — human visual check.

**FR coverage**: FR-011, FR-012, FR-013, FR-014

- [ ] T034 [US4] Author `shaders/renderer/skybox.glsl`: `@vs skybox_vs` — input `in vec3 position` only; uniform block `vs_params { mat4 view_proj; }`; compute `vec4 pos = view_proj * vec4(position, 1.0)`; set `gl_Position = pos.xyww` (w=w trick ensures depth=1.0 after perspective divide); output `v_texcoord = position` as cube direction; `@fs skybox_fs` — `uniform samplerCube skybox_tex`; output `frag_color = texture(skybox_tex, v_texcoord)`; `@program skybox skybox_vs skybox_fs`
- [ ] T035 [P] [US4] Author `shaders/renderer/line_quad.glsl`: `@vs line_quad_vs` — input `in vec3 position; in vec4 color`; uniform `vs_params { mat4 vp; }`; `@fs line_quad_fs` — input `in vec4 v_color`; output `frag_color = v_color`; `@program line_quad line_quad_vs line_quad_fs` (corner positions pre-computed on CPU, no geometry shader needed)
- [ ] T036 [US4] Create `src/renderer/skybox.cpp/h`: implement `renderer_upload_cubemap(faces, w, h, ch)` — create `sg_image` with `image_type = SG_IMAGETYPE_CUBE`, 6 face subimage data; return `RendererTextureHandle`; implement `renderer_set_skybox(handle)` storing handle in internal state; implement `draw_skybox_pass()` called first in `end_frame()`: create 1-unit cube mesh internally (14-vertex strip or 36-index); bind skybox pipeline (depth write OFF, depth test ON, cull front), set view_proj with rotation-only view matrix (strip translation from camera view matrix)
- [ ] T037 [P] [US4] Create `src/renderer/debug_draw.cpp/h`: implement `renderer_enqueue_line_quad()` CPU billboard computation — `vec3 dir = normalize(p1-p0)`; `vec3 cam_pos` from camera view matrix inverse (extract last column); `vec3 to_cam = normalize(cam_pos - midpoint)`; `vec3 right = normalize(cross(dir, to_cam)) * width * 0.5`; generate 4 corners; 6 indices (2 triangles); append to `line_quad_queue`
- [ ] T038 [P] [US4] Create transparent pipeline and line-quad pipeline in `src/renderer/renderer.cpp`: `pipeline_transparent` — same Vertex layout; `sg_blend_state` with `SG_BLENDFACTOR_SRC_ALPHA / SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA`; depth write OFF, depth test ON; `pipeline_line_quad` — position+color vertex layout (simpler than full Vertex); same blend state; `pipeline_skybox` — position-only; depth write OFF; front-face cull reversed; on creation failure each falls back to magenta
- [ ] T039 [US4] Extend `renderer_end_frame()` pass order in `src/renderer/renderer.cpp`: (1) skybox pass via `draw_skybox_pass()` if `skybox_handle.id != 0`; (2) opaque draws (Unlit + Lambertian); (3) transparent draws — iterate `draw_queue` where `material.alpha < 1.0`, bind `pipeline_transparent`, draw; (4) line quad draws from `line_quad_queue` with `pipeline_line_quad`; (5) `simgui_render()`
- [ ] T040 [P] [US4] Integrate `simgui_new_frame()` call into `renderer_begin_frame()` in `src/renderer/renderer.cpp`: call `simgui_new_frame({.width=sapp_width(), .height=sapp_height(), .delta_time=sapp_frame_duration(), .dpi_scale=sapp_dpi_scale()})`
- [ ] T041 [US4] Update `src/renderer/app/main.cpp` for R-M3 MVP demo: load cubemap from `ASSET_ROOT "/skybox/"` faces (or generate a simple solid-color cubemap from code if asset files not ready); add 3 laser line-quads with alpha 0.6; add ImGui HUD showing FPS and draw count; render opaque + transparent geometry mix
- [ ] T042 [US4] `cmake --build build --target renderer_app`; human behavioral check: skybox visible as background, laser quads alpha-composite correctly, ImGui HUD renders on top — SC-004 sign-off; **Renderer MVP complete**

---

## Phase 7: User Story 5 — Blinn-Phong Shading + Diffuse Textures (Priority: P3, R-M4) ✨ Desirable

**Goal**: 2D texture upload, Blinn-Phong pipeline with specular highlights and texture sampling.

**Independent Test (SC-008)**: `renderer_app` shows a textured mesh and specular-highlighted surface side by side under the directional light — distinct visible highlight and correct texture mapping.

**FR coverage**: FR-015, FR-016

- [ ] T043 [US5] Author `shaders/renderer/blinnphong.glsl`: `@vs blinnphong_vs` — uniform `vs_params { mat4 mvp; mat4 model; mat4 view; }`; output `v_normal` (model-space), `v_uv`, `v_world_pos`; `@fs blinnphong_fs` — `uniform sampler2D albedo_tex`; uniform `fs_params { vec3 light_dir; vec3 light_color; float light_intensity; vec4 base_color; float shininess; float use_texture; }`; compute Lambertian diffuse + Blinn-Phong specular (`half = normalize(light_dir + view_dir)`; `spec = pow(max(dot(v_normal, half), 0), shininess)`); `albedo = use_texture > 0.5 ? texture(albedo_tex, v_uv).rgb : base_color.rgb`; `@program blinnphong blinnphong_vs blinnphong_fs`
- [ ] T044 [P] [US5] Create `src/renderer/pipeline_blinnphong.cpp/h`: `sg_pipeline create_pipeline_blinnphong()` — include `blinnphong.glsl.h`; full `Vertex` layout; depth test ON, write ON, cull back; on failure return magenta; expose getter for use in dispatch
- [ ] T045 [P] [US5] Create `src/renderer/texture.cpp/h`: implement `renderer_upload_texture_2d(pixels, w, h, ch)` — `sg_image_desc` with `SG_PIXELFORMAT_RGBA8` (convert RGB→RGBA if `ch==3`); `sg_make_image`; on failure return `{0}`; store in `texture_table[next_texture_id]`; return `{next_texture_id++}`; implement `renderer_upload_texture_from_file(path)` — include `<stb_image.h>`; `stbi_load(path, &w, &h, &ch, 4)`; call `renderer_upload_texture_2d`; `stbi_image_free(data)` (define `STB_IMAGE_IMPLEMENTATION` at top of this file — one TU only)
- [ ] T046 [P] [US5] Extend opaque draw dispatch in `src/renderer/renderer.cpp` for `ShadingModel::BlinnPhong`: bind `pipeline_blinnphong`; if `material.texture.id != 0` bind `texture_table[material.texture.id]` to sampler slot 0 and set `use_texture=1.0` in fs_params; else set `use_texture=0.0`; upload `shininess`; implement `renderer_make_blinnphong_material()` helper fully
- [ ] T047 [US5] Update `src/renderer/app/main.cpp` for R-M4 demo: upload a PNG texture via `renderer_upload_texture_from_file(ASSET_ROOT "/textures/asteroid.png")`; create one Blinn-Phong material with texture, one with only base color; render two spheres side by side under directional light; verify texture mapping + specular highlight — human sign-off SC-008

---

## Phase 8: User Story 6 — Frustum Culling + Front-to-Back Sort (Priority: P3, R-M6) ✨ Desirable

**Goal**: Reduce GPU submissions for off-screen objects; sort opaque queue front-to-back.

**Activation condition**: Only implement if visible lag/judder is observed during R-M3 human behavioral check.

**Independent Test (SC-009)**: 50+ object stress scene; GPU draw count drops for off-frustum objects; FPS improves measurably.

**FR coverage**: FR-019, FR-020

- [ ] T048 [US6] Implement frustum plane extraction in `src/renderer/renderer.cpp`: from combined VP matrix extract 6 planes (Gribb-Hartmann method: `plane[i] = row3 ± row_i` for left/right/bottom/top/near/far); store as 6 `glm::vec4` (xyz=normal, w=d); call once per `end_frame()` after camera is set
- [ ] T049 [P] [US6] Implement AABB-vs-frustum culling in `end_frame()` opaque loop in `src/renderer/renderer.cpp`: for each draw command, compute world-space AABB from mesh bounds (store axis-aligned bounds at upload time in T020) transformed by world matrix; test AABB center ± extents against all 6 frustum planes; skip `sg_draw` if fully outside; emit debug `printf` with cull count once per 60 frames
- [ ] T050 [P] [US6] Implement front-to-back sort of opaque `draw_queue` in `src/renderer/renderer.cpp` after frustum cull: compute camera-space Z for each draw command's world position (extract from world matrix column 3, dot with camera forward); `std::sort` by ascending Z (nearest first); only sort opaque queue — transparent queue is NOT sorted (back-to-front sort is R-M5 which is Desirable)
- [ ] T051 [US6] Update `src/renderer/app/main.cpp` stress mode: add 50 objects positioned both inside and outside frustum at random positions on a sphere; add ImGui window showing `draws submitted / draws culled / total enqueued`; confirm culling is working by toggling with `C` key

---

## Final Phase: Polish & Cross-Cutting Concerns

**Purpose**: Task board population, Catch2 tests, architecture finalization, interface freeze.

- [ ] T052 [P] Populate `_coordination/renderer/TASKS.md` with all milestone rows translated to blueprint schema: ID, Task, Tier, Status=TODO, Owner=copilot@laptopA, Depends_on, Milestone (R-M0..R-M6), Parallel_Group (PG-M1-A/B etc), Validation (SPEC-VALIDATE+REVIEW for milestones), Notes with `files:` for PG tasks
- [ ] T053 [P] Write Catch2 tests in `tests/renderer/test_mesh_builders.cpp`: sphere mesh at subdivisions=4 has expected vertex count `(4+1)*(4+1)` and index count `4*4*6`; cube mesh has exactly 24 vertices and 36 indices; all vertex normals in sphere are unit-length; all vertex positions in cube lie on `[-half_extent, half_extent]` range
- [ ] T054 [P] Write Catch2 tests in `tests/renderer/test_handles.cpp`: `renderer_handle_valid({0})` returns false; `renderer_handle_valid({1})` returns true; `renderer_enqueue_draw` with invalid handle does not increment `draw_count`; `renderer_enqueue_line_quad` with zero-length segment does not increment `line_quad_count` (test via exposed test-only getter or internal white-box header)
- [ ] T055 Update `docs/interfaces/renderer-interface-spec.md` with `**Status**: FROZEN — v1.0` marker after R-M1 human approval; add freeze date; commit and push to `feature/renderer`; announce to engine workstream (Person B / Laptop B) so engine SpecKit can begin
- [ ] T056 [P] Finalize `docs/architecture/renderer-architecture.md` after R-M3: fill actual pipeline inventory table with measured creation results, confirmed frame sequence diagram, confirmed module-to-file mapping from as-built code
- [ ] T057 [P] Update `docs/planning/speckit/renderer/tasks.md` (the guide file): add note pointing to `_coordination/renderer/TASKS.md` as the live operational task board; mark SpecKit planning outputs as complete

---

## Dependency Graph (User Story Completion Order)

```
Phase 1 (Setup) ──► Phase 2 (Foundation)
                          │
                          ▼
                   Phase 3 (US1 / R-M0)
                          │
                          ▼
                   Phase 4 (US2 / R-M1) ──► [INTERFACE FREEZE] ──► Engine workstream starts
                          │
                          ▼
              ┌──────────────────────────┐
              ▼                          ▼
       Phase 5 (US3 / R-M2)      (parallel possible if file sets disjoint)
              │
              ▼
       Phase 6 (US4 / R-M3)  ◄── RENDERER MVP
              │
              ▼
       Phase 7 (US5 / R-M4)  ◄── Desirable: only if MVP complete within ~3h
              │
              ▼
       Phase 8 (US6 / R-M6)  ◄── Desirable: only if visible perf issue observed

Final Phase runs as tasks complete throughout
```

---

## Parallel Execution Opportunities Per Phase

| Phase | Parallelizable Task Pairs | Disjoint Files |
|-------|--------------------------|----------------|
| Phase 1 | T002 \|\| T003 \|\| T004 \|\| T005 | renderer.h / paths.h.in / CMakeLists(stb) / mocks/ |
| Phase 4 (R-M1) | T018 (shader) \|\| T019+T020+T021+T022 (C++) | unlit.glsl / pipeline_unlit.cpp / mesh_builders.cpp |
| Phase 5 (R-M2) | T028 (shader) \|\| T029+T030 (C++) | lambertian.glsl / pipeline_lambertian.cpp / renderer.cpp |
| Phase 6 (R-M3) | T034+T035 (shaders) \|\| T036+T037 (skybox/debug_draw) \|\| T038 (pipelines) | skybox.glsl+line_quad.glsl / skybox.cpp / debug_draw.cpp / renderer.cpp |
| Phase 7 (R-M4) | T043 (shader) \|\| T044 (pipeline) \|\| T045 (texture) | blinnphong.glsl / pipeline_blinnphong.cpp / texture.cpp |
| Final | T052 \|\| T053 \|\| T054 \|\| T056 \|\| T057 | _coordination/ / tests/renderer/ / docs/ |

---

## Implementation Strategy

**MVP First**: US1 (R-M0) and US2 (R-M1) are the absolute gate. No Lambertian or skybox work begins until US1 is running on all machines. Interface freeze after US2 is a hard dependency for the engine workstream.

**Time budget** (from Hackathon Master Blueprint):
- R-M0: ≤ 45 min
- R-M1: ~ 1 h (includes interface freeze)
- R-M2: ~ 45 min
- R-M3: ~ 45 min (MVP done by ~3 h mark)
- R-M4, R-M6: Remaining time if MVP complete

**`renderer_app` is the acceptance vehicle**: Updated at every milestone to exercise the new capability. Never left showing a previous milestone's demo.

**Magenta fallback is always active**: If any pipeline creation fails, the magenta pipeline renders silently. This allows `renderer_app` to keep running even during shader authoring failures.
