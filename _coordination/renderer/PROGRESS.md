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
| R-M6 Frustum Culling + Front-to-Back Sort | DONE | — | Gribb-Hartmann VP planes, AABB cull (sphere approx), front-to-back sort, stress demo |
