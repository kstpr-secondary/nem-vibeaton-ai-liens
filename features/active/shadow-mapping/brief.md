# Feature Brief: Shadow Mapping

**Type**: Phased
**Branch**: feature/shadows
**Workstreams**: renderer (primary), game (material assignment at scene init)
**Frozen interfaces affected**: `docs/interfaces/renderer-interface-spec.md` (v1.3 — additive enum value + convenience factory; human-approved 2026-05-16)
**Architecture consult**: Completed 2026-05-16 (systems-architect)

---

## Goal

Add dynamic hard shadow casting from the single directional light to the forward renderer. Phase 1 delivers a single-cascade depth-map shadow pass that auto-casts all opaque geometry and lets any draw opt into shadow-receiving by using the new `BlinnPhongShadowed` builtin shader. A new renderer demo scene exercises and validates the full shadow pipeline before the feature is used in-game.

Subsequent phases will expand to full Cascaded Shadow Maps (Phase 2) and PCF soft-shadow filtering with a runtime configuration API (Phase 3).

---

## Success Criteria

- [ ] **Visual — shadow cast**: Opaque geometry rendered with `BlinnPhongShadowed` receives visible, correctly-positioned shadows from objects above/around it in the renderer demo scene.
- [ ] **Visual — no acne**: The illuminated faces of shadow-casting objects show no shadow acne (self-shadowing noise). Front-face culling on the shadow pass eliminates static-bias tuning for Phase 1.
- [ ] **Visual — shadow receiver opt-out**: Objects using the standard `BlinnPhong` shader render identically to before — no shadow darkening on them, confirming backward compatibility.
- [ ] **Visual — transparent geometry unaffected**: Alpha-blended objects in the demo scene do not participate in shadow casting or receiving and render correctly.
- [ ] **Visual — camera stability**: Shadows remain spatially stable as the orbiting demo camera moves. No per-frame crawl or severe flicker.
- [ ] **Demo scene**: The renderer shadow demo is accessible via ImGui tab in `renderer_app` and shows: plane, multiple spheres/cubes (opaque, shadow-receiving), one comparison non-shadowed object, one transparent object, orbiting camera, directional light from above slightly tilted.
- [ ] **Build**: `cmake --build build --target renderer_app` exits 0 with no new warnings.

---

## Scope

**In (Phase 1)**:
- Single-cascade depth-map shadow pass (no CSM frustum partitioning yet).
- Auto-cast: all `render_queue=0` draws are re-rendered in the shadow pass without API change.
- `BlinnPhongShadowed` builtin shader variant: opaque geometry that receives shadows uses this shader; `light_view_proj` and shadow map binding are renderer-internal.
- `renderer_make_blinnphong_shadowed_material()` convenience factory (mirrors `renderer_make_blinnphong_material()`).
- Fixed orthographic light projection sized to cover the ~1 km game arena (hardcoded constants, architecture supports future `renderer_set_shadow_config()` without restructuring).

**Out (Phase 1, deferred)**:
- Shadow map debug overlay (small corner quad, toggled via ImGui) - Phase 2.
- Renderer demo scene for shadow validation - Phase 2.
- PCF (soft shadow edges) — Phase 3.
- CSM (multiple cascades, frustum partitioning, cascade blend bands, texel snapping) — Phase 2.
- `renderer_set_shadow_config()` runtime API — Phase 3.
- `ShadowCaster` ECS component for per-entity opt-out of shadow casting — future phase. Until then all opaque geometry casts.
- Transparent/cutout shadow casting — out of scope for all phases at this time.
- Vulkan backend, compute culling, mesh shaders, bindless — future renderer evolution phases.
- In-game shadow integration (wiring asteroid/ship materials to `BlinnPhongShadowed`) — a follow-on game task after Phase 1 validates the pipeline.

---

## Key Unknowns

| Unknown | Risk if wrong | How to verify | Fallback |
|---------|---------------|---------------|---------|
| `sg_make_view()` with `depth_stencil_attachment` available in pinned sokol version | Shadow depth pass attachment cannot be created; Phase 1 blocked | Grep `sg_make_view` and `depth_stencil_attachment` in `src/renderer/` vendor headers at start of T-3 | If API differs, adapt to equivalent current API (`sg_make_pass_action` + `sg_make_attachments` if present); escalate to human |
| `SG_IMAGETYPE_ARRAY` available in pinned sokol version | Phase 2 (CSM texture array) must use a different resource strategy | Grep `SG_IMAGETYPE_ARRAY` in sokol_gfx.h at start of Phase 2 | Use N separate `sg_image` + N separate passes instead of texture array; binding model adjusts |

*Second unknown is not a Phase 1 blocker — noted here so Phase 2 planning accounts for it.*

---

## Known Risks

- **Ortho projection too small for Phase 1**: If the hardcoded ortho box (covering 1 km arena) is too large relative to the demo scene, shadow map resolution will be visibly poor even for hard shadows. Mitigate by using a separate tighter ortho for the demo scene and the large one for the in-game integration.
- **Front-face culling trade-off**: Phase 1 uses `SG_CULLMODE_FRONT` in the shadow pass (standard acne prevention without static bias). This can produce "detached shadow" Peter Panning on very thin geometry. Demo scene meshes (spheres, cubes, thick box plane) are not thin, so this risk is low in Phase 1.
- **CMakeLists.txt co-ownership**: Two new shaders (shadow_depth.glsl, blinnphong_shadowed.glsl, shadow_debug.glsl) require new sokol-shdc custom commands. CMakeLists.txt is co-owned by renderer and systems-architect — changes are additive only, no target restructuring.

---

## Phase Roadmap (overview)

| Phase | Name | Key Deliverable |
|-------|------|-----------------|
| **1** | Hard Shadows | Single-cascade depth map, BlinnPhongShadowed shader, renderer demo scene |
| **2** | Cascaded Shadow Maps | N-cascade frustum partitioning, texture array, cascade selection shader, texel snapping |
| **3** | PCF + Config API | Hardware PCF via `sampler2DShadow`, `renderer_set_shadow_config()` for cascade count/resolution/bias/kernel |
