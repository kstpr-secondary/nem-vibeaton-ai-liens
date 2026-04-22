# MASTER TASKS — Cross-Workstream Synthesis

> Populated by Systems Architect after all three SpecKit runs. This is the human supervisor's integration dashboard.
> Per-workstream TASKS.md files are authoritative on their feature branches.

## Pre-Hackathon Setup Tasks

| ID | Task | Tier | Status | Owner | Depends_on | Notes |
|---|---|---|---|---|---|---|
| SET-1 | Create repo directory scaffold | LOW | IN_PROGRESS | Systems Architect | — | Done: directories created, files to populate below |
| SET-2 | Create top-level CMakeLists.txt with FetchContent + sokol-shdc | HIGH | TODO | Systems Architect / Renderer | SET-1 | BOTTLENECK: everything depends on this |
| SET-3 | Author renderer-interface-spec.md (frozen) | MED | TODO | Renderer Specialist | — | Must be frozen before Engine SpecKit |
| SET-4 | Create cmake/paths.h.in template | LOW | TODO | Systems Architect | SET-2 | ASSET_ROOT path resolution |
| SET-5 | Generate mock implementations for engine→renderer | MED | TODO | Renderer Specialist | SET-3 | src/renderer/mocks/ and src/engine/mocks/ |
| SET-6 | Run Renderer SpecKit → _coordination/renderer/TASKS.md | MED | TODO | Renderer Specialist | SET-2, SET-3 | Produces tasks, architecture, interface freeze |
| SET-7 | Run Engine SpecKit → _coordination/engine/TASKS.md | MED | TODO | Engine Specialist | SET-6 (renderer interfaces frozen) | Uses frozen renderer interfaces |
| SET-8 | Run Game SpecKit → _coordination/game/TASKS.md | MED | TODO | Game Developer | SET-7 (engine interfaces frozen) | Uses frozen renderer + engine interfaces |
| SET-9 | Systems Architect synthesis → MASTER_TASKS.md | MED | TODO | Systems Architect | SET-6, SET-7, SET-8 | Cross-workstream integration plan |

## Hackathon Milestones (T+0 placeholder — populated after SpecKit)

| Workstream | Milestone | Target Time | Status |
|---|---|---|---|
| Renderer | R-M0 Bootstrap | T+0:45 | TODO |
| Renderer | R-M1 Unlit Forward | T+1:45 | TODO |
| Renderer | R-M2 Lambertian | T+2:30 | TODO |
| Renderer | R-M3 Skybox + Alpha | T+3:15 | TODO |
| Engine | E-M0 Bootstrap | T+0:45 | TODO |
| Engine | E-M1 Game Loop | TBD (after R-M1) | TODO |
| Game | G-M0 Bootstrap | T+0:45 | TODO |
