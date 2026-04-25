# Data Model: Space Shooter MVP

**Date**: 2026-04-23
**Feature**: [spec.md](./spec.md)
**Status**: Complete

## Overview

The game layer defines its own ECS components (tags and state structs) that are attached to entities in the engine-owned `entt::registry`. Engine-owned components (`Transform`, `RigidBody`, `Collider`, `Mesh`, `EntityMaterial`, `Camera`, `Light`) coexist on the same entities. The game never creates a second registry.

All game state is either (a) stored in ECS components on entities, or (b) in a small number of singleton structs managed by the game lifecycle (MatchState, field config). No disk persistence. No networking.

---

## Game-Layer ECS Components

### Tag Components (empty structs — presence is semantic)

| Component | Attached to | Purpose |
|-----------|------------|---------|
| `PlayerTag` | Player ship entity | Identifies the player for system queries |
| `EnemyTag` | Enemy ship entities | Identifies enemies for AI, win condition, targeting |
| `AsteroidTag` | Asteroid entities | Identifies asteroids for containment, collision damage |
| `ProjectileTag` | Plasma projectile entities | Identifies active projectiles for lifetime + collision |

### State Components

#### `Health`

| Field | Type | Default | Validation |
|-------|------|---------|------------|
| `current` | float | 100.0 | Clamped [0, max] |
| `max` | float | 100.0 | > 0 |

- Attached to: player, enemies
- When `current` reaches 0 → death trigger
- Damage applied after shield is depleted

#### `Shield`

| Field | Type | Default | Validation |
|-------|------|---------|------------|
| `current` | float | 100.0 | Clamped [0, max] |
| `max` | float | 100.0 | > 0 |
| `regen_rate` | float | 2.0 | ≥ 0 (units/sec) |
| `regen_delay` | float | 3.0 | ≥ 0 (seconds after last damage) |
| `last_damage_time` | double | 0.0 | Set on each damage event |

- Attached to: player, enemies
- Absorbs damage before health
- Regenerates passively after `regen_delay` seconds of no damage

#### `Boost`

| Field | Type | Default | Validation |
|-------|------|---------|------------|
| `current` | float | 100.0 | Clamped [0, max] |
| `max` | float | 100.0 | > 0 |
| `drain_rate` | float | 20.0 | > 0 (units/sec — drains in ~5s) |
| `regen_rate` | float | 5.0 | > 0 (units/sec — regens in ~20s) |
| `active` | bool | false | Set by input |

- Attached to: player only
- While `active` and `current > 0`: doubles forward thrust, drains at `drain_rate`
- While not `active`: regenerates at `regen_rate`

#### `WeaponState`

| Field | Type | Default | Validation |
|-------|------|---------|------------|
| `active_weapon` | enum WeaponType | Laser | {Laser, Plasma} |
| `laser_cooldown` | float | 5.0 | > 0 (seconds) |
| `laser_last_fire` | double | -∞ | Timestamp |
| `laser_damage` | float | 10.0 | > 0 |
| `plasma_cooldown` | float | 0.1 | > 0 (seconds) |
| `plasma_last_fire` | double | -∞ | Timestamp |
| `plasma_damage` | float | 0.5 | > 0 |
| `plasma_speed` | float | 200.0 | > 0 (units/sec) |
| `plasma_lifetime` | float | 3.0 | > 0 (seconds) |

- Attached to: player, enemies (enemies use plasma only)
- Tracks cooldown per weapon type independently
- `active_weapon` switches instantly on Q/E input

#### `ProjectileData`

| Field | Type | Default | Validation |
|-------|------|---------|------------|
| `owner` | entt::entity | required | Must be valid at spawn |
| `damage` | float | 0.5 | > 0 |
| `spawn_time` | double | required | Set at creation |
| `lifetime` | float | 3.0 | > 0 (seconds) |

- Attached to: plasma projectile entities (alongside `ProjectileTag`)
- Despawned when `engine_now() - spawn_time > lifetime` or on first collision

#### `EnemyAI`

| Field | Type | Default | Validation |
|-------|------|---------|------------|
| `pursuit_speed` | float | 30.0 | > 0 (units/sec) |
| `fire_range` | float | 300.0 | > 0 (units) |
| `fire_cooldown` | float | 0.5 | > 0 (seconds) |
| `last_fire_time` | double | -∞ | Timestamp |

- Attached to: enemy entities
- AI logic: steer toward player position, fire plasma when within `fire_range` + line-of-sight

#### `AsteroidData`

| Field | Type | Default | Validation |
|-------|------|---------|------------|
| `size_tier` | enum SizeTier | required | {Small, Medium, Large} |

- Attached to: asteroid entities (alongside `AsteroidTag`)
- Size tier determines mesh scale, mass assignment (via engine RigidBody.mass), and initial velocity range
- No additional runtime state — physics handled by engine RigidBody

---

## Singleton Game State (not ECS components)

### `MatchState`

| Field | Type | Default | Validation |
|-------|------|---------|------------|
| `phase` | enum MatchPhase | Playing | {Playing, PlayerDead, Victory, Restarting} |
| `phase_enter_time` | double | 0.0 | Set on phase transition |
| `auto_restart_delay` | float | 2.0 (death) / 3.0 (win) | > 0 seconds |
| `enemies_remaining` | int | 1 | ≥ 0 |

- State transitions:
  - Playing → PlayerDead: player Health.current reaches 0
  - Playing → Victory: enemies_remaining reaches 0 (checked before player death)
  - PlayerDead → Restarting: auto after `auto_restart_delay` or manual Enter
  - Victory → Restarting: auto after `auto_restart_delay` or manual Enter
  - Restarting → Playing: scene rebuilt, entities respawned

### `FieldConfig`

| Field | Type | Default | Validation |
|-------|------|---------|------------|
| `radius` | float | 1000.0 | > 0 (units) |
| `asteroid_count` | int | 200 | > 0 |
| `max_asteroid_speed` | float | 50.0 | > 0 (units/sec, post-reflection cap) |

- Read-only after init; drives procedural asteroid placement and containment logic

---

## Entity Archetypes

These are the typical component compositions for each entity type. All entities are created via `engine_create_entity()` and populated with engine + game components.

| Archetype | Engine Components | Game Components |
|-----------|------------------|-----------------|
| Player Ship | Transform, Mesh, EntityMaterial, RigidBody, Collider, Camera (child entity) | PlayerTag, Health, Shield, Boost, WeaponState |
| Enemy Ship | Transform, Mesh, EntityMaterial, RigidBody, Collider | EnemyTag, Health, Shield, WeaponState, EnemyAI |
| Asteroid | Transform, Mesh, EntityMaterial, RigidBody, Collider | AsteroidTag, AsteroidData |
| Plasma Projectile | Transform, Mesh, EntityMaterial, RigidBody, Collider | ProjectileTag, ProjectileData |
| Camera | Transform, Camera, CameraActive | (none — managed by camera_rig system) |
| Directional Light | Light | (none — set once at init) |

---

## Relationships

```
MatchState (singleton)
├── tracks enemies_remaining (count of entities with EnemyTag + Health.current > 0)
├── owns phase transitions
└── triggers scene rebuild on Restarting→Playing

Player Ship (1 per match)
├── has camera rig (separate Camera entity follows player Transform)
├── fires weapons → spawns ProjectileData entities (Plasma) or raycasts (Laser)
└── takes damage from: asteroid collisions, enemy projectiles

Enemy Ship (1 per match MVP, 3-8 desirable)
├── AI targets player ship
├── fires plasma projectiles toward player
└── takes damage from: player weapons (laser raycast, plasma projectile)

Asteroid (200 per match)
├── contained by FieldConfig.radius boundary
├── collides with: player, enemies, other asteroids (desirable), projectiles
└── takes impulse from: laser hits, plasma hits

Plasma Projectile (0-50 active)
├── owned by spawning entity (player or enemy)
├── despawns on: collision or lifetime expiry
└── deals damage to: first entity collided with (if has Health)
```
