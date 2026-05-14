# Feature Specification: Sokol Render Engine

**Feature Branch**: `001-sokol-render-engine`
**Created**: 2026-04-23
**Status**: Draft

---

## User Scenarios & Testing *(mandatory)*

### User Story 1 — Window Initialization and Input Pump (Priority: P1)

A renderer consumer (the standalone `renderer_app` driver or the game engine) calls `init(config)` to open a window with the configured resolution and clear color, then calls `run()` to start the event loop. Input events are forwarded to a registered callback. The consumer calls `shutdown()` to release all resources cleanly.

**Why this priority**: This is the foundational gate (R-M0). No other rendering capability can work without a stable window and graphics context. All target machines must pass this before any other work begins.

**Independent Test**: Launch `renderer_app`; the window opens, the framebuffer clears to the configured color, keyboard and mouse events appear in the console log, and the process exits cleanly on close.

**Acceptance Scenarios**:

1. **Given** a renderer config with resolution and clear color, **When** `init()` then `run()` are called, **Then** a window of the configured size opens and the framebuffer clears to the specified color on every frame.
2. **Given** the renderer is running, **When** a keyboard or mouse event occurs, **Then** it is forwarded to the registered input callback with the raw event data.
3. **Given** the renderer is initialized, **When** `shutdown()` is called, **Then** all GPU and window resources are released with no errors or leaks reported.

---

### User Story 2 — Unlit 3D Scene Rendering (Priority: P1)

A renderer consumer uploads procedural geometric shapes (spheres, cubes), sets a perspective camera, and enqueues draw calls each frame using flat-color materials. All enqueued objects appear on screen at their specified positions, orientations, and scales.

**Why this priority**: This is the first milestone sync point (R-M1). It unlocks parallel game engine development and validates the core draw-submission API that all downstream workstreams depend on.

**Independent Test**: `renderer_app` displays at least ten colored primitives (spheres and cubes) at varied transforms under a fixed perspective camera—no lighting, just flat colors—with no visual artifacts.

**Acceptance Scenarios**:

1. **Given** sphere and cube meshes are available from the renderer's procedural builders, **When** draw calls are enqueued with transforms and flat-color unlit materials, **Then** objects appear at the correct positions, orientations, and scales.
2. **Given** a camera specified by view and projection matrices, **When** `set_camera()` is called before `end_frame()`, **Then** all 3D objects transform correctly into screen space.
3. **Given** multiple objects enqueued in one frame, **When** `end_frame()` executes, **Then** all objects render without flickering, popping, or missing draw submissions.
4. **Given** an externally generated mesh (vertices + indices), **When** it is uploaded via `upload_mesh()`, **Then** draw calls against that mesh handle render the correct geometry.

---

### User Story 3 — Directional Light with Lambertian Shading (Priority: P2)

A renderer consumer sets a directional light (direction, color, intensity) and assigns Lambertian materials to objects. Surfaces facing the light are brighter; surfaces facing away are dark. Toggling between unlit and Lambertian pipelines is supported for debugging.

**Why this priority**: Lambertian shading (R-M2) is the first visual quality step that makes 3D objects read as solid. It depends on the unlit pipeline being stable (P1) and provides a sync point for game-world aesthetics.

**Independent Test**: `renderer_app` renders a procedural scene under one directional light; orbiting the camera or animating the light direction shows a clear and correct shading response across all surfaces.

**Acceptance Scenarios**:

1. **Given** a directional light set via `set_directional_light()`, **When** objects with Lambertian materials are enqueued, **Then** the diffuse shading responds correctly to the light direction and intensity.
2. **Given** two identical meshes — one with an unlit material and one with Lambertian — **When** rendered together, **Then** only the Lambertian object responds to the directional light.
3. **Given** a shader or pipeline creation failure at startup, **When** any draw call is submitted, **Then** a visually distinct fallback (magenta) renders and no crash occurs.

---

### User Story 4 — Skybox, Laser Line-Quads, and Alpha Blending (Priority: P2)

A renderer consumer sets a cubemap skybox, renders world-space quad lines between two 3D points (for laser effects), and submits transparent geometry that alpha-composites correctly over the opaque scene. The skybox appears behind all geometry; transparent quads occlude correctly by depth.

**Why this priority**: These three capabilities (R-M3) complete the renderer MVP. They deliver the visual identity core of the space shooter: a starfield background, visible laser beams, and the ability to composite VFX.

**Independent Test**: `renderer_app` shows a starfield skybox behind colored opaque objects, with laser quad lines that composite in front of the background but behind nearer solid objects—verified visually by a human reviewer.

**Acceptance Scenarios**:

1. **Given** a cubemap texture loaded from disk, **When** `set_skybox()` is called, **Then** the skybox renders as the background with no depth artifacts against opaque geometry.
2. **Given** two 3D world-space points and a width/color, **When** `enqueue_line_quad()` is called, **Then** a camera-facing billboard quad renders between the points at full visible width regardless of the camera viewing angle.
3. **Given** alpha-blended transparent quads and opaque geometry in the same frame, **When** `end_frame()` executes, **Then** the transparent quads composite correctly over the background and remain occluded by nearer opaque objects.
4. **Given** the scene includes a Dear ImGui HUD overlay, **When** `end_frame()` executes, **Then** the UI renders after the 3D scene without interfering with 3D depth or blending state.

---

### User Story 5 — Blinn-Phong Shading and Diffuse Textures (Priority: P3)

A renderer consumer uploads 2D textures and creates materials that reference them, yielding textured surfaces. Materials also accept a shininess parameter for specular highlights under the directional light.

**Why this priority**: Desirable feature (R-M4) that improves visual fidelity beyond MVP. Depends on Lambertian shading being complete. Attempted only after MVP milestones are merged.

**Independent Test**: `renderer_app` demo shows a textured asteroid and a specular-highlighted ship surface side by side under a directional light, with distinct visible highlights and correct texture mapping.

**Acceptance Scenarios**:

1. **Given** a 2D RGBA texture uploaded via `upload_texture_2d()`, **When** a material references that texture handle, **Then** the object renders with the texture correctly mapped to its UV coordinates.
2. **Given** a Blinn-Phong material with a shininess value, **When** rendered under the directional light, **Then** a specular highlight of appropriate size and intensity appears on the surface.

---

### User Story 6 — Frustum Culling and Sorted Opaque Queue (Priority: P3)

The renderer automatically discards draw submissions for objects outside the camera frustum and sorts the opaque queue front-to-back to reduce GPU overdraw.

**Why this priority**: Desirable performance optimization (R-M6). Undertaken only if the human reviewer notices visible lag or judder during the behavioral check — no numeric FPS target is required. No visible change to correctly rendered scenes.

**Independent Test**: A stress scene with 50+ objects positioned both inside and outside the frustum; the GPU draw call count drops significantly compared to the uncullled baseline; FPS improves measurably.

**Acceptance Scenarios**:

1. **Given** objects both inside and outside the camera frustum, **When** `end_frame()` executes, **Then** objects outside the frustum are not submitted as GPU draw calls.
2. **Given** many overlapping opaque objects, **When** the front-to-back opaque sort is active, **Then** overdraw measured on GPU is reduced compared to unsorted submission order.

---

### Edge Cases

- What happens when more than 1024 draw calls are enqueued in a single frame? → Submissions beyond the 1024-slot fixed queue are silently dropped; a debug log entry is emitted; no crash or corruption occurs.
- What happens when `enqueue_draw()` is called before `begin_frame()`? → Silently ignored (or debug-asserted); no state corruption.
- What happens when mesh upload fails (e.g., GPU memory exhausted)? → Returns an invalid handle; draw calls with an invalid handle are skipped without crashing.
- What happens when a shader program fails at pipeline creation? → Falls back to the magenta error pipeline; the failure is logged; no crash.
- What happens when `set_camera()` is not called before `end_frame()`? → A default identity camera is used; a warning is logged in debug builds.
- What happens when zero draw calls are enqueued in a frame? → Frame still completes; only the clear color and skybox (if set) are visible.
- What happens when `enqueue_line_quad()` receives two identical points (zero-length line)? → The quad degenerates; it is skipped or rendered as a point; no crash.

---

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The renderer MUST initialize a window with configurable resolution and clear color via a single `init(config)` call.
- **FR-002**: The renderer MUST expose a blocking event-loop entry point (`run()`) that owns the window and drives all frame callbacks.
- **FR-003**: The renderer MUST forward raw input events to a consumer-registered callback each frame.
- **FR-004**: The renderer MUST accept per-frame draw submissions of (mesh handle, world transform, material) tuples via `enqueue_draw()`, up to a maximum of **1024 draw calls per frame**; submissions beyond this limit are silently dropped and logged.
- **FR-005**: The renderer MUST provide procedural mesh builders for sphere geometry (`make_sphere_mesh`) and cube geometry (`make_cube_mesh`).
- **FR-006**: The renderer MUST provide an upload path for externally generated meshes using the canonical vertex layout (Position + Normal + UV + Tangent: `float3 position`, `float3 normal`, `float2 uv`, `float3 tangent`) plus an index buffer, returning a stable mesh handle.
- **FR-007**: The renderer MUST accept a camera specified by a view matrix and a projection matrix via `set_camera()`, applied once per frame.
- **FR-008**: The renderer MUST support an Unlit shading model (flat solid color; no lighting computation).
- **FR-009**: The renderer MUST support a Lambertian diffuse shading model driven by the active directional light.
- **FR-010**: The renderer MUST accept exactly one directional light per frame via `set_directional_light(direction, color, intensity)`.
- **FR-011**: The renderer MUST support world-space laser line-quad rendering between two 3D endpoints with configurable width and color via `enqueue_line_quad()`; quads are **camera-facing billboards** — they rotate each frame so their face always points toward the camera eye, ensuring full visible width from any viewing angle.
- **FR-012**: The renderer MUST support basic alpha blending for transparent geometry submitted via `enqueue_draw()`.
- **FR-013**: The renderer MUST render a cubemap skybox behind all opaque geometry when `set_skybox()` is called; depth writes are disabled for the skybox pass.
- **FR-014**: The renderer MUST own Dear ImGui initialization, per-frame UI setup, input forwarding, and final UI render; consuming workstreams only emit widget calls.
- **FR-015**: The renderer MUST expose two texture upload paths: (a) `upload_texture_2d(pixels, w, h, format)` — accepts a raw decoded pixel buffer (used by the engine for cgltf-embedded textures); and (b) `upload_texture_from_file(path)` — a convenience wrapper using `stb_image` that decodes a PNG/JPG/BMP file from disk and calls (a) internally. Both return a stable texture handle. `stb_image` is a FetchContent dependency available to all targets via the renderer's PUBLIC include path.
- **FR-016**: The renderer MUST support a Blinn-Phong shading model with specular term and shininess material parameter (Desirable—R-M4).
- **FR-017**: The renderer MUST fall back to a visually distinct magenta error pipeline on any shader or pipeline creation failure; it MUST NOT crash.
- **FR-018**: The renderer MUST release all GPU and window resources cleanly via `shutdown()`.
- **FR-019**: The renderer SHOULD cull draw submissions outside the camera frustum before GPU submission (Desirable—R-M6).
- **FR-020**: The renderer SHOULD sort the opaque draw queue front-to-back by camera distance to reduce overdraw (Desirable—R-M6).

### Key Entities

- **Mesh**: A GPU-resident buffer pair (vertices + indices) using the canonical vertex layout: **Position + Normal + UV + Tangent** (`xyz float3`, `normal float3`, `uv float2`, `tangent float3`). Created by procedural builders or uploaded from external geometry data. Referenced by an opaque handle.
- **Material**: A shading configuration specifying shading model (Unlit / Lambertian / BlinnPhong), base color, optional texture handle, and optional custom shader handle.
- **Draw Command**: An immutable per-frame submission tuple of (mesh handle, world-transform matrix, material). Consumed during `end_frame()` and discarded; not retained across frames.
- **Directional Light**: A global illumination descriptor with world-space direction vector, linear RGB color, and scalar intensity. One active light maximum per frame.
- **Camera**: A view-projection pair (two 4×4 float matrices) set once per frame. Controls perspective projection and view transform for all 3D draw commands.
- **Skybox**: A cubemap texture rendered as a full-scene background with depth writes disabled, before all opaque draws.
- **Line Quad**: A camera-facing billboard quad defined by two 3D world-space endpoints and a width scalar. The quad rotates each frame to face the camera eye, ensuring full visible width from any angle. Rendered with the Unlit shader and optional alpha.
- **Texture**: A 2D RGBA image resident on the GPU, referenced by an opaque handle. Created via `upload_texture_2d()` (raw pixels, used by the engine for cgltf data) or `upload_texture_from_file()` (PNG/JPG/BMP from disk, used by `renderer_app` demos and engine for loose assets).

---

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: `renderer_app` initializes, presents a clear-color window, and exits cleanly on all three target Ubuntu machines within the first 45 minutes of the hackathon (R-M0 gate).
- **SC-002**: A procedural scene of at least ten distinct colored 3D objects renders at correct positions under a perspective camera with no visual artifacts (R-M1 acceptance).
- **SC-003**: A single directional light produces visually correct Lambertian shading on procedural geometry—verified by a human reviewer from multiple camera angles (R-M2 acceptance).
- **SC-004**: Starfield skybox, alpha-blended laser quads, and opaque geometry composite correctly with no depth or blending artifacts—visually verified by a human reviewer (R-M3 acceptance = renderer MVP complete).
- **SC-005**: The renderer MVP builds and `renderer_app` runs within the 3-hour mark from hackathon start.
- **SC-006**: Any shader or pipeline creation failure produces a magenta fallback render; no renderer crash occurs under any submitted draw configuration during the hackathon.
- **SC-007**: The game engine workstream can link against the frozen renderer interface and compile against mock stubs without access to renderer internals.
- **SC-008** *(Desirable)*: Blinn-Phong shading with a diffuse texture renders correctly on at least one textured mesh under the directional light (R-M4).
- **SC-009** *(Desirable)*: Frustum culling is implemented and activated only if visible performance degradation is observed during a human behavioral check; no numeric FPS threshold is required.

---

## Clarifications

### Session 2026-04-23

- Q: What is the canonical vertex attribute set for the renderer's mesh format? → A: Position + Normal + UV + Tangent (`float3 xyz`, `float3 normal`, `float2 uv`, `float3 tangent`) — one universal layout covering all tiers from Unlit through Stretch (normal maps).
- Q: What is the minimum acceptable frame rate before R-M6 becomes required? → A: Uncapped / best effort — no numeric FPS floor; R-M6 is triggered only if visible lag or judder is noticed during the human behavioral check ("if it feels good, it is good").
- Q: Can mesh and texture handles be freed individually at runtime, or held until shutdown? → A: Hold until shutdown (Option A) — the full asset set is a small fixed library (~10 unique meshes: 5–7 asteroid variants, 2–4 ship types, plus procedural primitives); all uploads live for the process lifetime and `shutdown()` is the sole release point. No per-handle destroy API is needed.
- Q: What is the maximum number of draw calls the renderer must support per frame? → A: 1024 draw calls — fixed pre-allocated queue; generous headroom for the full game scene plus VFX; overflow submissions beyond 1024 are silently dropped and logged.
- Q: How should laser line-quads orient themselves relative to the camera? → A: Camera-facing billboard — the quad always rotates so its face points toward the camera eye, ensuring full visible width from any viewing angle.

---

## Assumptions

- The graphics context is **OpenGL 3.3 Core Profile** on desktop Linux (Ubuntu 24.04 LTS). Vulkan and OpenGL ES are explicitly out of scope.
- Shaders are precompiled at build time from annotated GLSL sources; no runtime GLSL loading or hot-reload is supported.
- The renderer owns the application main loop and window; all other workstreams hook into it via callbacks — they do not create their own windows.
- Exactly one directional light is supported in the MVP; multi-light support is not planned for any tier.
- Shadow mapping, PBR, deferred rendering, post-processing, MSAA, and GPU instancing are explicitly cut from all tiers (MVP, Desirable, Stretch) unless a spare agent finds GPU instancing trivial to slot into an earlier milestone.
- Mesh data is static after upload; dynamic vertex updates, skinned meshes, and morphing are out of scope. The full runtime asset library is a fixed small set (~10 unique meshes, ~5 textures); no per-handle destroy API (`destroy_mesh` / `destroy_texture`) is provided — all resources are released at `shutdown()`.
- The game shares mesh handles across all instances of the same shape (e.g. all plasma bolts share one plasma mesh handle); per-entity mesh creation is not supported.
- `stb_image` (single public-domain C header, FetchContent pinned pre-hackathon) is the image file decoder. It is added as a FetchContent dependency and exposed via the renderer's PUBLIC include path so all workstreams can use it. Its include path is populated in `CMakeLists.txt` before the hackathon starts.
- The renderer owns `STB_IMAGE_IMPLEMENTATION` in exactly one of its `.cpp` files; no other translation unit defines it.
- The renderer's public API (function signatures, types, handle types) is frozen before the game engine workstream begins. Frozen API changes require explicit human approval.
- All three workstreams share one repository; changes that affect the renderer's public header require a cross-workstream notification before commit.
- The hackathon window is 5–6 hours; MVP milestones R-M0 through R-M3 must land within the first 3 hours to leave adequate time for game engine and game integration.
- `renderer_app` (the standalone driver executable) is the canonical acceptance vehicle for every milestone and must be updated at each milestone to exercise the newly added capability.
- Dear ImGui integration is fully owned by the renderer; the game workstream is not allowed to initialize, shut down, or configure ImGui independently.
- The alpha-blended queue in the MVP is basic (no back-to-front sorting); sorted transparency is a Desirable feature (R-M5).
- Capsule mesh builder is a Desirable feature (R-M5); it is not required for MVP.
