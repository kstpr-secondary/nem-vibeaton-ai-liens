# Renderer SpecKit Tasks

> Translate final SpecKit output into `_coordination/renderer/TASKS.md` using the stable schema from the blueprint.

## Task shaping rules

- Use milestone IDs `R-M0` through `R-M5+` from the renderer concept doc.
- Use `PG-<milestone>-<letter>` only when sibling tasks have disjoint file sets.
- Mark true shared-header blockers as `BOTTLENECK`.
- Keep `Validation` aligned with milestone risk; do not silently downgrade.

## Expected translation targets

- `_coordination/renderer/TASKS.md`
- `docs/interfaces/renderer-interface-spec.md`
- `docs/architecture/renderer-architecture.md`
