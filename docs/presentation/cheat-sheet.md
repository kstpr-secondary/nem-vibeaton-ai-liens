# Hackathon Presentation — Cheat Sheet
> Extended Q&A reference for the presenter. More detail than slides.

---

## What Was Built

Three interconnected C++ systems developed from scratch:

1. **Rendering Engine** (`src/renderer/`) — forward renderer using `sokol_gfx` (sokol is a thin C wrapper over graphics APIs). OpenGL 3.3 Core backend. Features: perspective camera, unlit pipeline, Lambertian (diffuse) shading, skybox (cubemap), world-space line-quads (lasers), alpha blending, Dear ImGui integration. Shaders written in annotated GLSL and pre-compiled by `sokol-shdc` at build time.

2. **Game Engine** (`src/engine/`) — ECS-based engine using `entt`. Features: entity-component-system scene management, glTF/OBJ asset loading (`cgltf`/`tinyobjloader`), input handling, Euler rigid-body physics (linear + angular), AABB collision detection and impulse response, raycast/overlap queries, camera matrix computation.

3. **Space Shooter Game** (`src/game/`) — Freelancer-inspired third-person 3D space shooter. Player ship, 200-asteroid procedural field, enemy AI, two weapons (laser raycast + plasma projectile), HP/shield/boost resources, Dear ImGui HUD, win/death/restart flow.

---

## Team and Roles

- **Person A** — Rendering engine workstream
- **Person B** — Game engine workstream  
- **Person C** — Game workstream
- RTX 3090 desktop — shared validation, tests, local AI model, demo machine

---

## Programming Stack Detail

| Component | Library/Tool | Why |
|---|---|---|
| Language | C/C++17 | Fast to implement, low overhead |
| Build | CMake + Ninja + Clang | Standard, reproducible |
| Graphics | sokol_gfx + sokol_app | Cross-platform thin wrapper, OpenGL 3.3 backend |
| ECS | entt v3 | Lightweight, idiomatic, fast iteration |
| Asset loading | cgltf + tinyobjloader | glTF primary, OBJ fallback |
| Math | GLM | Standard 3D math |
| UI | Dear ImGui | Immediate-mode, renderer-owned via sokol_imgui.h |
| Testing | Catch2 v3 | Unit tests for math/ECS/physics |
| Shaders | sokol-shdc | Compile-time GLSL → header |
| Images | stb_image | Single-header image loader |

All dependencies via **CMake FetchContent** only — no package manager.

**Key design decisions:**
- No audio, networking, skeletal animation, particles, or post-processing — hard cuts from the start
- `sokol_app` owned by renderer; game engine ticks from inside the renderer frame callback
- Input flows: sokol_app → renderer → engine → game
- Asset paths composed from compile-time `ASSET_ROOT` macro — no cwd dependency

---

## AI Agent Stack Detail

| Agent | Tier | Role |
|---|---|---|
| Claude Code Sonnet (3×) | Primary | Hard/ambiguous impl tasks, architecture |
| GitHub Copilot Pro (1×) | Primary backup | Medium tasks, standard impl |
| OpenCode GO (2×, Qwen/Kimi/GLM) | Secondary | Parallel hard tasks, second half |
| Gemini Pro (3×) | Secondary | Validation, review, research, overflow |
| Local Qwen 3.6 35B A3B @RTX 3090 | Bounded/local | Tests, spec checks, shader review, small diffs |

**Escalation rule:** agent stall >90 sec → switch immediately, log blocker.
**Emergency path:** agent fails twice or >30 min behind → human takes over.

**Total cost:** ~$35/month in AI subscriptions for full stack.

---

## Pre-Hackathon Process (60-70 human hours)

### Phase 1: Conception — 8 Research Rounds
- Tool: Perplexity deep research with heavy human feedback loops
- Each round: propose architecture, argue tradeoffs, prune bad ideas, fix decisions
- Topics covered: graphics API choice, ECS vs object hierarchy, physics scope, shader pipeline, agent tooling, workflow structure
- Output: **Master Blueprint** — platform, stack, API design, workflow rules, agent roles

**Why 8 rounds?** Each round surfaced new problems and forced decisions. By round 8, all major unknowns were resolved and documented.

### Phase 2: Skill Engineering
**The problem:** sokol_gfx, entt, cgltf are 20-30k LOC single-header libraries. An agent searching them naively at runtime burns the entire context budget.

**Solution — Domain Skills:**
- Each library got a `SKILL.md` — a distilled API cheat-sheet covering only what the project needs
- Per-aspect reference files: e.g., `sokol-api-render-passes.md`, `sokol-api-draw-calls.md`
- Role skills: Renderer Specialist, Engine Specialist, Game Developer, Systems Architect, Code Reviewer, Spec Validator, Test Author, Plan Groomer
- Agents load role skill + relevant domain skill only → context preserved for actual work

### Phase 3: Spec-Driven Planning with SpecKit

Sequential pipeline (each spec uses frozen previous interfaces):
```
Renderer SpecKit → renderer-interface-spec.md (FROZEN v1.1)
        ↓
Engine SpecKit → engine-interface-spec.md (FROZEN v1.2)  
        ↓
Game SpecKit → game tasks
        ↓
Systems Architect synthesis → MASTER_TASKS.md
```

Each SpecKit run produced:
- Architecture document
- Interface spec (frozen before next workstream)
- `TASKS.md` with full schema: ID, Task, Tier, Status, Owner, Depends_on, Milestone, Parallel_Group, Validation, Notes

**Parallel groups:** Tasks within a milestone with disjoint file sets can run concurrently — explicitly declared in task notes. A `BOTTLENECK` marker blocks all others until a shared type/interface is defined.

---

## Hackathon Day Workflow Detail

### Human Role
Pure supervisor + orchestrator:
```
"Act as a Rendering Specialist, implement Phase 2, tasks R-012-R-015 from @TASKS.md"
```
Then observe, unblock, merge at milestones, manage git sync.

### Workstream Independence via Mocks
- At T+0, all three workstreams started simultaneously
- Game workstream worked against **mocks** (stub implementations of renderer + engine APIs)
- When a real milestone merged, the relevant mock was swapped out
- No workstream ever blocked waiting for another

### Milestone Protocol
Each milestone required:
1. All tasks marked DONE
2. Build passes (cmake --build)
3. Relevant tests pass
4. Spec Validator confirms acceptance criteria
5. Human behavioral check (visual confirmation)
6. Lightweight code review
7. Merge + downstream sync

### Parallel Work Rules
- Parallel groups: tasks explicitly declare file ownership
- If an agent touches a file outside its declared set → pause, update task, check for conflict
- Conflicts → BLOCKERS.md + wait for human resolution
- **This rule was violated during the hackathon** → merge chaos → abandoned parallel groups

### Milestone Target Cadence
~1 milestone merge per hour per workstream (~4-5 total per workstream in 5-6 hours).

---

## Pre-Hackathon Milestones Completed

**Renderer** — ALL 4 MVP milestones done pre-hackathon:
- R-M0 Bootstrap (window, clear color, input)
- R-M1 Unlit forward rendering + procedural scene
- R-M2 Directional light + Lambertian shading
- R-M3 Skybox + line-quads + alpha blending → **Renderer MVP COMPLETE**

**Engine** — ALL 4 MVP milestones done pre-hackathon:
- E-M1 Bootstrap + ECS + scene API
- E-M2 Asset import (glTF + OBJ)
- E-M3 Input + AABB colliders + raycast
- E-M4 Euler physics + collision response → **Engine MVP COMPLETE**

**Game** — Starting from scratch at hackathon T+0 (spec not ready in time)

---

## What Went Wrong — Full Account

### Hardware (Murphy's Law)
| Problem | Impact |
|---|---|
| Target: Metal (macOS) → IT banned Macs | Pivoted to Vulkan |
| Person B got PC with GPU too old for Vulkan | Pivoted to OpenGL 3.3 — 1 day before hackathon |
| Person A's laptop: keyboard + WiFi + Bluetooth all died after overheating in backpack | Worked around by borrowing peripherals |
| Venue monitors: 1080p — IDEs barely usable | Reduced productivity all day |
| 30 min late start due to setup | Behind schedule from the start |

### Process
| Problem | Root Cause | Effect |
|---|---|---|
| Two teammates ran SpecKit docs, not translated TASKS.md | Docs weren't clear enough for hand-off | Mixed results — engine actually worked well this way |
| Parallel tasks touched same files | Parallel groups not enforced strictly enough | Merge chaos; abandoned all parallel work |
| Agent quota exhaustion | Skybox impl is hard; all agents burned tokens in vain including Claude Opus 4.7 | Skybox partially broken for long time |
| Local RTX 3090 agent didn't work | Office network / environment issues | Lost the local overflow channel |

### Game-Specific
| Problem | Symptom |
|---|---|
| Enemy AI speed untweaked | Enemies instantly rammed player and died |
| Asteroid parameters untested | Field extremely sparse, didn't look like asteroid field |
| Ship glTF mesh parsed incorrectly | Rendered exhaust plume instead of ship |
| No parameter tuning pass | Nothing felt right |

---

## What Actually Worked

- **Lambertian rendering** — real 3D geometry lit by a directional light, visible on screen
- **Physics engine** — full MVP milestone achieved; elastic collisions, impulse response, all working
- **Game had movement** — flight controls, camera follow, asteroids visible (if sparse)
- **Renderer + Engine architectural pipeline** — the spec → frozen interface → parallel impl → mock-swap workflow proved viable
- **Agent-driven complex specialist code** — agents navigated low-level sokol, entt, cgltf without manual assistance
- **Blueprint as coordination mechanism** — a single authoritative doc kept 3 workstreams aligned

---

## After Hackathon (~20 hours post)

- Primary agent: local Qwen 3.6 35B A3B on RTX 3090 — "surprisingly capable"
- Mode: largely unattended — ran tasks overnight and between sessions
- Work done: implemented lagged tasks, plugged in combat loop, HUD, fixed asset loading, tweaked parameters

---

## Architecture Diagram Summary

```
sokol_app (window/input)
       ↓ owned by renderer
  Renderer (sokol_gfx, OpenGL 3.3)
  - forward pass (unlit / Lambertian)
  - skybox pass
  - line-quad pass (lasers)
  - ImGui pass (HUD)
       ↓ frame callback (renderer calls engine tick)
  Engine (entt ECS)
  - scene management
  - asset loading (cgltf/tinyobjloader)
  - Euler physics + AABB collision
  - input polling
  - camera matrix → pushed to renderer
       ↓ game API
  Game (space shooter logic)
  - player controller
  - asteroid field
  - weapons (laser raycast, plasma projectile)
  - enemy AI
  - HP/shield/boost
  - HUD widgets (emitted to ImGui)
```

---

## Frozen Interfaces

`docs/interfaces/renderer-interface-spec.md` — v1.1 (FROZEN)
- Key: `renderer_set_frame_callback(cb, user_data)` — how engine hooks into the render loop

`docs/interfaces/engine-interface-spec.md` — v1.2 (FROZEN)
- Key: ECS scene API, physics queries, input polled state

**Rule:** no cross-workstream code compiles against anything not in the frozen interface. Interface changes require explicit human approval.

---

## Key Q&A Topics

**Q: Why not use Unity/Unreal?**
A: That's the point. Trivially possible before AI. We wanted to prove agents can handle low-level specialist work that would normally require deep domain expertise.

**Q: How many lines of code?**
A: Not measured precisely — the focus was on delivered behavior, not LOC.

**Q: What's the quality of the code?**
A: Speed over maintainability was an explicit design decision. Working code with good ole' C-style pointers. Phase 2 (post-hackathon) involves cleanup and establishing sustainable patterns.

**Q: Could one person have done this alone?**
A: Pre-hackathon conceptual + scaffold work: yes, but 60-70h. Hackathon day execution of 3 simultaneous workstreams: no — not in 5-6 hours without AI.

**Q: What was the hardest technical part?**
A: Skybox rendering. Every agent including Claude Opus 4.7 failed repeatedly. The GPU API interaction between sokol's cubemap loading, the skybox shader, and the render pass ordering proved very stubborn.

**Q: What's Level 4 ASDLC?**
A: Autonomous Software Development Life Cycle — Level 4 meaning human only in the loop for: writing specs, validating artifacts, and visual checkpoint confirmation during feature development. Everything else (planning, task breakdown, implementation, testing, review) runs autonomously.

**Q: Why OpenGL 3.3 specifically?**
A: Compatibility crisis. Original plan: Metal on macOS. IT banned Macs → Vulkan on Linux. Then one team member got a machine with a GPU too old for Vulkan. OpenGL 3.3 runs on virtually anything. Decided 1 day before hackathon. The sokol abstraction layer made the switch relatively painless — it's just a backend flag.

**Q: What is sokol_gfx?**
A: A thin C header-only wrapper by Andre Weissflog that provides a unified API over OpenGL, Vulkan, Metal, D3D11/12, and WebGPU. We use it as an OpenGL 3.3 abstraction — it gives us `sg_buffer`, `sg_pipeline`, `sg_pass` etc. without writing raw GL calls everywhere. sokol-shdc compiles annotated GLSL into backend-specific headers at build time.

**Q: What is entt?**
A: Entity Component System library. Instead of class hierarchies, you have entities (just IDs), components (plain data structs attached to entities), and systems (code that iterates views of component combinations). entt is header-only, very fast, and widely used in game engines.

**Q: How did the mock system work?**
A: CMake `USE_RENDERER_MOCKS` / `USE_ENGINE_MOCKS` flags. When ON, CMake compiles stub implementations that return empty/default values but satisfy the frozen interface. Game workstream starts at T+0 with mocks. When a real upstream milestone merges, human flips the flag, verifies build, done.

---

## Numbers

| Metric | Value |
|---|---|
| Pre-hackathon human time | ~60-70 hours |
| AI research iterations (Perplexity) | 8 rounds |
| Renderer MVP milestones | 4 (all done pre-hackathon) |
| Engine MVP milestones | 4 (all done pre-hackathon) |
| Game MVP milestones | 4 (started at hackathon T+0) |
| Planned game tasks | ~30 |
| AI tools active | 9 instances across 6 providers |
| Monthly AI subscription cost | ~$35 |
| Post-hackathon work | ~20 hours |
| Lines of manually written C++ | 0 |

---
