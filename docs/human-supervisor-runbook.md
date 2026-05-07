# Human Supervisor Runbook

> Extracted from AGENTS.md. Contains all process, coordination, and escalation content that belongs with human supervisors — not implementing agents. Keep this document accessible to the human team.

---

## 1. Task Claiming Protocol

Task management flows through the feature's `tasks.md` or the planning section of the design document.

### How it works

1. **Human claims a task** by setting the `Owner` and updating `Status` to `CLAIMED`.
2. The human then triggers the assigned agent on the appropriate worktree.
3. Agent reads the claimed row and the spec it references before editing code.
4. When done, agent sets `Status = READY_FOR_VALIDATION` (or `READY_FOR_REVIEW` / `DONE` per the task's validation requirements).

### Owner tag format

| Format | When to use | Example |
|---|---|---|
| `<agent_name>` | AI agent implementation | `claude`, `gemini`, `copilot` |
| `FirstName` | Human manual path (emergency) | `Alice` |

- Agent names are lowercase: `claude`, `gemini`, `copilot`, `glm`, `local-qwen`.
- Multiple instances of the same agent: append `-2`, `-3` → `claude-2`.

---

## 2. Milestone Merge Protocol

Run this full protocol whenever a milestone includes tasks that touch frozen shared interfaces (`docs/interfaces/`).

### Full merge protocol (interface-touching milestones)

1. All milestone tasks marked `DONE`.
2. `cmake --build build` passes with exit code 0.
3. Smoke tests pass (game launches, basic interaction works).
4. Spec Validator confirms acceptance criteria are met against the spec documents.
5. Human performs a behavioral check — runs the feature interactively and verifies it behaves as described.
6. Code Reviewer performs a lightweight review — scans for obvious risks, bugs, or regressions.
7. Human merges the feature branch → `main`.
8. Other workstreams fetch and sync (`git pull`).
9. Mocks are swapped for real implementations where applicable (human flips CMake toggle, verifies build).

### Lightweight merge protocol (intra-workstream, no interface changes)

1. All milestone tasks marked `DONE`.
2. Per-workstream build passes: `cmake --build build --target <target>`.
3. Human spot-checks the feature visually or via smoke test.
4. Push to feature branch; merge to main.

### Milestone-ready definition

A milestone is ready for merge only when **all three** are true:
- Acceptance checklist is fully met (all tasks DONE).
- Required validation is complete.
- Human behavioral check is done (interactive verification).

---

## 3. Validation and Review

### Risk-based validation levels

| Task Tier | Required Validation | Who performs it |
|---|---|---|
| LOW / trivial | `SELF-CHECK` | Implementing agent |
| MEDIUM | `SPEC-VALIDATE` or `REVIEW` | Spec Validator / Code Reviewer |
| HIGH / hard | At least one secondary check before merge | Spec Validator + Code Reviewer |
| Every milestone | Spec Validator + human behavioral check + lightweight Code Review | All three roles |

### Testing scope

- **Catch2** for math, parsers, ECS logic — unit-testable behavior.
- **Rendering correctness** verified via human behavioral check + smoke-test visuals, NOT unit tests.

---

## 4. AI Tool Priority and Escalation

### Tool priority (use highest-priority available tool first)

1. **Claude Code Sonnet** — primary for hard/ambiguous tasks
2. **GitHub Copilot** — primary backup; medium-task worker
3. **Gemini CLI** — validation, review, research, overflow

### Escalation triggers

| Trigger | Action |
|---|---|
| Agent stalls >90 seconds | Switch to next priority tier |
| Agent fails a task twice | Switch to emergency manual path |

---

## 5. Scope-Cut Contingency Order

If time runs short, cut features in this order:

1. Blinn-Phong shading (fallback to Lambertian only)
2. Diffuse textures (solid colors only)
3. Custom shader hook / shader-based explosion VFX
4. Asteroid-asteroid collisions
5. Capsule mesh builder

---

## 6. Source Control Protocol

### Branches

```
main  /  feature/<name>
```

### Rules

- `main` stays demo-safe at all times.
- Always `git pull` before starting new work.
