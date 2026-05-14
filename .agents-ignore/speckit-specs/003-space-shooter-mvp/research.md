# Research: Space Shooter MVP

**Date**: 2026-04-23
**Feature**: [spec.md](./spec.md)
**Status**: Complete — no NEEDS CLARIFICATION items required

## Overview

All technical decisions for the game workstream are fully constrained by the constitution, the frozen upstream interfaces (renderer v1.1, engine v1.1), and the game design document. No external research was needed. This document records key design decisions and their rationale for traceability.

---

## R1: Match State Machine Design

- **Decision**: 4-state machine — Playing → PlayerDead / Victory → Restarting → Playing
- **Rationale**: Terminal states (PlayerDead, Victory) need a visible overlay and auto-countdown before scene teardown. Separating Restarting from terminal display lets entity cleanup happen in a distinct phase, preventing use-after-destroy issues when the HUD reads entity data during overlay.
- **Alternatives considered**:
  - 3-state (no Restarting phase): Rejected — conflates overlay display with entity cleanup, risks reading destroyed components during the terminal overlay frame.
  - 5-state (add Initializing): Rejected — adds complexity for a loading screen the MVP doesn't need. Game init is fast enough to be imperceptible.

## R2: Simultaneous Death Resolution

- **Decision**: Player wins if last enemy and player both reach 0 HP in the same tick.
- **Rationale**: The win condition is "enemy count == 0". If the objective is satisfied, the player succeeded. This rewards aggressive play and makes the demo more exciting. Enemy deaths are evaluated before player death within each tick.
- **Alternatives considered**:
  - Player loses (player death evaluated first): Rejected — punishes the player for completing the objective; feels unfair.
  - Draw / mutual destruction: Rejected — requires a fifth match state and unique overlay; adds scope for a rare edge case.

## R3: Collision Damage Model

- **Decision**: No minimum threshold. Every collision deals damage proportional to relative kinetic energy.
- **Rationale**: Simplest to implement — no threshold parameter to tune. Slow glances deal sub-1 damage (effectively negligible due to shield regen), while hard rams deal significant damage. Avoids the need for a "magic number" cutoff that might feel wrong in playtesting.
- **Alternatives considered**:
  - Energy threshold: Rejected — requires tuning a threshold constant; adds code complexity; slow collisions still feel like contact events.
  - Speed threshold: Rejected — decouples damage from mass (a slow massive asteroid should still hurt); less physically intuitive.

## R4: Camera Strategy

- **Decision**: Third-person camera rig as default; first-person fallback if rig exceeds time budget.
- **Rationale**: Third-person provides better spatial awareness for asteroid avoidance and enemy tracking. The game concept explicitly names this as the preferred experience. First-person is a trivial fallback (camera entity at player position, no offset) that preserves milestone predictability.
- **Alternatives considered**:
  - First-person only: Rejected — worse spatial awareness; reduces demo visual appeal.
  - Orbital camera: Rejected — more complex to implement; free orbit doesn't match Freelancer-style flight.

## R5: Enemy AI Strategy (MVP)

- **Decision**: Game-local seek + shoot. No engine E-M5 steering dependency.
- **Rationale**: The seek vector is `normalize(player_pos - enemy_pos) * speed`. Line-of-sight check uses `engine_raycast`. Fire decision is range + LOS + cooldown. This is ~20 lines of code, has zero upstream dependency, and ships with G-M3. Engine steering (E-M5) is desirable scope.
- **Alternatives considered**:
  - Wait for engine E-M5 steering: Rejected — blocks G-M3 on a desirable-tier engine milestone; violates "game workstream never idles on upstream delivery".
  - Behavior tree: Rejected — over-engineered for 1 enemy with 2 behaviors (seek, shoot); violates Principle III.

## R6: Weapon Integration Patterns

- **Decision**: Laser uses `engine_raycast` for instant hit; plasma spawns a projectile entity with RigidBody + Collider.
- **Rationale**: Maps directly to engine API capabilities. Raycast is the natural primitive for hitscan weapons. Projectile entities leverage existing ECS physics pipeline (E-M4) for movement and collision detection. No custom physics needed.
- **Alternatives considered**:
  - Both weapons as projectiles (laser = very fast projectile): Rejected — loses the distinct weapon feel; laser should be instant-hit for gameplay contrast.
  - Both weapons as raycasts (plasma = rapid short-range rays): Rejected — loses the visual appeal of flying projectiles; doesn't test the physics pipeline.

## R7: HUD Integration

- **Decision**: Game builds ImGui widgets; renderer owns ImGui lifecycle.
- **Rationale**: The renderer already owns `simgui_setup`, `simgui_new_frame`, event forwarding, and `simgui_render`. Game code only calls `ImGui::Begin/End/Text/ProgressBar` etc. inside the frame callback, between `renderer_begin_frame()` and `renderer_end_frame()`. No second ImGui context needed.
- **Alternatives considered**:
  - Game owns ImGui setup: Rejected — violates renderer ownership of sokol_app/sokol_imgui; would require game to depend on sokol internals.
  - Custom HUD rendering (no ImGui): Rejected — requires texture atlas, font rendering, quad batching; massive scope for a hackathon HUD.

## R8: Containment Implementation

- **Decision**: Game-layer constraint. Spherical boundary at configurable radius (~1000 units). Reflection = invert velocity component along inward normal; clamp asteroid speed post-reflection.
- **Rationale**: This is game logic, not engine physics. The engine provides RigidBody velocity access; the game reads position, checks distance from origin, and modifies velocity if outside bounds. Speed capping prevents runaway energy growth from repeated reflections.
- **Alternatives considered**:
  - Engine-level boundary collider: Rejected — engine only supports AABB colliders; spherical boundary would require engine API extension (violates frozen interface).
  - No speed cap: Rejected — repeated reflections can amplify speed due to floating-point accumulation; explicit cap is a one-line safety net.
