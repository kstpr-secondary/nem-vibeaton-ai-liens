# Game Interface Spec

> **Status:** Placeholder — to be populated during Game SpecKit planning cycle.  
> **Source**: Will be promoted from Game SpecKit `contracts/` output.  
> **Upstream dependencies**: Renderer interface spec (`renderer-interface-spec.md`, FROZEN v1.1). Engine interface spec (`engine-interface-spec.md`, DRAFT v0.1 — must be frozen before Game SpecKit).

---

## Version

`Placeholder — pending Game SpecKit`

---

## Overview

This document will define the public API surface of the game executable (`src/game/`). The game provides a 3D space shooter built on the engine and renderer public APIs.

The game consumes the engine public API (to be frozen) and the renderer public API (frozen v1.1). It is the final consumer in the dependency chain.

---

## Seed API Surface

> **Not binding.** These are illustrative signatures from `pre_planning_docs/Game Concept and Milestones.md`. The Game SpecKit will confirm final signatures.

- **Lifecycle:** `game_init()`, `game_tick(dt)`, `game_shutdown()`
- **Integration**: Game tick called from inside renderer's FrameCallback, sandwiched between `renderer_begin_frame()` / `renderer_end_frame()`. Engine tick called within game tick.
- **HUD**: Uses renderer-owned Dear ImGui path; no second ImGui context.

---

## Freeze Procedure

1. Game SpecKit completes → populates this document with final contracts.
2. G-M1 (Flight Controller + Scene + Camera) implemented and human-reviewed.
3. Human supervisor changes status to `FROZEN — v1.0`, commits, announces.
4. Post-freeze changes require explicit human approval + version bump.
