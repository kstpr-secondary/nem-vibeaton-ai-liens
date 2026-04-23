# Feature Specification: ECS Physics Engine

**Feature Branch**: `002-ecs-physics-engine`  
**Created**: 2026-04-23  
**Status**: Draft  
**Input**: User description: "A game engine that allows building a procedural scene via a glTF-fed asset pipeline, composing the scene as ECS entities, and running a physics simulation of N bodies moving and colliding in zero gravity via Eulerian physics and elastic collisions. The engine exposes an API for a game to build interaction upon the simulation."

## User Scenarios & Testing *(mandatory)*

### User Story 1 — Build a Procedural ECS Scene from Code (Priority: P1)

A game developer initializes the engine, creates entities through the public scene API, attaches transform, mesh, material, rigid-body, and collider components, and sees those entities rendered on screen through the renderer. The developer can spawn procedural primitives (spheres, cubes) and position them freely in 3D space.

**Why this priority**: The ECS scene is the foundation — every other feature (physics, assets, queries) builds on the ability to create and compose entities with components. Without it, nothing else is demonstrable.

**Independent Test**: Can be fully tested by spawning a handful of procedural primitives via the scene API and verifying they appear rendered on screen with correct positions and orientations.

**Acceptance Scenarios**:

1. **Given** the engine is initialized with a renderer reference, **When** the game creates an entity and attaches Transform + Mesh + Material components, **Then** the entity is rendered at the specified position and orientation each frame.
2. **Given** the engine is running, **When** the game calls `create_entity()` and later `destroy_entity()`, **Then** the entity appears and disappears from the rendered scene without errors.
3. **Given** the engine tick is called each frame, **When** multiple entities with Transform + Mesh + Material exist, **Then** all entities are enqueued for rendering and displayed correctly.
4. **Given** the engine is initialized, **When** the game uses procedural shape helpers (`spawn_sphere`, `spawn_cube`), **Then** correctly shaped primitives appear at the requested positions with the requested materials.

---

### User Story 2 — Load and Display glTF/OBJ Assets (Priority: P2)

A game developer loads a 3D model from a glTF or OBJ file on disk and spawns it into the scene as a fully composed entity. The loaded asset appears rendered alongside procedural primitives. The developer can load multiple distinct assets and position them independently.

**Why this priority**: Real game content comes from authored 3D models. The asset pipeline bridges authored content and the ECS scene, enabling the game to populate a visually rich asteroid field.

**Independent Test**: Can be tested by loading a known glTF file from the assets directory and verifying the mesh renders at the specified position with correct geometry.

**Acceptance Scenarios**:

1. **Given** a valid glTF file exists in the assets directory, **When** the game calls the asset-load function with its path, **Then** a mesh handle is returned and the mesh can be attached to an entity for rendering.
2. **Given** a valid OBJ file exists in the assets directory, **When** the game calls the OBJ-load function with its path, **Then** the mesh is loaded and renderable as a fallback format.
3. **Given** an asset is loaded, **When** the game calls the convenience spawn function with a position and rotation, **Then** a new entity is created with Transform, Mesh, and Material components, fully visible in the scene.
4. **Given** an invalid or missing asset path is provided, **When** the game calls the load function, **Then** the engine returns a failure indicator without crashing.

---

### User Story 3 — Simulate N-Body Zero-Gravity Physics with Elastic Collisions (Priority: P3)

A game developer assigns rigid-body and collider components to entities, applies initial velocities or impulses, and observes the entities moving and colliding under Euler integration in a zero-gravity environment. Collisions between bodies are elastic — kinetic energy is conserved. Static obstacles block moving bodies without being pushed.

**Why this priority**: The physics simulation is the core gameplay mechanic — asteroids tumbling and bouncing like billiard balls in space. This is the primary interactive behavior the game layer builds upon.

**Independent Test**: Can be tested by spawning several rigid-body entities with initial velocities, letting the simulation run, and verifying that bodies move, collide, bounce elastically, and that static obstacles remain fixed.

**Acceptance Scenarios**:

1. **Given** an entity has a RigidBody component with a non-zero linear velocity and zero external forces, **When** the engine ticks, **Then** the entity's position updates proportional to velocity × dt (Euler integration).
2. **Given** two dynamic entities with AABB colliders are on a collision course, **When** they overlap during a tick, **Then** the engine resolves the collision with impulse-based elastic response (kinetic energy conserved, restitution = 1).
3. **Given** a dynamic entity collides with a static entity (infinite mass), **When** the collision is resolved, **Then** the dynamic entity bounces off and the static entity does not move.
4. **Given** an entity has a RigidBody component, **When** the game calls `apply_force` or `apply_impulse_at_point`, **Then** the entity's linear and angular velocities change accordingly on the next tick.
5. **Given** the simulation timestep is large (frame-rate spike), **When** the engine ticks, **Then** dt is clamped to a configurable maximum (e.g., 33ms) to bound integration error and prevent tunneling.

---

### User Story 4 — Navigate and Query the Scene (Priority: P4)

A game developer moves a camera through the scene using input controls, casts rays to identify which entities the player is aiming at, and queries axis-aligned bounding volumes to find nearby entities. The engine provides polled input state and event-driven callbacks.

**Why this priority**: Interaction depends on input handling, camera control, and spatial queries. Raycasting enables targeting; overlap queries enable proximity detection. These are the bridge between the simulation and gameplay.

**Independent Test**: Can be tested by moving the camera with keyboard/mouse input and verifying that raycasts return correct hit results against known entity positions.

**Acceptance Scenarios**:

1. **Given** the engine is running, **When** the game polls `key_down`, `mouse_delta`, or `mouse_button`, **Then** the current input state is returned accurately.
2. **Given** entities with Collider components exist in the scene, **When** the game calls `raycast(origin, direction, max_distance)`, **Then** the nearest intersected entity is returned with hit point, normal, and distance.
3. **Given** entities with Collider components exist in a region, **When** the game calls `overlap_aabb(center, extents)`, **Then** all entities whose AABBs intersect the query volume are returned.
4. **Given** an entity is set as the active camera, **When** the engine ticks, **Then** the engine computes view and projection matrices from the camera entity's Transform and Camera components and pushes them to the renderer.

---

### User Story 5 — Game Builds Interaction on Top of the Engine API (Priority: P5)

A game developer uses the engine's public API to construct a complete interactive experience — spawning entities, applying forces, querying collisions, reading input, and attaching game-specific components — all without modifying engine internals. The engine exposes enough ECS surface for the game to define its own components and iterate over them.

**Why this priority**: The engine exists to serve the game. The API surface must be complete and stable enough for the game layer to build a full space-shooter experience without engine modifications.

**Independent Test**: Can be tested by building a minimal game loop that spawns entities, applies physics, reads input, and responds to collisions — all through the public API — producing interactive behavior.

**Acceptance Scenarios**:

1. **Given** the engine is initialized, **When** the game attaches custom game-layer components (e.g., player controller, weapon state) to entities via the ECS, **Then** those components are stored and iterable alongside engine components.
2. **Given** the engine exposes lifecycle functions (init, tick, shutdown), **When** the game calls tick(dt) from the renderer's frame callback, **Then** physics, collision, and scene updates run for that frame.
3. **Given** the engine provides time utilities, **When** the game calls `delta_time()` or `now()`, **Then** accurate frame timing information is returned.

---

### Edge Cases

- What happens when an entity's RigidBody has zero mass (infinite mass / static)? The engine treats it as immovable — it participates in collision detection but not dynamic response.
- What happens when two AABB colliders are deeply overlapping at spawn time? The engine separates them via positional correction on the next collision-resolution pass, without explosive forces.
- What happens when a glTF file references unsupported features (skeletal animation, morph targets)? The engine ignores unsupported features and loads only the mesh geometry, logging a warning.
- What happens when the entity count grows very large (hundreds of bodies)? The brute-force collision check (O(N²)) runs per tick; performance degrades gracefully. Spatial partitioning is a stretch goal, not MVP.
- What happens when `destroy_entity` is called during iteration? The engine defers destruction to end-of-tick to avoid invalidating active iterators.
- What happens when a raycast hits no entity? The query returns an empty/null result without error.
- What happens when dt is zero or negative? The engine clamps dt to a small positive minimum and logs a warning.

## Clarifications

### Session 2026-04-23

- Q: Should FR-018 use dt-cap or fixed-substep accumulator? → A: Fixed-timestep substep accumulator at 120 Hz with dt-cap (clamp accumulated dt to max 100ms). This provides deterministic physics necessary for stable elastic collisions.
- Q: Does the game layer get direct entt::registry access or an engine abstraction? → A: Direct entt::registry& exposed to the game layer.
- Q: Is the collision layer mask MVP-required or a stretch goal? → A: Stretch goal; game filters query results manually for MVP.
- Q: What logging mechanism should the engine use? → A: fprintf(stderr, ...) with a [ENGINE] prefix tag.
- Q: How should inertia tensors be computed for RigidBody components? → A: Uniform-density box approximation from AABB half-extents.

## Requirements *(mandatory)*

### Functional Requirements

#### Scene & ECS

- **FR-001**: Engine MUST provide entity lifecycle operations: create, destroy, and query existence.
- **FR-002**: Engine MUST support attaching, retrieving, and removing arbitrary components on entities.
- **FR-003**: Engine MUST support iterating over entities matching a set of component types (view/query).
- **FR-004**: Engine MUST provide procedural shape helpers that create entities with Transform, Mesh, and Material components for spheres and cubes.
- **FR-005**: Engine MUST support a directional light entity and an active camera entity with automatic view/projection matrix computation.
- **FR-006**: Engine MUST enqueue every entity with Transform + Mesh + Material for rendering each tick.

#### Asset Pipeline

- **FR-007**: Engine MUST load mesh geometry (positions, normals, UVs, indices) from glTF/GLB files via the asset import subsystem.
- **FR-008**: Engine MUST load mesh geometry from OBJ files as a fallback format.
- **FR-009**: Engine MUST bridge loaded mesh data to the renderer's mesh upload path and return a mesh handle.
- **FR-010**: Engine MUST resolve asset file paths through a configured root directory, never via hard-coded relative paths.
- **FR-011**: Engine MUST provide a convenience function to load an asset and spawn it as a fully composed entity in one call.

#### Physics Simulation

- **FR-012**: Engine MUST integrate rigid-body linear velocity into position each tick using Euler integration (p += v × dt).
- **FR-013**: Engine MUST integrate rigid-body angular velocity into rotation each tick using Euler integration with inverse inertia tensors.
- **FR-014**: Engine MUST provide force and impulse accumulation functions (`apply_force`, `apply_impulse_at_point`) affecting linear and angular velocity.
- **FR-015**: Engine MUST detect AABB-vs-AABB overlaps between all pairs of entities with Collider components each tick.
- **FR-016**: Engine MUST resolve detected collisions using impulse-based elastic response with hardcoded restitution = 1.0 (kinetic energy conserved). Configurable per-body restitution is post-MVP.
- **FR-017**: Engine MUST treat zero-mass (static) entities as immovable during collision response.
- **FR-018**: Engine MUST run physics integration in a fixed-timestep substep loop (default 120 Hz) with a dt-cap on accumulated frame time (default 100ms) to bound simulation error. Engine MUST also clamp dt to a positive minimum (e.g., 0.0001s) when dt is zero or negative, logging a warning.

#### Input & Queries

- **FR-019**: Engine MUST expose polled input state: key-down, mouse-delta, and mouse-button queries.
- **FR-020**: Engine MUST support event-callback registration for input events forwarded from the renderer.
- **FR-021**: Engine MUST provide a ray-vs-AABB query returning the nearest hit entity, hit point, surface normal, and distance.
- **FR-022**: Engine MUST provide an AABB overlap query returning all entities whose colliders intersect a given volume.

#### Lifecycle & Time

- **FR-023**: Engine MUST expose `init`, `tick(dt)`, and `shutdown` lifecycle functions callable by the renderer frame callback.
- **FR-024**: Engine MUST expose `now()` and `delta_time()` time utilities based on the platform's high-resolution timer.

#### API Contract

- **FR-025**: Engine MUST expose a direct reference to the `entt::registry`, allowing the game layer to define, attach, and iterate over its own custom components without modifying engine code (e.g., `registry.view<PlayerController, Transform>()`).

### Key Entities

- **Entity**: A lightweight identifier representing any object in the scene. Has no behavior on its own; behavior is determined by attached components.
- **Transform**: Spatial placement of an entity — position (3D vector), rotation (quaternion), and scale (3D vector).
- **Mesh**: A handle referencing renderer-owned geometry data (vertices, indices, attributes). Created via procedural builders or asset import.
- **EntityMaterial**: Engine-side material wrapper storing the renderer's `Material` value type. Named `EntityMaterial` to avoid collision with the renderer's `Material` struct. Determines visual appearance (base color, shading model).
- **RigidBody**: Physical properties of a dynamic entity — mass, inverse mass, linear velocity, angular velocity, inverse inertia tensor (auto-computed as uniform-density box from AABB half-extents), restitution coefficient.
- **Collider**: Axis-aligned bounding box (AABB) in local space defined by half-extents. Used for collision detection and spatial queries. Layer mask filtering is a stretch goal — MVP checks all pairs; game code filters results as needed.
- **Camera**: Viewing parameters — field of view, near plane, far plane. Combined with Transform to produce view and projection matrices.
- **Light**: Illumination source — initially directional only (direction, color, intensity). Point lights are a stretch goal.
- **Tag Markers**: Lightweight flags — Static (no physics), Dynamic (physics-driven), Interactable (raycast-visible) — used to filter behavior in systems.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A standalone engine driver application displays an ECS-driven procedural scene of at least 10 primitives rendered through the renderer.
- **SC-002**: At least one glTF model loads from disk and renders correctly alongside procedural primitives in the driver application.
- **SC-003**: A cluster of 20+ rigid-body entities with initial velocities simulate in zero gravity, collide elastically, and conserve kinetic energy within 5% tolerance over 10 seconds of simulation time.
- **SC-004**: The camera is navigable via keyboard and mouse input in the driver application with no perceivable input lag at ≥30 FPS.
- **SC-005**: Raycast queries against a scene of 50+ collidable entities return correct nearest-hit results within one frame.
- **SC-006**: The game layer can create, query, and iterate over custom components through the public API without any engine source modifications.
- **SC-007**: The engine driver application runs at ≥30 FPS with 100 active rigid-body entities on the target hardware.
- **SC-008**: All engine math and physics functions pass unit tests covering integration, collision detection, and impulse response.

## Assumptions

- The renderer public API (lifecycle, mesh upload, draw enqueue, camera, lights, input bridge) is available and frozen before engine implementation begins. Engine can start against mocks and swap to real renderer at milestone merge.
- The target platform is desktop Linux (Ubuntu 24.04 LTS) with OpenGL 3.3 Core; no other platforms are considered.
- All physics simulation operates in zero gravity — there is no global gravity vector. Forces and impulses are applied explicitly by game code.
- Collision geometry is AABB-only for the MVP. Convex hulls, spheres, and capsules as collision shapes are stretch goals.
- The brute-force O(N²) collision detection approach is acceptable for the expected entity counts (≤200). Spatial partitioning (BVH, grid) is a stretch goal.
- Skeletal animation, morph targets, and material import from glTF are explicitly out of scope. Only mesh geometry (positions, normals, UVs, indices) is extracted.
- Audio, networking, particle systems, and editor tooling are explicitly cut.
- The engine does not own the main loop or window — it ticks from inside the renderer's frame callback. Input events flow from the renderer to the engine.
- Implementation order may differ from user story priority order due to technical dependencies (e.g., collider system from US4 is needed before physics in US3).
- Asset file paths are resolved via a compile-time `ASSET_ROOT` macro. No runtime path configuration is required for MVP.
- The game layer is responsible for all gameplay logic (player controllers, weapons, AI behavior, HUD). The engine provides only the simulation and query primitives.
- All engine warnings and errors are logged via `fprintf(stderr, "[ENGINE] ...")`. No callback-based or leveled logging system is required for MVP.
- Inertia tensors are auto-computed as uniform-density box approximations using AABB half-extents (`I = m/12 * diag(h²+d², w²+d², w²+h²)`). No mesh-derived or user-supplied tensors for MVP.
