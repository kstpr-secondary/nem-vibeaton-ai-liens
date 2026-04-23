# Renderer SpecKit Tasks

> **SpecKit planning outputs are COMPLETE.**  
> This file is a guide copy maintained under `docs/`. See live operational files below.

---

## Live Operational Files (authoritative for hackathon execution)

| Artifact | Stable Location | Status |
|---|---|---|
| Task board | `_coordination/renderer/TASKS.md` | **Active — claim tasks here** |
| Progress tracker | `_coordination/renderer/PROGRESS.md` | Updated per milestone |
| Interface spec | `docs/interfaces/renderer-interface-spec.md` | **FROZEN — v1.0** |
| Architecture | `docs/architecture/renderer-architecture.md` | **FROZEN** |
| Cross-workstream arch | `docs/architecture/ARCHITECTURE.md` | Active |
| Interface index | `docs/interfaces/INTERFACE_SPEC.md` | Active |

---

## SpecKit Source Artifacts (read-only planning reference)

| Artifact | Location |
|---|---|
| Feature spec | `specs/001-sokol-render-engine/spec.md` |
| Implementation plan | `specs/001-sokol-render-engine/plan.md` |
| Research / decisions | `specs/001-sokol-render-engine/research.md` |
| Data model | `specs/001-sokol-render-engine/data-model.md` |
| API contracts (source) | `specs/001-sokol-render-engine/contracts/renderer-api.md` |
| SpecKit tasks (source) | `specs/001-sokol-render-engine/tasks.md` |
| Quickstart | `docs/planning/speckit/renderer/quickstart.md` |

---

## Task shaping rules (reference)

- Use milestone IDs `R-M0` through `R-M6` from the renderer concept doc.
- Use `PG-<milestone>-<letter>` only when sibling tasks have disjoint file sets.
- Mark true shared-header blockers as `BOTTLENECK`.
- Keep `Validation` aligned with milestone risk; do not silently downgrade.
