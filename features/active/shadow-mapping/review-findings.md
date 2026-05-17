# Shadow Mapping — Review Findings

## Finding: T-2 — 2026-05-16

- **Source**: code-reviewer
- **Defect class**: weak-acceptance-criteria
- **Suspected owner**: planner/groomer
- **Evidence**: Pipeline creation function existed but was never wired into the init sequence; acceptance criterion ("pipeline logged as created") did not specify the integration call site, so a correct implementation of "create pipeline" did not satisfy "logged as created".
- **Retrospected**: true
- **Process edit**: plan-groomer SKILL.md check 1 — extended to require integration call-site verification for component-creation tasks (cluster with T-3, T-4).

## Finding: T-1 — 2026-05-16

- **Source**: code-reviewer
- **Defect class**: ownership-boundary-violation
- **Suspected owner**: implementor-skill
- **Evidence**: Bumped frozen interface version banner (v1.3 → v1.4) without updating the frozen spec document, violating the freeze rule that any post-freeze contract change requires explicit human approval and synchronized spec update.
- **Retrospected**: true
- **Process edit**: none — single-incident implementor-discipline failure; existing rules in AGENTS.md §5 and renderer-specialist G5 are explicit and sufficient.

## Finding: T-3 — 2026-05-17

- **Source**: code-reviewer
- **Defect class**: weak-acceptance-criteria
- **Suspected owner**: planner/groomer
- **Evidence**: Init function returned early on mid-step failure without cleaning up partially-created GPU resources; shutdown destroyed image handle before dependent view handles (wrong ownership order). Neither the partial-init cleanup pattern nor the reverse-ownership destroy ordering was mentioned in the acceptance criteria, so a correct-looking implementation still leaked GPU objects.
- **Retrospected**: true
- **Immediate edit**: renderer-specialist SKILL.md §3 — added "GPU resource init/teardown pattern" requiring single cleanup helper with reverse-order destroy, called on every mid-init error return.

## Finding: T-4 — 2026-05-17

- **Source**: code-reviewer
- **Defect class**: weak-acceptance-criteria
- **Suspected owner**: planner/groomer
- **Evidence**: Adding a new builtin shader required reserving its ID in the custom-shader counter, creating an sg_shader handle in the init sequence, integrating light-field patching into apply_draw_uniforms(), and binding albedo/shadow resources in the draw loop. The plan specified "write shader + pipeline + factory" but did not enumerate these four integration steps, so the implementor only produced the shader/pipeline/factory without wiring it as a real builtin end-to-end.
- **Retrospected**: true
- **Immediate edit**: renderer-specialist SKILL.md §3 — added "Adding a new builtin shader" checklist covering all four required wire-up locations (ID reservation, shader handle, uniform patching, resource binding).

## Finding: T-5 — 2026-05-17

- **Source**: code-reviewer + spec-validator
- **Defect class**: incorrect-formula-in-plan / contradictory-acceptance-test
- **Suspected owner**: shared-docs (plan text contradicts project convention; acceptance test case direction contradicts shading convention)
- **Evidence**: The plan specified `scene_center - normalize(light.direction) * ortho_far * 0.5f` which places the shadow camera behind the scene for a surface-to-light direction convention, producing inverted depth maps. The acceptance test case used `direction = normalize({0.3, -1.0, 0.2})` (pointing down), contradicting the project's surface-to-light convention where directions point toward the light source. Fix applied: sign changed to `+`.
- **Retrospected**: true
- **Immediate edit**: plan-p1.md T5 task text updated to `scene_center + dir * eye_dist` with explicit note about surface→light convention; unit tests added in `src/renderer/tests/test_shadow_pass.cpp` (9 tests, 13 assertions).

## Finding: T-6 — 2026-05-17

- **Source**: code-reviewer
- **Defect class**: structural-correctness / nested-pass-violation
- **Suspected owner**: planner/groomer
- **Evidence**: The shadow pass was started inside `renderer_end_frame()` while the swapchain pass (opened in `renderer_begin_frame()`) was still active — two concurrent `sg_begin_pass()` calls violate sokol's single-pass rule. The plan said "before pass 0 (skybox)" but did not specify that the main swapchain pass must be deferred until after the shadow pass completes. Fix applied: deferred `sg_begin_pass` for the swapchain from `renderer_begin_frame()` into `renderer_end_frame()`, executing after shadow pass ends.
- **Retrospected**: true
- **Process edit**: sokol-api SKILL.md §1 — added core rule 8 stating that only one pass may be active at a time, with explicit multi-pass frame structure.

## Finding: T-6 — 2026-05-17 (hot-path)

- **Source**: code-reviewer
- **Defect class**: performance-regression / spec-drift
- **Suspected owner**: implementor-skill
- **Evidence**: `light_view_proj` was recomputed inside every BlinnPhongShadowed draw loop iteration instead of once per frame. The plan explicitly says "compute it once, then reuse it" and the cost is a full `glm::lookAt` + `glm::ortho` multiplication per shadow-receiving draw. Fix applied: moved computation to the top of `renderer_end_frame()` into a single `const glm::mat4 light_view_proj` reused by both the shadow pass loop and the main-pass uniform upload.
- **Retrospected**: true
- **Process edit**: none — single-incident implementor-adherence failure; plan text was explicit, code-reviewer Axis 3 caught it, process worked correctly.
