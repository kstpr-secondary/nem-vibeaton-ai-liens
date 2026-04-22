# Renderer Architecture

> **Status:** Pre-SpecKit scaffold. This file is the landing zone for the renderer workstream architecture output and is not frozen yet.

## Inputs

- `pre_planning_docs/Hackathon Master Blueprint.md`
- `pre_planning_docs/Renderer Concept and Milestones.md`
- `docs/interfaces/renderer-interface-spec.md`

## Ownership and boundaries

- Renderer owns `sokol_app` initialization, the main frame callback, and input forwarding.
- Renderer owns Dear ImGui backend wiring through `util/sokol_imgui.h`.
- Renderer does not own a scene graph; the engine enqueues draw work each frame.
- Procedural mesh builders live in the renderer public API and are consumed downstream.

## Topics to freeze during renderer SpecKit

- Frame lifecycle and driver executable responsibilities
- Public API surface and handle types
- Shader compilation flow via `sokol-shdc`
- Pipeline fallback behavior for shader/pipeline creation failures
- Mesh upload path for engine-imported assets
- Input callback bridge contract for the engine

## Open questions

- Exact public header layout for the frozen renderer API
- Mock/header generation convention consumed by the engine workstream
- Naming and file split for pipeline, material, and camera modules
