# Renderer Interface Spec

> **Status:** Draft scaffold. **Not frozen.** Renderer SpecKit will replace this with a versioned freeze before the engine planning cycle starts.

## Intended freeze inputs

- `pre_planning_docs/Hackathon Master Blueprint.md`
- `pre_planning_docs/Renderer Concept and Milestones.md`
- `docs/architecture/renderer-architecture.md`

## Public API surface checklist

- Lifecycle: `init(config)`, `shutdown()`, `run()`
- Per-frame flow: `begin_frame()`, `enqueue_draw(...)`, `enqueue_line_quad(...)`, `end_frame()`
- Meshes: procedural builders and `upload_mesh(...)`
- Materials and textures
- Directional light upload
- Camera application via view/projection matrices
- Skybox setup
- Input callback registration
- Renderer-owned ImGui bridge

## Freeze rules

- This document becomes authoritative only after a versioned freeze marker is added.
- Engine planning must not rely on any signature here until that freeze happens.
- Any later contract change requires explicit human approval and a new version marker.

## Mock handoff note

Renderer SpecKit should define a single engine-consumable public API header shape and the mock surface that will live under `src/renderer/mocks/` for downstream bootstrap work.
