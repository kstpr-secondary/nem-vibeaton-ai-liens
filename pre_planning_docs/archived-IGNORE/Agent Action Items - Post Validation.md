# Agent Action Items — Post Validation Findings

> Generated after deep cross-document analysis of:
> - `Hackathon Master Blueprint.md` (Iteration 8)
> - `Renderer Concept and Milestones.md`
> - `Game Engine Concept and Milestones.md`
> - `Game Concept and Milestones.md`
> - `Agent Handoff Prompt.md` (previous review)
>
> Validates mutual consistency with the agentic workflow in the blueprint, assesses realism within the 5–6 hour time frame, and produces actionable scope adjustments.

---

## 1. Validation Summary: Agentic Workflow Consistency

The three workstream concept documents are **largely consistent** with the Master Blueprint's agentic workflow. The mock-first strategy, frozen interface contracts, parallel group design, bottleneck markers, and milestone merge protocol all align across documents. Three workstreams can start at T+0 against mocks; each milestone swap follows the blueprint's §9.2 protocol.

**Consistent areas:**
- Renderer owns `sokol_app` init, main frame callback, input event pump; engine ticks from inside frame callback; game uses engine API. All four documents agree. (§3.3 Blueprint + all three concept docs)
- Build topology: `renderer` static lib → `engine` static lib → `game` executable. Matches §3.5 build commands.
- Procedural shape builders owned by renderer, consumed by engine via wrappers. All three concept docs agree.
- Mock-first strategy: game runs against mocks at T+0; engine runs against renderer mocks; each milestone merge swaps mocks for real. Consistent with blueprint §11.2.
- Parallel group design: file-disjoint boundaries declared in `Notes`. Concept documents identify natural parallel splits (e.g., E-M3 input vs collider/raycast).
- Bottleneck markers: R-M2 uniform block struct correctly identified as BOTTLENECK in Renderer Concept.
- Validation granularity: risk-based per task, mandatory per milestone. Consistent with blueprint §8.6.

**Agent routing alignment:**
- Hard graphics/math → Claude Sonnet (Blueprint §7.2 matches concept document complexity)
- Tests / narrow spec checks → local RTX 3090 model (§7.3 limits respected — each concept doc's milestones are scoped to single-file or file-disjoint tasks suitable for bounded context)

---

## 2. Inconsistencies Found (Must Fix Before SpecKit)

### I1: Vulkan vs OpenGL Backend Conflict (HIGH)

**Where:** `Hackathon Master Blueprint.md` §3.3, §10.5 examples, §15 item 18, §14 Pre-Hackathon Checklist
**vs:** `Renderer Concept and Milestones.md` "Resolved Open Decisions" — OpenGL 3.3 Core (Fixed)

**Problem:** The blueprint still has Vulkan as primary backend with OpenGL 3.3 fallback via build flag. The Renderer Concept explicitly removed Vulkan for hardware compatibility, fixing OpenGL 3.3 Core only. This is a critical inconsistency that will confuse agents during implementation.

**Impact:** Agents following the blueprint would attempt Vulkan init in R-M0, potentially stalling on driver/backend issues. The previous feasibility review also flagged this as an open decision. The Renderer Concept has already resolved it — the blueprint needs to catch up.

> **Human Comment**: OpenGL 3.3 is the selected backend, Vulkan references should be removed from the documentation.

### I2: Stale "Runtime GLSL Loading" Text (MEDIUM)

**Where:** `Hackathon Master Blueprint.md` §4 ("Rendering Engine — Scope")
**vs:** Blueprint §3.3 (Iter 9), Renderer Concept §Resolved Open Decisions

**Problem:** Blueprint §4 says MVP includes "runtime GLSL loading." But §3.3 (Iter 9) explicitly cuts it in favor of precompiled sokol-shdc headers. The Renderer Concept correctly lists no runtime GLSL.

**Impact:** Agents reading §4 might attempt to implement runtime GLSL loading, wasting time on a cut feature. The fixed decisions in §15 item 18 acknowledge the cut, but §4 is the summary scope that agents reference first.

> **Human Comment**: Precompiled glsl is fixed. Precompiled shaders should compile to OpenGL-compatible binary for the machine, not to SPIR-V. There are pitfalls with compiled shaders for old versions of OpenGL, we should thread carefully here.

### I3: Capsule Mesh Dependency Mismatch (MEDIUM)

**Where:** `Game Engine Concept and Milestones.md` E-M1 expected outcome + files listing
**vs:** `Renderer Concept and Milestones.md` R-M5 (Desirable) — "Procedural shape builder: make_capsule_mesh"

**Problem:** The engine's E-M1 MVP milestone references `spawn_capsule(...)` as part of its expected outcome. But the renderer only provides `make_capsule_mesh` in R-M5 (Desirable), not in MVP milestones. This creates a dependency gap — the engine can't deliver E-M1 without something that doesn't exist yet at MVP level.

**Note:** The Renderer Concept's "Resolved Open Decisions" section states: "Capsules: Demoted to Desirable (R-M5)." But the Engine Concept still references capsules in E-M1. This needs resolution.

> **Human Comment**: Capsules are removed from the MVP, can be introduced with a later milestone. Engine doc should be updated.

### I4: Blueprint §6 Game Scope Missing Alpha Blending Mention (LOW)

**Where:** `Hackathon Master Blueprint.md` §6 ("Game — Scope")
**vs:** Renderer Concept R-M3, Game Concept weapons visuals

**Problem:** Blueprint §6 game MVP scope doesn't mention alpha blending or transparency, but the game's laser beam visual ("color-flashing + fading over ~0.5s") and plasma projectiles require it. The Renderer Concept correctly puts basic alpha blending in R-M3 (MVP). This is a minor omission — not a conflict, just incomplete scope documentation.

> **Human Comment**: Should be specified briefly in the blueprint.

---

## 3. Timeline Realism Assessment

### Overall: Tight but Achievable with Parallel Execution

| Workstream | MVP Milestones | Est. Wall-Clock | Headroom |
|---|---|---|---|
| Renderer (R-M0 to R-M3) | 4 milestones | ~3h 15m | ~45m |
| Engine (E-M1 to E-M4) | 4 milestones | ~3h 45m | ~15m |
| Game (G-M1 to G-M4) | 4 milestones | ~3h 45m | ~15m |

**Critical path:** Engine E-M4 (Physics + Collision Response) at ~1h is the hardest milestone. The previous feasibility review correctly flagged this as a risk. If E-M4 slips, G-M2 (physics-dependent) and G-M3 (weapons with physics-based collision damage) are blocked.

**Game dependency chain:** G-M1 → G-M2 → G-M3 → G-M4 is sequential by design. But G-M1 only needs R-M1 + E-M1, and G-M2 needs R-M1 + E-M4. The game workstream can start flight controls at T+0 against mocks and only needs real physics when E-M4 lands. This is well-designed.

### Bottleneck Tasks on Critical Path

| Task | Workstream | Risk Level | Fallback if slipping |
|---|---|---|---|
| R-M2 (uniform block struct) | Renderer | LOW | Define struct with placeholders; refine when shader work catches up |
| E-M4 (collision response) | Engine | **HIGH** | Demote angular dynamics to Desirable; MVP uses linear-only elastic response |
| G-M3 (weapons + enemies) | Game | **HIGH** | Laser only (raycast + line), skip plasma projectiles; 1 enemy with static behavior |

**Fallback triggers:**
- If E-M4 slips past T+3:00 → demote angular velocity/inertia to Desirable. MVP collision response uses linear-only impulse resolution. This removes the most complex math (tensor math, rotational collision) from the MVP.
- If G-M3 slips past T+5:00 → ship laser only, skip plasma projectiles. Plasma requires projectile entity lifecycle + collision detection which adds significant code paths.

### What to Demote to Stretch

The following items are **realistically stretch**, not MVP or Desirable, given the 5–6 hour constraint and the complexity of getting a convincing demo:

| Item | Current Tier | Proposed Tier | Rationale |
|---|---|---|---|
| Asteroid-asteroid elastic collisions | Game Desirable (G-M6) | **Stretch** | Game logic only; engine doesn't need to change. Adds collision detection overhead per frame for 200 asteroids (O(n²)). Demo value is marginal — asteroids already drift and collide with player. |
| Engine E-M5 steering AI swap | Game Desirable (G-M5) | **Stretch** | Game concept already uses game-local seek AI for MVP. Swapping to engine steering is a code refactor with no visible demo improvement at MVP level. |
| Shader-based explosion VFX (R-M5 custom shader hook) | Game Desirable (G-M6) / Renderer R-M5 | **Stretch** | Minimal primitive flash or expanding sphere is sufficient for MVP. Custom shader hook requires renderer R-M5 AND game integration — two milestones of dependency. |
| Blinn-Phong shading (R-M4) | Both: Renderer Desirable, Game Scope §1 (no mention) | **Keep as Desirable** | Does NOT belong in MVP. Lambertian is visually sufficient for asteroids; Blinn-Phong specular is polish for ships. If time runs short, fallback to Lambertian-only (already listed in scope-cut order §13.1). |
| Diffuse textures | Renderer R-M4 | **Keep as Desirable** | Requires asset pipeline + texture upload code. MVP can use solid colors for everything. |
| Multiple enemy ships (3–8) | Game Desirable (G-M5) | **Keep as Desirable** | Single enemy demonstrates the combat loop. Scaling to 3–8 is tuning, not core functionality. |

### What to Add / Strengthen from Stretch Goals

None of the stretch goals should be promoted to MVP. However, one item merits **promotion from "cut" to "MVP safety net":**

| Item | Current Status | Proposed Change | Rationale |
|---|---|---|---|
| First-person camera fallback | Game Concept: "first-person fallback if rig time overruns" | **Promote to MVP contingency plan** — document explicitly in G-M1 notes | Third-person camera rig adds complexity (offset math, lag interpolation, gimbal prevention). If Person C's agent stalls on camera rig, first-person is a one-line change (set camera entity as child of player entity). Make this explicit so agents don't burn time debugging third-person issues. |

### What to Cut Entirely from MVP Scope

These are already cut in the individual concept documents but appear in blueprint scope summaries that agents might reference:

| Item | Where it appears | Action |
|---|---|---|
| Audio | Blueprint §3.2 "Explicit cuts" — correct, no action needed | Already cut ✓ |
| Post-processing | Blueprint §4, §13.1 scope-cut order — correct | Already cut ✓ |
| MSAA | Blueprint §4 — correct | Already cut ✓ |
| Shadow mapping | Blueprint §4 — correct | Already cut ✓ |
| PBR | Blueprint §4 — correct | Already cut ✓ |
| Deferred rendering | Blueprint §4 — correct | Already cut ✓ |
| Networking | Blueprint §3.2, Game Concept — correct | Already cut ✓ |
| Skeletal animation | Blueprint §3.2, Engine Concept — correct | Already cut ✓ |

**No cuts needed** — all hard cuts are consistent across documents.

---

## 4. Actionable Changes for Agent

The following file-by-file changes produce a fully consistent set of planning documents ready for SpecKit execution.

### File 1: `Hackathon Master Blueprint.md`

#### Change 1A: Update §3.3 — Graphics Backend (Vulkan → OpenGL)
**Line ~80–92:** Replace the entire §3.3 "Shader Pipeline and Graphics Backend" section.

**Old (key excerpts):**
```
FIXED: Primary backend: Vulkan on desktop Linux. Fallback: OpenGL 3.3 Core via build flag.
...
Cut: Runtime GLSL loading; shader hot-reload.
```

**New:**
```
FIXED: Only backend: **OpenGL 3.3 Core** on desktop Linux.

FIXED: Shaders are annotated .glsl files under /shaders/renderer/ or /shaders/game/, precompiled ahead of time via sokol-shdc into per-backend headers...
...
Cut: Runtime GLSL loading; shader hot-reload; Vulkan backend.
```

#### Change 1B: Update §4 — Rendering Engine Scope (remove "runtime GLSL loading")
**Line ~149:** Replace:
```
MVP: perspective camera, forward rendering, unlit→Lambertian→Blinn-Phong, diffuse textures, static skybox, line renderer, simple procedural geometry, runtime GLSL loading, minimal shader-error reporting.
```
With:
```
MVP: perspective camera, forward rendering, unlit→Lambertian, static skybox, line renderer (world-space quads), alpha blending (basic), simple procedural geometry, minimal shader-error reporting.
Desirable: Blinn-Phong + diffuse textures, alpha-blended transparency queue (sorted back-to-front), custom shader hook, capsule mesh builder, normal maps.
Stretch/Cut: frustum culling, front-to-back sort, stress test, normal maps, procedural sky shader, shadow mapping, PBR, deferred rendering, post-processing.
```

#### Change 1C: Update §5 — Game Engine Scope (add capsule note)
**Line ~159:** Replace:
```
MVP: ECS via entt, asset/mesh upload bridge, game loop synced with renderer, input, Euler physics, simple collision, raycasting, thin gameplay API.
```
With:
```
MVP: ECS via entt, asset/mesh upload bridge, game loop synced with renderer, input, Euler physics (linear-only collision response), AABB collision, raycasting, thin gameplay API.
Desirable: enemy steering, multiple lights, capsule mesh integration, prefab spawning helpers.
Stretch/Cut: pathfinding, animation, audio, networking, editor, convex-hull colliders, spatial partitioning.
```

#### Change 1D: Update §6 — Game Scope (add alpha blending mention)
**Line ~169:** Replace:
```
MVP: procedural asteroid field, player spaceship, enemy ships, plasma gun, laser beam, shields/HP, containment boundary, Dear ImGui HUD.
```
With:
```
MVP: procedural asteroid field, player spaceship, enemy ships, plasma gun (projectile entity), laser beam (raycast + line quad with alpha fade), shields/HP, boost, containment boundary, alpha-blended VFX, Dear ImGui HUD.
```

#### Change 1E: Update §13.1 — Scope-Cut Order
**Line ~610–617:** Replace the scope-cut order to reflect revised tiers:
```
If time runs short, cut in this order:
1. Blinn-Phong (fallback to Lambertian only)
2. Diffuse textures (solid colors only)
3. Custom shader hook / shader-based explosion VFX
4. Enemy AI steering (game-local seek only; no engine E-M5 swap)
5. Multiple enemy ships (keep 1)
6. Asteroid-asteroid collisions
7. Capsule mesh builder
```

#### Change 1F: Update §15 — Fixed Decisions Cumulative list
**After item 19, add:**
```
20. **(Post-validation)** Graphics backend is OpenGL 3.3 Core only; Vulkan removed. All agents target GL 3.3.
21. **(Post-validation)** Alpha blending (basic) is MVP in renderer R-M3 for laser beam and plasma VFX transparency.
22. **(Post-validation)** Capsule mesh builder is Desirable (R-M5 / E-M5), not MVP. Engine E-M1 uses sphere + cube only.
```

#### Change 1G: Update §14 — Pre-Hackathon Checklist
**Line ~631:** Add to Infrastructure section:
```
- [ ] OpenGL 3.3 Core backend verified on all laptops. **(VERIFY)**
```

**Line ~645:** Replace:
```
- [ ] Copilot, Claude, Gemini, and z.ai workflows verified on real repo structure. **(VERIFY: Copilot CLI availability and Gemini throughput)**
```
With:
```
- [ ] Copilot, Claude, Gemini, and z.ai workflows verified on real repo structure targeting OpenGL 3.3. **(VERIFY: Copilot CLI availability and Gemini throughput)**
```

### File 2: `Game Engine Concept and Milestones.md`

#### Change 2A: E-M1 — Remove capsule reference from MVP
**Line ~56:** Replace:
```
Procedural shape spawners (spawn_sphere, spawn_cube, spawn_capsule) that consume renderer's mesh builders.
```
With:
```
Procedural shape spawners (spawn_sphere, spawn_cube) that consume renderer's mesh builders. Capsule spawner deferred to Desirable (requires renderer R-M5).
```

#### Change 2B: E-M1 Expected Outcome — Remove capsule
**Line ~60:** Replace:
```
`engine_app` shows an ECS-driven procedural scene rendered by the renderer. Game workstream can begin coding against engine API + mocks for M2–M4 pieces.
```
With:
```
`engine_app` shows an ECS-driven procedural scene of spheres and cubes rendered by the renderer. Game workstream can begin coding against engine API + mocks for M2–M4 pieces.
```

#### Change 2C: Milestone Group — Desirable — Add capsule to E-M5 or E-M6
**After E-M6, add:**
```
### E-M7 — Capsule Mesh Integration (Stretch)
- Consume renderer's `make_capsule_mesh` in engine's spawn helpers.
- Demo: `engine_app` renders a capsule alongside spheres and cubes.
```

#### Change 2D: Resolved Open Decisions — Update procedural shapes note
**Line ~146:** After "Pathfinding renamed to steering...", add:
```
- **Capsule mesh:** renderer R-M5 (Desirable), engine E-M7 (Stretch). MVP engine uses sphere + cube only.
```

### File 3: `Game Concept and Milestones.md`

#### Change 3A: G-M1 — Add first-person camera fallback contingency note
**After the Expected outcome paragraph (~line 160), add:**
```
**Contingency:** If third-person camera rig stalls beyond T+1h, switch to first-person (camera entity as child of player entity with no offset). Document decision in TASKS.md and notify Person A. First-person requires ~5 min of code changes; third-person rig can take 30–45 min.
```

#### Change 3B: Milestone Group — Desirable — Demote asteroid-asteroid collisions to Stretch
**Line ~211:** Replace:
```
### G-M6 — Physics & Visual Polish
- Enable asteroid-asteroid collisions.
```
With:
```
### G-M6 — Visual Polish (Asteroid-asteroid collisions moved to Stretch)
- Explosion VFX via renderer R-M5 custom shader hook.
- Laser hit-flash on asteroid surface.
**Note:** Asteroid-asteroid collisions demoted to Stretch due to O(n²) complexity with 200 asteroids and marginal demo value.
```

#### Change 3C: Milestone Group — Stretch — Add asteroid-asteroid collisions
**After the existing stretch items, add:**
```
- **Asteroid-asteroid collisions:** Elastic response via game-layer collision detection (engine provides AABB tests). O(n²) with 200 asteroids; only viable if agent finds a trivial optimization or reduces active asteroid count.
```

#### Change 3D: Scope Summary — Align weapon descriptions
**Line ~23–28:** Ensure consistency with the alpha blending changes. The weapon descriptions are already correct but add a note:
```
- **Minimal explosion VFX** (simple primitive flash or expanding sphere with basic alpha blending) on death. Shader-based VFX deferred to Desirable (requires renderer R-M5).
```

### File 4: `Renderer Concept and Milestones.md`

#### Change 4A: Verify consistency — no changes needed
The Renderer Concept is already consistent with the proposed fixes above. It correctly:
- Uses OpenGL 3.3 Core only
- Has no runtime GLSL loading
- Places capsules in R-M5 (Desirable)
- Includes basic alpha blending in R-M3 (MVP)
- Lists skybox + line quads as MVP

**One minor improvement:** In the "Resolved Open Decisions" section (line ~152), add:
```
- **Graphics API:** OpenGL 3.3 Core only (Vulkan removed for hardware compatibility). This is now fixed across all documents.
```

---

## 5. Final Scope Matrix — Post-Changes

### MVP (must ship within 5–6 hours)

| Workstream | Milestones | Key Deliverables |
|---|---|---|
| **Renderer** | R-M0 bootstrap → R-M1 unlit + API → R-M2 Lambertian → R-M3 skybox + lines + alpha blending | Lit procedural scene, starfield, laser beams, basic transparency |
| **Engine** | E-M1 ECS + scene → E-M2 asset import → E-M3 input + colliders + raycast → E-M4 Euler physics (linear) | Scene management, glTF/OBJ loading, WASD+mouse navigation, collision response |
| **Game** | G-M1 flight + scene → G-M2 physics + containment → G-M3 weapons + enemies + HP/shield → G-M4 HUD + game flow | Playable space shooter: fly, shoot enemies, avoid asteroids, win/lose/restart |

### Desirable (if MVP lands with headroom)

| Workstream | Milestones |
|---|---|
| **Renderer** | R-M4 Blinn-Phong + textures, R-M5 custom shader hook + sorted alpha queue |
| **Engine** | E-M5 steering AI, E-M6 multiple point lights |
| **Game** | G-M5 AI upgrade + 3 enemies, G-M6 visual polish (explosion shader, hit-flash) |

### Stretch (only with significant headroom)

| Workstream | Milestones |
|---|---|
| **Renderer** | R-M6 frustum culling, R-M7 normal maps + procedural sky |
| **Engine** | E-M7 capsule mesh integration, convex-hull colliders, spatial partitioning |
| **Game** | G-M7 feel tuning, asteroid-asteroid collisions, screen-space damage numbers, power-ups, rocket launcher |

---

## 6. Ready for SpecKit

After applying the changes above, the documents will be:
1. **Mutually consistent** on backend (OpenGL), shader pipeline (sokol-shdc precompile), procedural shapes (renderer-owned, engine consumes), capsules (Desirable), and alpha blending (MVP).
2. **Realistic within 5–6 hours** with clear fallback triggers for critical path risks.
3. **Properly tiered** with MVP items that can each be delivered in ~45–60 min per milestone.
4. **Ready for SpecKit execution** — frozen interfaces, milestone unlocks, mock-to-real transitions, and validation levels will be derived from this consistent baseline.
