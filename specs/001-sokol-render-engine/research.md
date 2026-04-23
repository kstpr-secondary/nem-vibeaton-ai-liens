# Renderer SpecKit — Research

> **Phase 0 output** for `001-sokol-render-engine`. All NEEDS CLARIFICATION items resolved.

---

## Decision 1: Graphics Backend

**Decision**: OpenGL 3.3 Core Profile only — the sole `sokol_gfx` backend compiled in.  
**Rationale**: All four Ubuntu 24.04 machines have drivers that expose GL 3.3 Core. Vulkan requires SPIR-V + reflection descriptors on `sg_shader_desc`; enabling Vulkan would require glslang/shaderc + SPIRV-Cross integration (~2-4h of plumbing with no visible output). GL 3.3 Core is fully supported by `sokol_gfx` and sokol-shdc without additional tooling.  
**Alternatives considered**: Vulkan (removed, too much plumbing), OpenGL ES (desktop-incompatible), Metal/D3D11 (wrong OS).  
**References**: `pre_planning_docs/Hackathon Master Blueprint.md §3.3`, constitution Technology Constraints.

---

## Decision 2: Shader Compilation Pipeline

**Decision**: Annotated `.glsl` files under `shaders/renderer/`, precompiled at **build time** by `sokol-shdc` into `${CMAKE_BINARY_DIR}/generated/shaders/<name>.glsl.h`. Consumed in C++ via `#include` + `shd_<name>_shader_desc(sg_query_backend())`.  
**Rationale**: `sokol-shdc` is the intended sokol workflow. Build-time errors surface shader mistakes before runtime. Generates backend-portable bytecode + reflection descriptors automatically (uniform block layout, vertex attributes, sampler bindings). Zero runtime file I/O for shaders.  
**Alternatives considered**: Runtime GLSL loading via glslang (2-4h extra plumbing, cut), shader hot-reload (cut explicitly), raw `sg_make_shader` with manually-crafted descriptors (fragile, error-prone).  
**Error handling**: Any `sg_make_shader` / `sg_make_pipeline` failure logs the error and activates the **magenta error pipeline** (solid magenta fragment output, no crash). This is acceptance criterion SC-006.

---

## Decision 3: Vertex Layout

**Decision**: Single universal vertex layout for all mesh types — **Position + Normal + UV + Tangent**:
```c
struct Vertex {
    float position[3];   // xyz
    float normal[3];     // xyz
    float uv[2];         // st
    float tangent[3];    // xyz
};
```
**Rationale**: One layout covers all tiers (Unlit through Blinn-Phong/stretch normal maps) without any vertex-buffer reformatting on pipeline switch. Tangent upload at Unlit tier costs only 12 bytes/vertex padding — acceptable overhead for zero pipeline fragmentation.  
**Alternatives considered**: Separate compact layout for Unlit (no normal/tangent) — rejected because it would require a second VBO format and a pipeline-conditional vertex descriptor.

---

## Decision 4: Draw Queue Capacity

**Decision**: Fixed pre-allocated array of **1024 draw commands** per frame. Overflow submissions are silently dropped and emit a single debug log line. No dynamic reallocation.  
**Rationale**: The full game scene (5–7 asteroid variants × N instances, 2–4 ship types, VFX quads) will never approach 1024 simultaneous draw calls at hackathon scale. A fixed array eliminates heap allocation on the hot path and bounds memory usage.  
**Alternatives considered**: `std::vector` with dynamic growth (heap pressure on hot path, rejected), 512-slot queue (too tight for stress tests, rejected), unlimited (defeats the predictable-memory goal).

---

## Decision 5: Asset Lifetime and Handle API

**Decision**: All GPU resources (meshes, textures, pipelines) are **held until `shutdown()`**. No per-handle `destroy_mesh()` / `destroy_texture()` API. Handles are opaque `uint32_t`-backed structs.  
**Rationale**: The full runtime asset library is ~10 unique meshes and ~5 textures — a fixed small set loaded once at startup. The simplest correct design is a flat handle table freed at shutdown. No reference counting, no deferred deletion, no resource pools.  
**Alternatives considered**: Ref-counted resource pool (overengineered for hackathon scale), slab allocator (same), per-handle destroy API (adds complexity, no benefit for fixed asset set).

---

## Decision 6: Line-Quad Billboard Orientation

**Decision**: `enqueue_line_quad(p0, p1, width, color)` generates a **camera-facing billboard quad** — the quad face always points toward the camera eye. The camera-to-quad direction is computed each frame in C++ before GPU upload; the quad corner positions are recomputed on CPU.  
**Rationale**: Ensures full visible width of laser beams from any viewing angle without a geometry shader. A geometry shader would require a separate GPU path and add sokol pipeline complexity.  
**Alternatives considered**: Fixed-axis billboard (breaks at glancing angles), geometry shader (requires separate pipeline), vertex shader billboard (needs per-vertex camera position uniform already available in the frame UBO).  
**Implementation note**: The perpendicular axis is `normalize(cross(dir, cam_to_midpoint))` × `width/2`. Four corners are generated on CPU and submitted as two triangles (or a quad via index buffer).

---

## Decision 7: Alpha Blending Strategy

**Decision**: **MVP (R-M3)**: basic `SG_BLENDSTATE_ALPHA` on a separate transparent pass with depth write off, depth test on. **No back-to-front sort in MVP.** Desirable (R-M5): sorted transparency queue.  
**Rationale**: Sorting requires per-frame distance computation and a sort call — overhead not justified for MVP laser quads where overdraw is limited. Back-to-front correctness is a Desirable quality improvement.  
**Alternatives considered**: Depth peeling (too expensive), OIT (too complex), always-sorted transparent queue from day one (over-engineering for MVP).

---

## Decision 8: Skybox Rendering Strategy

**Decision**: Draw skybox **first** in the frame with depth write OFF and depth range clamped to far clip (or use the "infinite skybox" trick by setting w=1 in the vertex shader so z/w = 1 always). Opaque geometry draws after on top of the skybox pixels.  
**Rationale**: Drawing skybox last requires a depth comparison at the far plane which can produce z-fighting. Drawing first avoids that while only paying for overdraw on pixels where geometry covers the skybox — acceptable at hackathon scale.  
**Alternative**: Draw skybox last with reversed-z. Rejected — adds projection matrix complexity incompatible with the fixed view/projection API.

---

## Decision 9: Dear ImGui Integration

**Decision**: Renderer owns `util/sokol_imgui.h` from the pinned sokol repo. Renderer initializes ImGui, forwards `sokol_app` events, calls `simgui_new_frame()` before 3D rendering, calls `simgui_render()` after the 3D scene in the same render pass. Game workstream only calls `ImGui::*` widget functions.  
**Rationale**: Single ownership of the ImGui backend prevents double-init and event-forwarding conflicts. Sokol's `simgui` integration is purpose-built for this workflow.  
**Alternatives considered**: Game-owned ImGui (conflicts with renderer event pump), GLFW backend (prohibited by constitution).

---

## Decision 10: sokol-shdc CMake Integration

**Decision**: A CMake custom command per shader file:
```cmake
add_custom_command(
  OUTPUT ${GENERATED}/shaders/foo.glsl.h
  COMMAND sokol-shdc --input shaders/renderer/foo.glsl
                     --output ${GENERATED}/shaders/foo.glsl.h
                     --slang glsl330
  DEPENDS shaders/renderer/foo.glsl
)
```
`VIBEATON_REQUIRE_SOKOL_SHDC=ON` (default) treats missing `sokol-shdc` on `PATH` as a CMake configure error.  
**Rationale**: Ensures build-time validation of every shader. Ninja's dependency tracking rebuilds generated headers only when `.glsl` sources change.  
**Alternatives considered**: Makefile-based pre-build step (breaks CMake incremental), embedding GLSL as string literals (loses sokol-shdc reflection).

---

## Open-Question Resolutions

| Question | Resolution |
|----------|-----------|
| Final public header split | Single `renderer.h` public header with all types + free-function declarations; implementation split across `renderer.cpp`, `pipeline_*.cpp`, etc. |
| Handle naming conventions | `RendererMeshHandle`, `RendererTextureHandle` (struct wrapping `uint32_t`); invalid handle is `{0}` |
| Mock surface shape | `src/renderer/mocks/renderer_mock.cpp` provides no-op implementations of every function in `renderer.h`; swapped in via `USE_RENDERER_MOCKS=ON` CMake flag |
| stb_image ownership | Renderer owns `STB_IMAGE_IMPLEMENTATION` in exactly one `.cpp`; exposed via `PUBLIC` include path in CMakeLists |
| Catch2 scope | Tests cover: mesh builder vertex/index counts, handle validity after upload, matrix passthrough in camera set, magenta fallback trigger |
