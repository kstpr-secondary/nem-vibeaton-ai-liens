# Engine Specialist SKILL — Compaction Opportunities

> Analysis of `.agents/skills/engine-specialist/SKILL.md` (352 lines) vs requirements from `Hackathon Master Blueprint.md` and `Game Engine Concept and Milestones.md`. Goal: reduce to ~270 lines (~23% reduction) without losing decision-critical knowledge.

---

## 1. Cut Entirely — Redundant with Source Documents

### 1.1 Component Schema (§4, ~10 lines)

**Current:** Verbatim copy of the component schemas from `Game Engine Concept and Milestones.md`.

```
- `Transform { position: vec3, rotation: quat, scale: vec3 }`.
- `Mesh { mesh_handle }`, `Material { material_handle }` ...
- `RigidBody { mass, inv_mass, linear_velocity, angular_velocity, inv_inertia, restitution }`.
- `Collider { AABB { half_extents } }` ...
- `Light { Directional { direction, color, intensity } }` ...
- `Camera { fov, near, far }`.
- Tag markers: `Static`, `Dynamic`, `Interactable`.
```

**Why cut:** §3 already points to the Engine seed as "Source of milestone structure, MVP cuts, Public API Surface, component schemas." Keeping a verbatim copy creates maintenance burden — if the schema changes in the seed during SpecKit, this SKILL drifts and violates AGENTS.md Rule 13 (SKILL drift).

**Replacement:** One line in §3: "Component schemas — Engine seed (§2) + frozen `engine-interface-spec.md`."

### 1.2 API Surface (§4, ~10 lines)

**Current:** Verbatim copy of "Public API Surface" from `Game Engine Concept and Milestones.md`.

```
- Lifecycle: `init(renderer&, config)`, `shutdown()`, `tick(dt)`.
- Scene: `create_entity()`, `destroy_entity(e)` ...
- Procedural spawners: `spawn_sphere`, `spawn_cube` ...
- Assets: `load_gltf(path) → mesh_handle` ...
```

**Why cut:** Same redundancy problem as §1.1. The agent reads the seed.

**Replacement:** One line in §3: "Public API Surface — Engine seed (§2) + frozen `engine-interface-spec.md`."

### 1.3 Assumptions and Open Questions (§5, ~15 lines)

**Current:** Pre-SpecKit placeholders (e.g., "point-light component shape deferred to E-M6", "whether engine exposes `raycast_all`").

**Why cut:** These belong in the Engine SpecKit output (`docs/planning/speckit/engine/`), not in a reusable pre-hackathon SKILL. A SKILL should be stable across SpecKit runs; these items will change or resolve during the first planning pass.

**Replacement:** One line: "Pre-SpecKit open questions are resolved during engine SpecKit — see `docs/planning/speckit/engine/research.md`."

---

## 2. Condense

### 2.1 Milestone Playbooks (§6.5, ~25 lines → ~10)

**Problem:** Each milestone playbook reads like implementation notes rather than a skill. The agent is supposed to consult the Engine seed for details per §3. Keep only the milestones where the SKILL adds non-obvious guidance; reduce the rest to one-liners.

**Proposed reduction:**

| Milestone | Keep? | Rationale |
|---|---|---|
| E-M1 | **Keep full** | Sync point with game workstream; mocks prerequisite is critical context |
| E-M2 | One line | Asset import path is straightforward; seed covers the cgltf/tinyobj split |
| E-M3 | **Keep PG hint** | The input vs collider/raycast disjointness is the key decision; rest from seed |
| E-M4 | **Keep full (truncated)** | Fixed-timestep + static bodies + impulse response are the high-impact decisions; trim angular integration detail |
| E-M5 | One line | Seek formula is standard; raycast-ahead avoidance is obvious |
| E-M6 | One line | Depends on renderer R-M2/R-M5; engine side is just "collect up to N, upload" |
| E-M7 | One line | Trivial wrapper once R-M5 lands — defer entirely |

**Effect:** ~15 lines saved.

### 2.2 Gotchas (§8, 18 items → ~12)

**Problem:** Some gotchas duplicate or belong in other sections.

| Item | Action |
|---|---|
| "Engine does not drive the loop" | **Keep** — #1 mistake for Unity developers |
| "Procedural mesh builders live in renderer" | **Keep** — #1 cross-workstream conflict |
| "Renderer handles are opaque" | **Keep** — prevents agents from inspecting internals |
| "view() invalidated by structural changes" | **Keep** — critical entt gotcha |
| "Quaternion composition order / GLM column-major" | **Keep** — silent correctness killer |
| "Non-uniform scale + rotated AABB out of scope" | **Keep** — prevents scope creep |
| "Variable-dt Euler tunnels" | **Keep** — behavioral correctness gate |
| "glTF/OBJ paths from ASSET_ROOT" | Fold into §4 asset paths or §6.4 workflow |
| "cgltf two-step (parse + load)" | Fold into §6.5 E-M2 one-liner |
| "tinyobjloader missing normals" | Fold into §6.5 E-M2 one-liner |
| "Asset hot-reload out of scope" | Remove — already in Blueprint cuts, no agent will invent this |
| "Only one directional light at MVP" | Fold into §4 scope tiers |
| "Mock swap must be clean" | Fold into §6.3 mock procedure |
| "ECS component schema is a freeze target" | Fold into §6.2 interface freeze |
| "Top-level CMakeLists.txt NOT engine-owned" | **Keep** — prevents cross-workstream build edits |
| "Agents do not self-claim" | Remove — covered by AGENTS.md Rule 2, not engine-specific |
| "Fixed-timestep substepping with dt cap" | Fold into §6.5 E-M4 |
| "Inverse-mass / inverse-inertia preference" | Fold into §7 decision rules (already there as a separate rule) |

**Effect:** ~6 lines saved by folding + removing 3, keeping 12 high-impact items.

### 2.3 File-Loading Rules (§10, bullet list → compact table, ~8 lines saved)

**Current:** Prose bullet list of "load only what the current task needs."

**Proposed:** Single compact table mapping task type to files:

| Task type | Load |
|---|---|
| Always (once/session) | `AGENTS.md`, this SKILL, `TASKS.md` row, frozen renderer interface (for bridge tasks) |
| Task authoring | Engine seed milestone section, Blueprint §§8.4/10, sibling `TASKS.md` for disjoint-file check |
| Interface freeze | Engine seed Public API + component schema, existing draft spec |
| ECS / scene work | `entt-ecs/SKILL.md` + references |
| Asset import | `cgltf-loading/SKILL.md` + references; `tinyobjloader` notes for OBJ |
| Physics / collision | `physics-euler/SKILL.md` if present; Engine seed otherwise |
| Raw headers | Only when SKILL is insufficient or error names unknown symbol |

**Effect:** ~8 lines saved, same coverage.

### 2.4 Output Structure (§11, ~6 lines → 3 lines)

**Problem:** Mostly procedural items covered by AGENTS.md (queue files, blockers, commit style). Only the mock structure and implementation commit convention are engine-specific.

**Proposed:** Condense to 3 bullets:
- Mocks: `src/engine/mocks/` with CMake toggle `USE_ENGINE_MOCKS`; same signatures as real engine.
- Commits: milestone-scoped, reference task ID (`E-14: Euler integrator for RigidBody`).
- Queues/Blockers: append to `_coordination/queues/VALIDATION_QUEUE.md` / `REVIEW_QUEUE.md` / `_coordination/overview/BLOCKERS.md`.

**Effect:** ~3 lines saved.

---

## 3. Preserve — Do Not Touch

These sections are **high-impact, non-redundant, and decision-critical**:

| Section | Lines | Why keep |
|---|---|---|
| §1 Objective (dual role: implementer + task author) | ~7 | Defines the role's unique responsibility |
| §2 Scope (in/out of scope with handoff pointers) | ~15 | Prevents cross-workstream overreach |
| §3 Authoritative sources | ~8 | Grounding — tells agent where to look, not what to remember |
| §4 Confirmed facts (ownership, loop, build topology, scope tiers, cross-workstream deps) | ~30 | Core facts that agents must not relitigate; most is project-specific not document-duplicate |
| §6.1 Task authoring workflow (full procedure) | ~18 | Unique to this skill; no other doc has the authoring procedure |
| §6.2 Interface freeze procedure | ~8 | Unique to this skill |
| §6.3 Mock delivery procedure | ~8 | Unique to this skill |
| §7 Decision rules (all 10) | ~25 | Each is a real decision point; "prefer thin entt re-exports" and "prefer inverse-mass over mass" are non-obvious hackathon optimizations |
| §9 Validation checklist (all 10) | ~18 | Milestone-specific gates not covered by AGENTS.md |
| §12 Evolution | ~8 | Keeps SKILL alive across milestones |
| Companion files list | ~7 | Navigation aid, low cost |

**Total preserved:** ~154 lines.

---

## 4. Projected Final Size

| Section | Before | After | Saved |
|---|---|---|---|
| §1 Objective | 7 | 7 | — |
| §2 Scope | 15 | 13 | 2 |
| §3 Authoritative sources | 8 | 8 | — |
| §4 Confirmed facts | 52 | 29 | 23 |
| §5 Assumptions/open questions | 15 | 1 | 14 |
| §6 Workflows (incl. 6.5 playbooks) | 70 | 55 | 15 |
| §7 Decision rules | 12 | 12 | — |
| §8 Gotchas | 18 | 12 | 6 |
| §9 Validation | 14 | 14 | — |
| §10 File-loading rules | 10 | 3 | 7 |
| §11 Output structure | 6 | 3 | 3 |
| §12 Evolution | 8 | 8 | — |
| Companion files | 7 | 7 | — |
| **Total** | **~352** | **~270** | **~82 (~23%)** |

---

## 5. Risk Assessment

| Change | Risk | Mitigation |
|---|---|---|
| Cut component schema from §4 | Medium — agent needs it for task authoring | Agent reads Engine seed (§3) + frozen interface spec; both are authoritative sources |
| Cut API surface from §4 | Medium — same as above | Same mitigation |
| Remove open questions (§5) | Low — they're pre-SpecKit artifacts | Move resolution to SpecKit output |
| Condense milestone playbooks | Low — agent has seed + domain skills | Keep E-M1, E-M3 PG hint, E-M4 decision points; rest is standard |
| Fold gotchas | Low — content preserved, just relocated | Cross-check folded items land in the section where they're actionable |

No change risks breaking any decision-critical path. All cut content is either verbatim duplication of source documents or pre-SpecKit artifacts that belong elsewhere.
