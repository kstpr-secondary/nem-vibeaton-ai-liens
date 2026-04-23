# Test Author Skill — Compaction Opportunities

> Analysis of `_agents/skills/test-author/SKILL.md` (189 lines) for areas where content can be removed or condensed without losing actionable knowledge.

---

## 1. Sections to remove entirely

### §10 File loading (lazy) — ~10 lines

**Why remove:** Redundant with `AGENTS.md` §8.11 (Lazy Skill Loading) and AGENTS.md Rule 9 ("When working with a library that has a dedicated SKILL, load and follow the SKILL first"). Agents already know the lazy-loading protocol from the root behavior file. The Blueprint cross-references at the end (`§3.1`, `§3.5`, `§8.6`, `§13`) add nothing — an agent that loaded this skill already has the context it needs.

**Impact:** Zero loss of actionable knowledge.

### §11 Evolution — ~12 lines

**Why remove:** Meta-notes for repo maintainers, not actionable guidance for an agent during a hackathon. The three bullet points are speculative ("revisit when..."). Hackathon participants don't need to know when the skill should be revisited post-event.

**Impact:** Zero loss of actionable knowledge.

**Total lines saved: ~22**

---

## 2. Cross-references to consolidate (not remove, but inline)

The skill references `Blueprint §8.6`, `Blueprint §3.1`, `Blueprint §3.5`, `AGENTS.md §7`, `AGENTS.md §10 rule 11` in various places. These force an agent to load a 700+ line Blueprint document or navigate AGENTS.md. Replace with inline one-liners:

| Current reference | Replacement |
|---|---|
| "human behavioral check — Blueprint §8.6" | "human behavioral check only" |
| "2-minute notice (AGENTS.md §10 rule 11)" | "escalate to Renderer / Systems Architect" |
| "`AGENTS.md` §7" | "task's declared `Notes: files:` set" |
| "Blueprint §3.5" (build commands) | already covered by the explicit commands in §4 — no reference needed |
| "`AGENTS.md`" (task status transitions) | "per task row" |

**Impact:** Saves agent context window; eliminates need to load external documents. ~5 lines of text condensed.

---

## 3. Content to add (small, high-signal additions)

### 3a. Parallel group coordination note (~3 lines)

Add to §4 (workflow), after step 6:

> If writing the test requires touching a file outside the task's declared `Notes: files:` set, pause and flag — do not race. Follow AGENTS.md §7 file-ownership rule.

**Why:** RTX 3090 runs Test Author agents in parallel with implementation agents. Without this note, a test author might accidentally edit `physics.cpp` while the engine specialist is editing it, violating the disjoint-file-set rule.

### 3b. "Most important functions" heuristic (~3 lines)

Add to §3 (decision rule), as a new bullet after the "Not worth testing" list:

> **When unsure which function in a task to test:** prioritize the one with the most complex control flow or the most non-obvious math. A buggy `ray_vs_aabb` costs more than a buggy `get_component`.

**Why:** The skill says "test high-impact/high-risk methods" but doesn't give agents a practical way to identify them when looking at an arbitrary task. This heuristic lets agents self-direct without domain expertise.

### 3c. Sharper CMake wiring guidance (~3 lines)

Replace the current §4 step 5 ("Wire into the `_tests` target only if the CMake setup doesn't pick it up automatically") with:

> Check the workstream `CMakeLists.txt` for `file(GLOB ...)` or `target_sources(... PRIVATE ...)`. If globbed, do nothing. If explicit, add one line. If you need to edit a top-level `CMakeLists.txt` (not a workstream subdirectory), flag — do not touch it.

**Why:** The current wording is correct but vague. Agents should know exactly what to look for and when to stop vs escalate.

---

## 4. Estimated result after compaction

| Metric | Before | After |
|---|---|---|
| Total lines | ~189 | ~158 (~16% shorter) |
| Actionable content | 100% | 100% (zero loss) |
| External doc references | 6+ | 0 (all inlined) |
| New additions | — | 3 practical notes (§3a, §3b, §3c) |

---

## 5. Not recommended for removal

The following sections are **essential** and should not be cut:

- **§1 Objective + time budget** — defines the role's purpose and hard stop
- **§2 In/Out of scope** — prevents scope creep (no game tests, no GL context tests)
- **§3 Decision rule** — the most important section; tells agents what to test vs skip
- **§5 Catch2 v3 essentials** — sufficient framework knowledge for agents without prior Catch2 experience
- **§6 Default skeleton** — immediately usable template
- **§7 Decision rules** — hard constraints (no production code edits, no new CMake targets)
- **§8 Gotchas** — real-world pitfalls (glm quat convention, fixture paths, GLM w/x/y/z ordering)
- **§9 Validation checklist** — ensures quality before marking DONE
