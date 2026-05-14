# Proposed Edits to AGENTS.md

> This document lists every section in the current AGENTS.md that should be cut or trimmed, with rationale and replacement guidance. Applied edits bring AGENTS.md to a more manageable size.

## Cut: §2 — Hardware, hostnames, and owner tags (full table)

**Lines:** The hostname/owner markdown table.
**Rationale:** Person names and role assignments are human supervisor information. Agents only need the `agent_name@machine` naming convention and the four hostnames as strings.
**Keep:** "Ubuntu 24.04 LTS everywhere." and the owner tag format explanation (`agent_name@machine`, with `-2`/`-3` suffixes, and `FirstName@machine` for human manual entries).

## Cut: §5 — Task schema and claiming (Claiming protocol subsection)

**Lines:** The entire 5-point "Claiming protocol" subsection.
**Rationale:** This describes how humans claim tasks *for* agents. Agents already know: read TASKS.md for the assigned task. The claiming protocol is process metadata, not agent instructions.
**Keep:** The task row schema table. It tells agents what each column means so they can parse their own TASKS.md rows correctly.

## Cut: §6 — Interfaces and milestones (Milestone merge protocol)

**Lines:** The 8-step "Milestone merge protocol" list.
**Rationale:** Every step (build passes, smoke tests, Spec Validator confirms, human behavioral check, Code Reviewer review, human merges, downstream sync, mock swap) is a human supervisor action. Agents don't run merges or validate against specs.
**Keep:** The frozen interface rule ("never change frozen shared interfaces without explicit human approval") and the definition of "milestone-ready" (all three conditions). These are directly actionable during implementation.

## Cut: §8 — Validation, review, and queues (queue file listing)

**Lines:** The three queue file paths and language about adding entries to them.
**Rationale:** Queue management is a human/planning supervisor activity. During hackathon implementation, agents don't manage validation, review, or test queues — they respect the `Validation` column on their assigned task.
**Keep:** The risk-based validation levels:
- LOW/trivial → SELF-CHECK
- Medium → SPEC-VALIDATE or REVIEW
- High/hard → at least one secondary check
- Every milestone → Spec Validator + human behavioral check + lightweight review
- Testing note (Catch2 for math/parsers/ECS; rendering via human check)

## Cut: §10 — AI tool priority and escalation (entire section)

**Lines:** The 6-tier priority table, agent outage rule, and emergency manual path.
**Rationale:** This is entirely human supervisor orchestration content. An agent running in Claude Code does not decide whether to escalate to Gemini or GLM. The human makes those calls.
**Keep:** Only Rule 12 from §11 (the stashed version of this rule): "Stall >90s → log in BLOCKERS.md." This is the one part actionable by agents who can detect their own stalls. Everything else — priority tiers, escalation chain, `FirstName@machine` takeover — belongs exclusively with humans.

## Cut: §13 — Scope-cut contingency order (entire section)

**Lines:** The 7-item scope-cut list and the T-30 / T-10 min finalization rules.
**Rationale:** Agents will not make scope-cut decisions during implementation. This is a human supervisor contingency plan for when time runs short. Priorities are already encoded in the MVP/Desirable/Stretch breakdown in spec documents; agents follow what their TASKS.md rows specify.
**Keep:** Nothing from this section in AGENTS.md. If humans want it preserved, it belongs in the Human Supervisor Runbook (see companion document).

## Cut: §14 — Source control (branch and worktree names)

**Lines:** The branch names list and worktree names list.
**Rationale:** Agents don't manage branches or worktrees. Humans do.
**Keep:** Rule 8 from §11 ("Pull before starting new work") — already in the global rules and agent-actionable.

## Trim: §15 — If you are stuck

**Lines:** All 5 rules.
**Rationale:** Rules 2 and 5 reference cut content (§10 escalation, BLOCKERS.md logging of agent outages). They lose meaning without that context.
**Keep (renumber):**
1. Check the role SKILL and per-aspect reference for the subsystem.
2. If a frozen interface change is tempting, stop; flag the blocker; wait for human approval.
3. If a file outside your declared set is needed, follow the §7 pause protocol.

**Cut:**
- "If the SKILL is wrong/missing, log in BLOCKERS.md and stop" (BLOCKERS.md logging of skill issues is a human activity).
- "If stalled >90s, escalate per §10" (§10 is cut; the stall rule already lives in §11 Rule 12).

## Trim: §12 — Quality gates

**Lines:** Gates G3–G7.
**Rationale:** G3 (frozen interfaces), G4 (validation completed), G5 (milestone validation), G6 (main demo-safe), G7 (parallel file sets disjoint) are enforced by humans/supervisors, not the implementing agent.
**Keep:** G1 (`cmake --build` returns 0) and G2 (relevant tests pass) — these are directly enforced by agents before marking DONE.

---

## Resulting structure of trimmed AGENTS.md

```
AGENTS.md (~500 lines)
├── §1  What we are building          ← project context, agents need this
├── §2  Hardware + owner convention    ← trimmed: hostnames only, no person names
├── §3  Build stack                   ← agents run builds, handle shaders/paths
├── §4  Repository map                ← agents need file locations
├── §5  Task schema (table only)       ← trimmed: removed claiming protocol
├── §6  Interfaces + milestones        ← trimmed: removed merge protocol
├── §7  Parallel groups + file ownership ← critical agent-executable rules
├── §8  Validation levels              ← trimmed: removed queue management
├── §9  Skills and large headers       ← core agent workflow
├── §10 (removed)                     ← entire section cut → Human Runbook
├── §11  15 global rules               ← trimmed: removed human-only rules
├── §12 Quality gates                  ← trimmed: G1, G2 only
├── §13 (removed)                     ← entire section cut → Human Runbook
├── §14 (removed)                     ← trimmed: branch names cut
└── §15 If you are stuck               ← trimmed: 3 rules only
```
