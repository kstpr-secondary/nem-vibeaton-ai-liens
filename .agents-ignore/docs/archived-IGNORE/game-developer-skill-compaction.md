# Game Developer SKILL — Compaction Opportunities

Audit of `.agents/skills/game-developer/SKILL.md` (365 lines). Identifies redundancy, cut candidates, and missing practical gaps relative to the project's actual needs.

---

## Current State

| Metric | Value |
|--------|-------|
| Total lines | 365 |
| Sections | 12 (§1 Objective → §12 Evolution) + Companion files |
| Inline "use when" description (line 3) | ~140 chars of dense text |
| Expert-in summary (line 17) | ~110 chars |

## Cut Candidates (no knowledge loss)

### 1. Line 3 — Inline "use when..." description (~20 lines)

**Current:** Massive single-paragraph block listing every component, mechanic, exclusion, and handoff. Covers player controller, camera rig, asteroid field, weapons, enemy AI, HP/shield/boost, explosion VFX, ImGui HUD, game flow, ECS components, task authoring, SpecKit hooks, and every cross-workstream handoff in one breath.

**Problem:** Agents never read this line as prose. It is parsed as a machine-compatible `description` field by the skill-loading system. The detail belongs in the body sections below.

**Action:** Reduce to 2 lines — what role this is, when to activate it, and the single most important exclusion (renderer/engine internals). Everything else is already covered in §1–§6.

### 2. Line 17 — "Expert in" summary vs §4 redundancy (~3 lines)

**Current:** Lines 16-18 state "Expert in: C++17, real-time game loops, 3D math..., entt ECS usage..., camera rigs..., navigation/motion controls..., Dear ImGui..., game-feel tuning..., asset pipelines..." — then §4 "Confirmed facts" restates controls, resources, weapons, asteroids, enemies, HUD, and milestones in far more detail.

**Action:** Collapse "Expert in" to a single opening line in the §1 intro. Let §4 be the authoritative reference. No information is lost; §4 already has all of it plus more.

### 3. §5 Assumptions and Open Questions (~15 lines)

**Current:** 5 assumed conservative placeholders + 5 open questions to escalate.

**Problem:** Open questions belong in `TASKS.md` or a dedicated "Open Questions" doc. They are *living* state, not skill knowledge. The assumptions are mostly reasonable but some (Dear ImGui initialization ownership, third-person smoothing formula) can be verified from the frozen interfaces before writing code — they don't need to live in the skill permanently.

**Action:** Keep 2-3 core assumptions that cannot be resolved from interfaces alone (e.g., "Ship orientation stored as quaternion on Transform"). Move all open questions out. Saves ~12 lines.

### 4. §12 Evolution (~15 lines)

**Current:** Post-hackathon retrospective prompts — record camera constants after G-M1, capture speed caps after G-M2, note damage numbers after G-M3, etc.

**Problem:** This is useful for a post-mortem but has zero operational value during implementation. An agent running G-M3 does not need to know what to record *after* G-M4 lands.

**Action:** Move to `docs/` as a retrospective checklist, or cut entirely. Saves ~15 lines.

### 5. §3 Project grounding — numbered cross-reference list (~8 lines)

**Current:** Seven numbered items pointing at Blueprint sections, Game seed, frozen interfaces, GAME_DESIGN.md, AGENTS.md, and entt skill.

**Problem:** These are files the agent loads anyway per §10 (lazy loading rules). The list adds nothing actionable — it just duplicates what §3.5 of the Blueprint already says about authoritative sources.

**Action:** Replace with a single bullet: "Load once per session: Blueprint (§§3,6,7,8,10,13,15), Game seed, frozen engine+renderer interfaces, `GAME_DESIGN.md`, `AGENTS.md`, `entt-ecs/SKILL.md`." Saves ~6 lines.

## Missing Practical Items (add to preserve capability)

### 6. C++17 pattern reminder for Unity/C# devs (+2 lines)

**Gap:** Team is "rusty C++ syntax" per Blueprint §1.3 — three expert Unity/C#/3D-math programmers. The skill says "Expert in: C++17" but provides zero C++ guidance. Agents will stall on `std::optional` return patterns, struct vs class default visibility, or initializer lists.

**Add to §4 intro or §7:** One-liner reminder: "C++ reminders for C# devs — `struct` defaults are public (opposite of C#), use `std::optional<T>` for nullable returns, prefer `{}` init over `()` constructor calls."

### 7. ImGui widget pattern for HUD bars (+3 lines)

**Gap:** The skill mentions `ProgressBar`, `Text`, `GetForegroundDrawList` in the gotchas but doesn't show the actual widget pattern. A C# dev writing C++ ImGui for the first time needs to see:

```cpp
ImGui::ProgressBar(shield / 100.0f, ImVec2(200, 20), "Shield");
```

**Add to §6.4 G-M4 section:** One line showing the ProgressBar pattern before agents start writing HUD code.

### 8. Mock return-value example (+3 lines)

**Gap:** The skill says mocks have identical signatures and game code does not branch on mocks vs real — good. But agents don't know what mocked functions *return*. A mock `raycast()` might return `{hit: false, entity: nullopt}` or throw — without knowing the contract, agents write mock-breaking code.

**Add to §6.2:** One line: "Mock behavior: `raycast` returns `{hit: false, entity: nullopt}`, physics tick is a no-op, `spawn_*` creates entities with default-transform components. Check mock implementations in `src/engine/mocks/` or `src/renderer/mocks/` before writing code that depends on return values."

### 9. Strengthen `tuning.h` statement (+2 lines)

**Current:** §11 mentions "game code references these numbers via a single `tuning.h`" in passing.

**Problem:** This is critical hackathon discipline — agents must not scatter magic numbers across systems. It deserves a rule-strength statement.

**Add to §7 Decision rules:** "All tuning values (damage, cooldowns, speeds, masses, sizes) live in a single `tuning.h` header. Never hard-code them in system code."

## Summary

| Change | Lines Saved | Lines Added | Net |
|--------|------------:|------------:|----:|
| Cut line 3 ("use when") | ~20 | 2 | -18 |
| Collapse "Expert in" (line 17) | ~3 | 0 | -3 |
| Trim §5 open questions | ~12 | 0 | -12 |
| Cut §12 Evolution | ~15 | 0 | -15 |
| Condense §3 cross-references | ~6 | 1 | -5 |
| Add C++17 reminder | — | 2 | +2 |
| Add ImGui widget pattern | — | 3 | +3 |
| Add mock return example | — | 3 | +3 |
| Strengthen tuning.h rule | — | 2 | +2 |
| **Total** | **-56** | **+13** | **-43** |

**Target: ~320 lines (from 365), same operational coverage, faster context loading, better scannability for agents.**
