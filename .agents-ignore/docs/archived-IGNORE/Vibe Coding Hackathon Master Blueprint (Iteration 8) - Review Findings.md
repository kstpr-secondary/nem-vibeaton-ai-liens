# Review Findings — Vibe Coding Hackathon Master Blueprint (Iteration 8)

## Overall assessment

The blueprint is **directionally sound**: the core strategy of frozen interfaces, mock-first parallelism, milestone sync points, and role separation between implementation / validation / review is logically strong for an AI-assisted hackathon.

However, the document is **not yet fully internally consistent as an operational authority**. The biggest issues are:

1. the worked milestone example contradicts the rules it is supposed to illustrate,
2. milestone numbering and MVP/desirable scope do not line up cleanly,
3. the merge / validation cadence appears too heavy for a 5–6 hour event.

## Concrete contradictions and coherence problems

### 1. Rendering milestone numbering conflicts with rendering scope

> **Comment by the human supervisor:** this is now fixed directly in the document.

- Section 4 says renderer **MVP is Milestones 1–3** and includes **Blinn-Phong shading** and **static skybox**.
- The same section says **Milestones 4–5** are **desirable** work such as alpha blending, normal maps, instancing, and material hooks.
- But the worked example in section 10 defines:
  - **M4 = Blinn-Phong Shading**
  - **M5 = Background Skybox**

This makes milestone-based planning ambiguous: in one place those features are MVP, in another they are milestone 4/5 example work, while milestone 4/5 are also described as desirable non-MVP items.

### 2. The worked example violates the document's own parallel-group rule

> **Comment by the human supervisor:** This section and section 3 - inconsistencies in the example should be fixed. NOTE: The example is only for process clarification, IT DOESN NOT correspond to the actual milestones and tasks. This should be mentioned in the document. Those are to be deviced by the task creation procedure.

Section 10.4 says tasks may only be placed in the same parallel group when their file sets are **disjoint**.

But in the M2 example:

- `R-10` touches `frustum.cpp, frustum.h`
- `R-11` touches `frustum.cpp`
- `R-12` touches `renderer.cpp` and depends on `R-11`

`R-10` and `R-11` are not disjoint, and `R-11`/`R-12` are explicitly sequential, yet all three are shown inside `PG-M2-A`. The example therefore contradicts the rule it is meant to teach.

### 3. The worked example contains internal logic errors

- M2 is placed under **"Milestone Group — Shading"** even though it is **Frustum Culling**.
- Section 10.6 says: **"M4 and M2 are independent"**. This looks like a mistake; later text argues that **M4 and M5** can proceed in parallel.
- M5 is described as **"only touches skybox files"** and **"fully parallel with M4 since file sets are disjoint"**, but `R-23` touches `renderer.cpp`, and M4's `R-20` also touches `renderer.cpp`. Those file sets are not disjoint.

These are not cosmetic errors; they weaken trust in the main concurrency model.

### 4. "Freeze docs and task lists" conflicts with live coordination workflow

At hackathon start, section 11.2 says:

- "Freeze docs and task lists."

But elsewhere:

- `TASKS.md` is the **operational source of truth**
- claiming a task requires editing `TASKS.md`
- file-set changes must update `TASKS.md`
- progress and blockers are updated during execution

So "freeze task lists" cannot literally be correct. Likely the intent is to freeze **initial milestone structure / interface contracts**, not the live coordination files.

> **Comment by the human supervisor:** most likely freeze the tasks structure (numbering, description), but not status, ownership, etc.

### 5. Repository structure is underspecified relative to the project concept

Section 1 says the three projects live in **separate directories** in one repo.

But the high-level tree in section 8.3 only ends with:

- `shaders/`
- `src/...`

This leaves the physical code layout ambiguous at exactly the point where branch ownership, include paths, generated interfaces, and mock placement matter most.

> **Comment by the human supervisor:** In my opinion the render engine, game engine and game directories should be in src/. Shaders location should be precised too, since shaders can be created by the rendering engine (built-in lambertian, phong, pbr, shadow passes, etc.) or the game - special effect shaders (explosions, plasma pulses).

### 6. Status convention mentions `VERIFY BEFORE HACKATHON` but no such items appear

The document status line says items requiring empirical verification are marked **VERIFY BEFORE HACKATHON**, but the document appears not to contain any actual tagged instances. That suggests either:

- those checks were resolved and the status note is stale, or
- unresolved assumptions still exist but are not called out consistently.

> **Comment by the human supervisor:** most likely removed from the document or hallucinated by the agent that composed the document. I removed this mention in the preamble.

## Feasibility risks

### 7. Validation overhead is likely too high for the time budget

The blueprint requires, for **every milestone**:

- build/smoke pass,
- spec validation,
- human behavioral check,
- lightweight review,
- merge,
- downstream sync where relevant.

At the same time, section 10.7 targets **one milestone merge every 25–35 minutes per workstream**.

In a 5-hour window, that implies roughly **24–36 milestone merges across three workstreams** if followed literally. That is probably too much coordination overhead for a three-person team, especially when humans are also supervising agents and resolving integration issues.

> **Comment by the human supervisor:** Should we do roughly 5 merges per workstream then (one per hour)? 

### 8. Task-claim commit/push protocol may create coordination thrash

The rule to claim a task by editing `TASKS.md`, committing, and pushing whenever concurrency exists is safe in principle, but expensive in practice if task granularity is small.

Risk:

- many tiny claim commits,
- frequent pulls to stay current,
- humans spending time on coordination instead of supervision,
- agents blocked on bookkeeping rather than coding.

This is especially risky because the plan also encourages many parallel agents and short milestone cadence.

> **Comment by the human supervisor:** I would suggest that the human in charge to do the commit. Tasks execution is triggered by the human nontheless (e.g. pastes in CLI "Implement Task X - <short description to trigger proper skill usage>").

### 9. T+0 generation of shared API headers and mocks is risky unless fully rehearsed

The hackathon start sequence includes:

1. generating shared API headers from the interface spec,
2. generating mock implementations,
3. then starting all three workstreams.

This is feasible only if the generation path is already deterministic and dry-run proven. Otherwise, the event begins with toolchain/debugging work instead of visible delivery.

For a 5–6 hour hackathon, any "generate at T+0" step should be treated as high-risk unless it is already rehearsed to near-zero friction.

> **Comment by the human supervisor:** Taken into consideration. Can be added in the blueprint in the pre-hackathon phase.

## Important missing or under-specified considerations

### 10. Dependency acquisition / offline resilience is not addressed strongly enough

The stack relies on **FetchContent only** for all dependencies. That is simple, but it creates risk if:

- network access is slow or flaky,
- GitHub rate limits or transient outages occur,
- one machine misses caches and does a cold fetch during the event.

The checklist verifies tools are installed, but it does not explicitly require **pre-warmed dependency caches** or vendored fallback sources.

> **Comment by the human supervisor:** We expect good network and github to be working properly. Still we should have the dependencies downloaded before the hackathon. Should be noted in the blueprint.

### 11. Asset/runtime path handling is a probable demo-day failure mode

The blueprint says assets are pre-verified and runtime GLSL loading is the default, but it does not spell out:

- working-directory assumptions,
- how shaders/assets are copied into build output,
- relative path policy,
- what the one-command demo launch path is.

For demo reliability, this is a common failure point and deserves explicit pre-hackathon treatment.

> **Comment by the human supervisor:** Agreed, should be noted in the blueprint. Should this be decided by the spec-kit planning phase or in pre-planning phase? 

### 12. A real runbook should not be optional

Section 8.9 says `quickstart.md` is optional. For a time-boxed live demo, a short runbook is not really optional; it is part of operational reliability.

At minimum, the team should have one authoritative note covering:

- setup assumptions,
- build command,
- run command,
- asset/shader path assumptions,
- fallback demo steps if one workstream slips.

> **Comment by the human supervisor:** Agreed on this, should be noted in the blueprint
### 13. Final demo strategy is implied but not explicit

The document says `main` remains demo-safe, but does not clearly define:

- which machine is the final demo machine,
- when the final branch freeze happens,
- what the fallback is if one feature branch misses its last milestone,
- whether the demo always runs from `main` or may run from `integration`.

That decision matters because the branch policy currently leaves too much room for ad hoc judgment under time pressure.

> **Comment by the human supervisor:** This will be decided on the spot. The most capable machine is the RTX 3090 one. We will run whatever is pushed to main when time ends, branch freeze is not considered. It's pointless to plan the last minutes of the hackathon as things will most likely go wrong long before that and only improvisation will save the day.

## Bottom line

The plan is **conceptually strong but operationally brittle in its current wording**.

The main architecture is sound:

- separate workstreams,
- frozen interfaces,
- mock-first startup,
- milestone sync,
- task/validation/review separation.

The main problems are:

1. the worked example is internally inconsistent and undermines the concurrency model,
2. milestone numbering and MVP scope need one unambiguous interpretation,
3. the merge/validation process is probably too heavy for the claimed cadence,
4. offline dependency handling, asset/runtime paths, and final demo runbook need stronger treatment.

If those points are corrected, the blueprint becomes much more credible as a real hackathon operating plan.
