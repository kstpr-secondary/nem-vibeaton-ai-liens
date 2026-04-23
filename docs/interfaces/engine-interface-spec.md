# Engine Interface Spec

> **Status:** Draft — to be populated during Engine SpecKit planning cycle.  
> **Source**: Will be promoted from Engine SpecKit `contracts/` output.  
> **Upstream dependency**: Renderer interface spec (`renderer-interface-spec.md`, FROZEN v1.0).

---

## Version

`Draft — pending Engine SpecKit`

---

## Overview

This document will define the public C++ API surface of the game engine static library (`src/engine/`). The engine provides ECS scene management, asset import, Euler physics, AABB collision, raycasting, input routing, and a thin gameplay API to the game layer.

The engine consumes the renderer public API (frozen) and is consumed by the game executable.

---

## Seed API Surface

> **Not binding.** These are illustrative signatures from `pre_planning_docs/Game Engine Concept and Milestones.md`. The Engine SpecKit will confirm final signatures.

- **Lifecycle:** `init(renderer&, config)`, `shutdown()`, `tick(dt)`
- **Scene:** `create_entity()`, `destroy_entity(e)`, `add_component<T>(e, ...)`, `get_component<T>(e)`, `remove_component<T>(e)`, `view<Components...>()`
- **Asset loading:** `load_gltf(path) → mesh_handle`, `load_obj(path) → mesh_handle`, `spawn_from_asset(path, pos, rot) → entity`
- **Time:** `now()`, `delta_time()`
- **Input:** `key_down(K)`, `mouse_delta()`, `mouse_button(b)` + event callbacks
- **Physics queries:** `raycast(origin, dir, max_dist) → optional<hit>`, `overlap_aabb(center, extents) → span<entity>`
- **Camera:** `set_active_camera(entity)`

---

## Components (tentative)

- Transform, Mesh, Material, RigidBody, Collider, Light, Camera
- Tag markers: Static, Dynamic, Interactable
- Game-layer components (player, weapon, shield) live in game code, not engine

---

## Freeze Procedure

1. Engine SpecKit completes → populates this document with final C++ header and contracts.
2. E-M1 (Bootstrap + ECS + Scene API) implemented and human-reviewed.
3. Human supervisor changes status to `FROZEN — v1.0`, commits, announces to game workstream.
4. Post-freeze changes require explicit human approval + version bump.
