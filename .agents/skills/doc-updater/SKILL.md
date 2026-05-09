---
name: doc-updater
description: Updates live reference docs in `docs/` to reflect what was actually built after a feature's final checkpoint passes. Run once per feature, before archiving. Covers interfaces, architecture, and game-design docs at both workstream and top-level layers.
compatibility: Portable across agents. No model-specific behaviors.
metadata:
  version: "1.0"
  role: doc-updater
  activated-by: human-explicit (feature lifecycle step 1)
---

# Doc Updater

Brings `docs/` in sync with what was actually built. Run **once**, after the final phase checkpoint passes and the human signs off, before the feature directory is moved to `features/archive/`.

This skill does not plan or implement. It reads completed feature artifacts, determines the scope of change, and surgically updates the relevant docs.

---

## When to invoke

From `WORKFLOW.md` Feature Lifecycle step 1: after all `checkpoint-pN.md` files are written and the human confirms the feature is complete.

**Do not invoke mid-feature.** Docs reflect the final shipped state, not intermediate phases.

---

## Step 1: Read the completed feature artifacts

From `features/active/<feature-name>/`:

1. `brief.md` — what the feature was supposed to do and its success criteria.
2. All `checkpoint-pN.md` files — what was actually verified and any surprises.
3. All `plan-pN.md` files — what tasks were executed (check `[x]` statuses).
4. `design.md` if present — intended data structures and module interactions.

Read them all before touching any doc. The checkpoints are the ground truth — if a checkpoint notes a deviation from the plan, the docs reflect the checkpoint, not the plan.

---

## Step 2: Classify the doc impact

For each category below, decide YES (needs update) or NO (not affected):

| Category | Needs update when… |
|----------|-------------------|
| **Interface spec** | A public API was added, removed, or signature-changed. Includes new function, new struct field, new component, new enum value. |
| **Architecture doc** | A subsystem's internal design changed — new module, new pass, changed data flow, new dependency. |
| **Game design doc** | A gameplay mechanic, tuning value, or system behavior changed in a way that contradicts or extends the written design. |

If a category is not affected, skip it entirely. Unnecessary edits to stable docs create noise.

---

## Step 3: Identify the specific doc files

### Interface specs (`docs/interfaces/`)

| File | Update when… |
|------|-------------|
| `renderer-interface-spec.md` | Renderer's public API changed (new `enqueue_*`, new `Material` field, new `RendererShaderHandle`, etc.) |
| `engine-interface-spec.md` | Engine's public API changed (new `engine_*` function, new component field, new spawner) |
| `game-interface-spec.md` | Game-layer interface changed (if such a contract exists) |
| `INTERFACE_SPEC.md` | Top-level summary of all interfaces — update if the summary table is stale after workstream-level changes |

### Architecture docs (`docs/architecture/`)

| File | Update when… |
|------|-------------|
| `renderer-architecture.md` | Renderer subsystem internals changed (new pipeline, new pass, new shader, new resource type) |
| `engine-architecture.md` | Engine subsystem internals changed (new system, changed physics model, new asset import path) |
| `game-architecture.md` | Game subsystem changed (new state machine state, new system, new component) |
| `ARCHITECTURE.md` | Top-level cross-workstream architecture summary — update if workstream relationships changed |

### Game design docs (`docs/game-design/`)

| File | Update when… |
|------|-------------|
| `game-workstream-design.md` | Gameplay mechanics, weapons, enemy behavior, tuning intent changed |
| `GAME_DESIGN.md` | Top-level game design summary — update if the high-level design intent changed |

---

## Step 4: Read each targeted doc and update

For each doc identified in Step 3:

1. **Read the current doc.**
2. **Identify the specific sections that are stale.** Do not rewrite sections that are accurate.
3. **Make surgical edits** — update the stale section, add new entries where needed, remove entries that no longer exist.
4. **Preserve the document's existing structure.** Do not reorganize or reformat.

### Interface spec update rules

- Add new API entries in the same format as existing ones (function signature + one-line description).
- Update changed signatures in-place.
- Mark removed entries as removed — do not silently delete without noting the removal (e.g., *Removed in [feature name]*).
- Do not add implementation notes (e.g., "uses sg_make_pipeline internally"). Interface specs are public contracts only.

### Architecture doc update rules

- Describe structural changes at the module level, not at the line-of-code level.
- If a new module was added, add it to any module inventory or diagram description.
- If a dependency changed, update the dependency description.
- Do not re-describe implementation details that belong in code comments.

### Game design doc update rules

- Reflect what was actually shipped, not what was intended if the checkpoint deviated.
- Update tuning values if they differ from what is documented (the code's `tuning.h` is authoritative).
- If a mechanic was intentionally not implemented, note it as "out of scope for MVP" rather than removing it from the doc.

---

## Step 5: Report what changed

After all edits:

```
## Doc Update Report: [Feature Name]

**Updated**
- `docs/interfaces/renderer-interface-spec.md` — added [X], removed [Y]
- `docs/architecture/engine-architecture.md` — updated [section]

**No change needed**
- `docs/game-design/GAME_DESIGN.md` — feature did not affect top-level design
- [other files confirmed accurate]

**Skipped**
- [any files not examined and why]
```

---

## What the Doc Updater does NOT do

- Does not write new docs from scratch. It updates existing docs only.
- Does not create `docs/` files for features that don't affect them.
- Does not update `features/` artifacts (those are historical record, not living docs).
- Does not change `docs/planning/` — those are historical speckit outputs and are never updated.
- Does not archive the feature directory — the human does that after verifying the report.
- Does not review code quality or spec compliance — that is `spec-validator` and `code-reviewer`.
