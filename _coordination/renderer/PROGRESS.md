# PROGRESS.md — Renderer Workstream

> Updated by renderer specialist as milestones complete. See `_coordination/overview/PROGRESS.md` for cross-workstream status.

## Milestones

| Milestone | Status | Owner | Notes |
|---|---|---|---|
| R-M0 Bootstrap | TODO | — | Sokol init, window clear, input callback |
| R-M1 Unlit Forward + Procedural Scene | TODO | — | First sync point with engine |
| R-M2 Directional Light + Lambertian | TODO | — | Uniform block struct (BOTTLENECK) |
| R-M3 Skybox + Line Quads + Alpha Blending | TODO | — | MVP complete |
| R-M4 Blinn-Phong + Diffuse Textures | DONE | — | Fixed cam_pos extraction: inv(view)[3][0..2] not [0..2][3] |
| R-M5 Custom Shader + Sorted Transparency + Capsule | TODO | — | Desirable |
| R-M6 Frustum Culling + Front-to-Back Sort | DONE | — | Gribb-Hartmann VP planes, AABB cull (sphere approx), front-to-back sort, stress demo. Post-review: AABB-cull extracted to `is_culled()` lambda (DRY fix), radius auto-computed from vertices when not provided (`renderer_upload_mesh` default 0.0f → centroid scan), `renderer_get_culled_count()` accessor + HUD display. R-053 CRITICAL FIX: plane extraction corrected from `c[i].w + c[i].x` to `col3 ± coli`; cull logic "all corners behind ALL 6 planes" → "all corners outside AT LEAST ONE plane". R-054 CRITICAL FIX: cull count exceeded total objects (both loops calling `is_culled()` on every opaque index → double-counting). Replaced with single pre-pass, cached in `bool culled[1024]` indexed by sort position. R-055 CRITICAL FIX: Gribb-Hartmann normals are outward-facing; `d >= 0` means OUTSIDE, not inside — original test inverted this (culling objects fully INSIDE the frustum). Fixed comparison to `d < 0.0f`. Added `renderer_set_culling_enabled(bool)` API; `C` key now actually toggles culling in both renderer app and game app.
