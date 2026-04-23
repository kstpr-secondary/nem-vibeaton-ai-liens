# Engine SpecKit — Quick Start

> Planning scaffold for the game engine workstream SpecKit cycle.

---

## Seed Document

`pre_planning_docs/Game Engine Concept and Milestones.md`

## Upstream Dependencies (frozen)

- Renderer interface: `docs/interfaces/renderer-interface-spec.md` — **FROZEN v1.0**
- Renderer architecture: `docs/architecture/renderer-architecture.md` — **FROZEN**

## SpecKit Outputs (will be created here)

- `specs/002-game-engine/spec.md` — feature specification
- `specs/002-game-engine/plan.md` — implementation plan
- `specs/002-game-engine/research.md` — technology research
- `specs/002-game-engine/tasks.md` — task decomposition
- `specs/002-game-engine/contracts/` — API contracts

## Post-SpecKit Promotion

- `docs/interfaces/engine-interface-spec.md` ← promoted from contracts
- `docs/architecture/engine-architecture.md` ← promoted from plan
- `_coordination/engine/TASKS.md` ← translated from SpecKit tasks to blueprint schema

## Relevant Skills

- `.agents/skills/engine-specialist/SKILL.md`
- `.agents/skills/entt-ecs/SKILL.md`
- `.agents/skills/cgltf-loading/SKILL.md` + `references/`
- `.agents/skills/physics-euler/SKILL.md` + `references/`
- `.agents/skills/sokol-api/SKILL.md` + `references/`

## Key Constraints (from blueprint)

- Engine ticks from inside renderer frame callback — no independent main loop
- All asset paths via `ASSET_ROOT` macro — never hard-code
- Renderer handles are opaque uint32_t — engine must not inspect internals
- `USE_RENDERER_MOCKS=ON` allows engine to compile without renderer implementation
- C++17, no exceptions, no RTTI
