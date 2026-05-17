# Checkpoint: Shadow Mapping — Phase 1

**Date**: 2026-05-17
**Verified by**: human

## Result: PASS

## What was tested

Ran `cmake --build build --target renderer_app && ./build/renderer_app`. Build exits 0 with no new warnings. Application starts; console shows shadow pass init completed — shadow map image handle non-zero, shadow depth pipeline created without sokol validation error. No sokol validation errors during frame execution (shadow pass running before main pass each frame). Existing renderer demo renders identically to before — no visual regression, no new GL errors.

## Observations

All 4 Human Checkpoint criteria met:
1. Build clean — zero warnings, zero errors.
2. Shadow pass init succeeds — shadow map image handle non-zero; depth pipeline created cleanly via `create_pipeline_shadow()` from `renderer_internal_init()`.
3. Shadow pass executes before main pass each frame with no sokol validation errors.
4. Existing renderer demo unchanged — no visual regression, no GL errors.

No surprises or deviations. Core hard-shadow pipeline is fully wired and running. The deferred tasks (T-7 demo scene, T-8 debug overlay) remain for Phase 2 as planned.

## Roadmap impact

N/A — Phased feature, not Exploratory.

## Next step

generate plan-p2.md
