# Renderer Specialist SKILL.md — Audit & Compaction Review

> Date: 2026-04-22
> Auditor: Systems Architect / Skill Creator agent
> Source files reviewed: `.agents/skills/renderer-specialist/SKILL.md`, `pre_planning_docs/Hackathon Master Blueprint.md` (iter 8), `pre_planning_docs/Renderer Concept and Milestones.md`

---

## 1. Overall Verdict

The renderer-specialist SKILL.md is **well-designed** and follows the "domain skills for library knowledge, agent skill for project knowledge" pattern correctly. The agent has all tools needed to both implement milestones and author/sanction tasks. There is approximately **10% fluff** that can be safely removed without losing any operational knowledge.

---

## 2. Responsibilities — Assessment

**Well presented.** Section 1 lists four core responsibilities (task authorship, implementation, interface freezing, demo safety) mapped to concrete workflows in §6 and gates in §9. Section 2 (Scope) and its out-of-scope handoffs are explicit. Decision rules (§7) and gotchas (§8) capture the high-impact judgment points an agent needs to make autonomously.

---

## 3. Domain Knowledge Distribution

The skill correctly follows the "high-impact project knowledge only, domain details in dedicated skills" principle. No duplication of library API cheat-sheets. Distribution is as follows:

| Knowledge area | Where it lives |
|---|---|
| Sokol API (descriptors, buffers, passes, draws, compute) | `.agents/skills/sokol-api/` + per-aspect references |
| sokol-shdc shader dialect + CMake integration | `.agents/skills/sokol-shdc/SKILL.md` |
| GLSL patterns (lighting math, skybox tricks, alpha, line-quad gen) | `.agents/skills/glsl-patterns/SKILL.md` |
| Uniform-block layout rules (std140, vec3 padding) | §4 + §8 gotcha |
| Normal matrix CPU-side computation | §6.4 R-M2 + §7 decision rule + §8 gotcha |
| Skybox depth strategy (`pos.xyww`) | §6.4 R-M3 + §8 gotcha |
| Alpha-blending MVP vs sorted queue (R-M3 vs R-M5) | §6.4 R-M3 + §8 gotcha |
| Magenta pipeline fallback behavior | §4 + §6.3 step 7 + §9 gate 5 |
| Asset path resolution via `ASSET_ROOT` macro | §4 |
| Line-width portability (glLineWidth not portable in GL 3.3 Core) | §8 gotcha |

The skill does **not** try to teach general computer graphics, real-time rasterization, 3D math, C++, or shader concepts — it assumes these come from the agent's base capabilities plus the domain skills. This is the correct design.

---

## 4. Agent Capability Assessment

**The agent has all tools needed.** The skill covers:

- **Task authoring** (§6.1): full schema, parallel-group rules, validation column guidance
- **Interface freeze** (§6.2): version markers, symbol declarations, migration workflow
- **Implementation loop** (§6.3): read → load skills → implement → build → behavioral check → queue
- **Milestone playbooks** (§6.4): R-M0 through R-M5 sequenced cheat-sheets with exact shader math and sokol patterns
- **File-set violation handling** (§6.5): pause → update task → check conflicts → flag blocker
- **Validation gates** (§9): 8-item checklist mapping to AGENTS.md G1–G7

---

## 5. Compaction Opportunities — What to Cut

### 5.1 YAML front matter boilerplate (~3 lines)

**Lines 1–10.** The `compatibility` line ("Portable across heterogeneous agents") is redundant with AGENTS.md. `metadata.author` and `metadata.version` are never consumed by any agent tooling.

**Action:** Keep only `name` and `description`. Remove `compatibility`, `metadata.author`, `metadata.version`, `project-stage`, `role`, `activated-by`.

### 5.2 Description field duplicates §2 (~1 line)

**Line 3–4 of the description.** The "Do NOT use for..." exclusions (engine ECS, gameplay, cross-workstream planning) are already enumerated in §2 (Out of scope).

**Action:** Trim the description to the activation trigger only. Remove the exclusion list.

### 5.3 §3 (Project grounding) and §4 (Confirmed facts) overlap (~10 lines)

§4 restates everything from the Blueprint and Renderer seed that §3 already points at via its source list. For example, "OpenGL 3.3 Core only" appears in both §3 item 5 and §4 Backend section. "sokol-shdc precompiled" appears in both places. The same for ownership, API surface, scene philosophy, build topology, asset paths, and scope tiers.

**Action:** Merge §3 into a single short bullet list (3 items max — Blueprint, Renderer seed, AGENTS.md). Fold the confirmed facts from §4 into the merged section or keep §4 as-is but remove the redundant restatements.

### 5.4 §10 (File-loading rules) duplicates §6.3 step 2 (~15 lines)

§6.3 step 2 already says "Load only the domain skills this task needs" and lists the exact same skill paths (`sokol-api/SKILL.md`, `sokol-shdc/SKILL.md`, `glsl-patterns/SKILL.md`). §10 repeats this as a longer table with the same content.

**Action:** Collapse §10 to one sentence: "Load only what the current task needs — see §6.3 step 2 for skill-loading order. Do not pre-load heavy headers."

### 5.5 §12 (Evolution) is hackathon-irrelevant (~7 lines)

Documents post-hackathon re-visit triggers (R-M0 lands, R-M1 merges, first magenta trigger, first cross-workstream break). These will not happen in a 5-hour hackathon window.

**Action:** Drop entirely or reduce to one line: "Post-hackathon: split into renderer-specialist and renderer-reviewer if the role grows."

### 5.6 §11 (Output structure) is AGENTS.md boilerplate (~8 lines)

The TASKS.md row format, commit message convention (`R-14: upload...`), queue-append behavior, and blocker format are all covered by AGENTS.md §5 (§8 rules). This section adds nothing extra.

**Action:** Replace with one sentence: "Follow AGENTS.md output conventions for task rows, commits, queues, and blockers."

### 5.7 Summary of cuts

| Section | Lines removable | Risk |
|---|---|---|
| YAML front matter (boilerplate) | ~3 | None |
| Description exclusions (dup of §2) | ~1 | None |
| §3/§4 overlap | ~10 | Low — merge, don't delete |
| §10 vs §6.3 step 2 | ~15 | None |
| §12 (post-hackathon evolution) | ~7 | None |
| §11 (AGENTS.md boilerplate) | ~8 | None |
| **Total** | **~40 lines** (~13% of 311)** | **None** |

---

## 6. What Must NOT Be Cut

Everything else is high-signal. The following sections are essential and must remain intact:

- **§6.4 Milestone playbooks (R-M0 through R-M5):** These are the cheat-sheet that prevents the agent from repeatedly looking up specs. Contains exact shader math formulas (`max(dot(N, -L), 0.0) * base_color * light_color * intensity`), pipeline setup patterns, and per-milestone file lists.
- **§7 Decision rules:** Project-specific preferences (CPU normal matrices over vertex-shader inverse, magenta fallback over exceptions, SEQUENTIAL over PG, scope-cut order). These encode judgment that a generic graphics expert would not know.
- **§8 Gotchas:** Uniform-block layout gotchas, skybox depth, line widths, re-entrancy safety, sokol-shdc compile-time vs runtime error surfaces. These are derived from actual pain points and prevent costly mistakes.
- **§9 Validation gates:** The 8-item pre-merge checklist mapping to G1–G7. This is the agent's self-check before marking anything DONE.
- **§4 Confirmed facts (core content):** API surface, backend constraints, ownership model, scope tiers, build topology, asset path policy. These are the reference facts the agent needs for every task.

---

## 7. Recommended New Section: Pre-Compaction Checklist

Before applying any cuts, verify these still hold after compaction:

1. Every milestone playbook (§6.4) references its corresponding Renderer seed section.
2. All file paths in §10/§6.3 skill-loading list are accurate (`.agents/skills/sokol-api/`, `sokol-shdc/SKILL.md`, `glsl-patterns/SKILL.md`).
3. The `sg_directional_light_t` BOTTLENECK pattern is still documented (§6.4 R-M2 + §7).
4. The magenta fallback acceptance criterion is visible to the agent at implementation time (appears in both §6.3 step 7 and §9 gate 5).
