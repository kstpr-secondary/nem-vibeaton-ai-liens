
---

## Slide 1 — AI-LIENS

**We Built a 3D Space Shooter (or Hoomper?)**
**Every Line of C++ Written by AI.**

- 3 engineers. 3 AI workstreams. 4-hour hackathon.
- Renderer · Game Engine · Space Shooter Game
- From blank repo to "working" demo — zero manual code

---

## Slide 2 — Why We Did This

**Three Goals**

- 🔴 **Showcase** — Non-trivial, impossible-before-AI, flashy
- 🔵 **Experiment** — Autonomous SDLC from spec to working impl in a complex specialist project
- 🟢 **Explore** — Parallel workstreams, orchestration, multi-agent sync

---

## Slide 3 — What We Built

**Three Interconnected Systems**

- **Renderer** — Forward rendering, Lambertian shading, skybox, laser line-quads · built on sokol_gfx
- **Game Engine** — ECS, Euler physics, AABB collision, raycast, asset loading · built on entt + cgltf
- **Space Shooter** — Asteroid field, weapons, enemy AI, shields/HP, HUD · Freelancer-inspired

> All three talk to each other via frozen interface contracts

---

## Slide 4 — The Programming Stack

**Low-Level. Minimalist. Intentional.**

- C/C++17 · CMake + Ninja + Clang · no package manager
- **sokol_gfx** — thin OpenGL 3.3 Core wrapper
- **entt** — ECS scene management
- **cgltf / tinyobjloader / stb** — asset loading
- **Dear ImGui** — HUD overlay

> Single-header 20-30k LOC libraries. No hand-holding. Deliberately hard.

---

## Slide 5 — The AI Agent Stack

**9 AI instances across 6 providers**

- 3× Claude Code Pro · 1× Copilot Pro (now discontinued)
- 2× OpenCode GO (Qwen / Kimi / GLM on Chinese infra)
- 3× Gemini Pro (free trial)
- 1× Local Qwen 3.6 35B A3B on RTX 3090

> ~83 EURO AI subscription cost for the full stack

---

## Slide 6 — Conception: 8 Rounds of Research

**Before a single line of code — the blueprint**

- 8 deep research sessions (Perplexity) with heavy human feedback
- Each iteration: clarify architecture, prune bad ideas, fix decisions
- Output: Master Blueprint — platform, stack, API design, agent roles, workflow
- Result: 60–70 hours of pre-hackathon human time

---

## Slide 7 — Skill Engineering

**The secret weapon: distilled library docs**

- sokol_gfx / entt / cgltf are 20–30k LOC single-header libraries
- Agents searching them naively → context death
- Solution: **Domain Skills** — distilled API cheat-sheets + per-aspect reference files
- Roles: Renderer Specialist · Engine Specialist · Game Developer · Systems Architect · Code Reviewer · Spec Validator

---

## Slide 8 — Spec-Driven Planning

**From Blueprint → Frozen Contracts → Tasks**

```
Blueprint → SpecKit (renderer) → Frozen Renderer Interface
                ↓
           SpecKit (engine) → Frozen Engine Interface
                ↓
           SpecKit (game) → Task Lists
```

- Sequential dependency: each spec uses the previous frozen interface
- Renderer / Engine fully spec'd and interface-frozen before hackathon
- ~30 renderer tasks · ~35 engine tasks · ~30 game tasks · all with parallel groups

---

## Slide 9 — The Hackathon Workflow

**Human = Supervisor. Agent = Engineer.**

```
Person A:  "Act as Renderer Specialist, implement Phase 2, tasks R-012..R-015"
Person B:  "Act as Engine Specialist, implement E-M3: input + AABB + raycast"
Person C:  "Act as Game Developer, implement G-M1: flight + scene + camera"
```

- 3 parallel workstreams · sync at milestone boundaries
- Parallel groups: file-disjoint task sets run simultaneously within one workstream
- Frozen interfaces + mocks: game workstream started at T+0, never blocked on upstream

---

## Slide 10 — Milestone Architecture

**~1 merge/hour per workstream**

[DIAGRAM: Sequential dependency chain with parallel task groups inside each milestone]

- R-M0 Bootstrap → R-M1 Unlit → R-M2 Lambertian → R-M3 Skybox+Lines
- E-M1 ECS → E-M2 Assets → E-M3 Input+Collision → E-M4 Physics
- G-M1 Flight → G-M2 Physics → G-M3 Weapons+AI → G-M4 HUD+Flow
- Every milestone: build gate + spec validation + human behavioral check

---

## Slide 11 — What Went Wrong (Hardware)

**Murphy's Law hit hard**

- 🖥️  Target GPU: Metal (Mac) → banned → Vulkan → ancient ITS GPU can't run it → **OpenGL 3.3**, 1 day before
- 💻  Personal laptop: keyboard, WiFi, Bluetooth all died after cooking in a backpack
- 📺  1080p monitors by ITS — IDEs barely usable
- ⏰  30 min late start · behind schedule the whole day

---

## Slide 12 — What Went Wrong (Process)

**The workflow was set in stone but not battle-tested**

- 📋 Two teammates ran SpecKit docs instead of translated task files → some misfire, but engine actually worked well this way
- 🔄 Parallel tasks touched same files → merge chaos → abandoned parallel work
- 🚦 Agent quota hits hard — skybox broke every agent including Claude Opus 4.7
- 🤖 Local RTX 3090 agent didn't work in the office
- 🚀 Enemy AI speed was untweaked → enemies instantly rammed player
- 🪨 Asteroid field extremely sparse — parameters never tested
- 🛸 Ship mesh parsed wrong — rendered the **exhaust plume** instead of the ship

---

## Slide 13 — What Actually Worked

**Something real came out of this**

- ✅ **Actual Lambertian rendering** — geometry lit by a directional light
- ✅ **Fully working physics engine** — hit full MVP milestone
- ✅ **Something was moving in the game** — flight, camera, asteroids
- ✅ Engine: all planned MVP milestones delivered
- ✅ The agent-driven workflow proved it can drive complex specialist code

---

## Slide 14 — After the Hackathon

**~20 more hours, mostly unattended**

- Local Qwen 3.6 35B on RTX 3090 — surprisingly capable
- Implemented lagged tasks + some new ones
- Game now has: functioning combat loop, HUD, asteroid field, enemy AI

> "Reading a book while the local agent implements tasks"

---

## Slide 15 — What's Next

**Level 4 Autonomous SDLC**

- 🧹 Unlegacy and greenify the codebase
- 🤖 Semi-autonomous workflow — human only in the loop for:
  - Writing specs
  - Validating artifacts
  - Visual confirmation at checkpoints
- 📐 New feature-based workflow already in progress
- The experiment continues — and it's working

---
