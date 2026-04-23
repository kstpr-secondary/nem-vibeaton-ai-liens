# Renderer SpecKit Research

> **Status**: Fully resolved. See `specs/001-sokol-render-engine/research.md` for the complete Decision records generated during the SpecKit plan phase.

## Summary of resolved items

- **Graphics backend**: OpenGL 3.3 Core only (sokol_gfx `SOKOL_GLCORE33`).
- **Shader pipeline**: `sokol-shdc` build-time compilation; no runtime GLSL loading.
- **Vertex layout**: Universal — Position + Normal + UV + Tangent.
- **Draw queue**: 1024 entries fixed; overflow silently dropped + logged.
- **Asset lifetime**: All GPU resources held until `shutdown()`; no per-handle destroy.
- **Line quads**: Camera-facing billboard (CPU quad generation each frame).
- **Alpha blending**: Basic `SG_BLENDSTATE_ALPHA` in MVP; sorted transparent queue in R-M5.
- **Skybox**: Drawn first, depth write OFF.
- **Dear ImGui**: Renderer-owned via `util/sokol_imgui.h`; game only emits widgets.
- **Public header split**: Single `renderer.h`; all internal files private.
- **Handle naming**: `RendererMeshHandle`, `RendererTextureHandle` (struct `{ uint32_t id; }`).
- **Mock surface**: `src/renderer/mocks/renderer_mock.cpp`, swapped via `USE_RENDERER_MOCKS=ON`.
- **stb_image**: Renderer owns `STB_IMAGE_IMPLEMENTATION`; exposed via PUBLIC include path.

## Risks resolved

- `sokol-shdc` availability: treated as a configure-time error when missing (`VIBEATON_REQUIRE_SOKOL_SHDC=ON` default).
- OpenGL 3.3 startup: validated at `renderer_init()` via `sg_query_backend()` check; fatal log on mismatch.
- Public API stable enough for engine planning: `docs/interfaces/renderer-interface-spec.md` is populated and ready for freeze review.
- Mock swap path: `USE_RENDERER_MOCKS` CMake flag already wired in `CMakeLists.txt`.
