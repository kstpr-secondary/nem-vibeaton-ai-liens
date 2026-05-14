# Engine SpecKit — Research

> **Phase 0 output** for `002-ecs-physics-engine`. All NEEDS CLARIFICATION items resolved.

---

## Decision 1: Engine–Renderer Integration Pattern

**Decision**: The engine does NOT own the main loop. `renderer_run()` drives `sokol_app`'s event loop. The engine registers a per-frame tick via `renderer_set_input_callback()`. Inside the renderer's frame callback, `engine_tick(dt)` is invoked. The engine's `init()` takes a config struct (no renderer reference stored — all renderer calls go through the frozen free-function API in `renderer.h`).  
**Rationale**: The renderer owns `sokol_app`, which requires the main thread. The engine cannot drive its own loop. This matches `renderer-interface-spec.md v1.0` calling convention: `renderer_init()` → `renderer_set_input_callback()` → `renderer_run()` (blocking). The engine registers its tick as a frame callback and issues `renderer_begin_frame()` / `renderer_end_frame()` each tick.  
**Alternatives considered**: Engine owns main loop and calls renderer per-frame (rejected — sokol_app requires owning the loop); coroutine-based interleaving (rejected — overengineered).  
**Key implication**: `engine_app/main.cpp` calls `renderer_init()`, registers a callback that calls `engine_tick(dt)` sandwiched between `renderer_begin_frame()` and `renderer_end_frame()`, then calls `renderer_run()`.

---

## Decision 2: Physics Integration Strategy — Fixed-Timestep vs dt-Cap

**Decision**: Use **fixed-timestep substepping with dt-cap accumulator** (120 Hz substeps, DT_CAP = 0.1s). This satisfies FR-018 (dt is clamped) while providing stable physics.  
**Rationale**: The spec clarification says "dt-cap only (clamp to max, e.g. 33ms)." However, domain analysis (physics-euler SKILL §5) shows that pure variable-dt Euler integration tunnels fast-moving bodies through geometry on frame spikes. Fixed substepping at 120 Hz is cheap for O(N²) ≤200 entities and prevents tunneling. The dt-cap prevents spiral-of-death. This **supersedes** the simplistic dt-cap-only answer in the spec clarification, which was a scope simplification that would produce incorrect physics.  
**Alternatives considered**: Pure dt-cap with variable step (FR-018 literal — rejected: tunneling), RK4 integration (rejected: overkill for hackathon), verlet integration (rejected: harder to add impulse response).  
**Spec deviation note**: FR-018 is satisfied (dt IS clamped). The addition of fixed substepping is an implementation detail that makes the cap actually effective. No functional requirement is violated. This deviation is flagged for human supervisor awareness.

---

## Decision 3: Component Layout — Renderer Handle Storage

**Decision**: Engine components `Mesh` and `Material` store **renderer handle types directly** (`RendererMeshHandle`, `RendererTextureHandle`) from the frozen renderer interface. No engine-side wrapper.  
**Rationale**: The renderer interface spec defines `RendererMeshHandle { uint32_t id; }` and `Material { ShadingModel, base_color[4], texture, shininess, alpha }`. Wrapping these adds zero value in a hackathon — it just duplicates types. The engine includes `renderer.h` (which is its `PUBLIC` dependency) and stores the handles directly.  
**Alternatives considered**: Engine-local `EngineMeshHandle` that maps to renderer handles (rejected — indirection with no benefit), storing raw `uint32_t` IDs (rejected — loses type safety).

---

## Decision 4: Asset Import — Vertex Layout Bridge

**Decision**: The engine's `asset_bridge.cpp` converts cgltf extracted vertex data into the renderer's frozen `Vertex` struct (`{position[3], normal[3], uv[2], tangent[3]}`) and calls `renderer_upload_mesh()`. Tangent is zero-filled if absent from the glTF; normals are zero-filled with Y-up default if absent.  
**Rationale**: The renderer's frozen `Vertex` layout includes tangent for future normal-map support (R-M7 stretch). Engine must supply all 11 floats per vertex even at Unlit/Lambertian tier — the pipeline expects it. Zero-fill is harmless: Unlit ignores normals/tangents, Lambertian uses only normals.  
**Alternatives considered**: Request a compact vertex layout from renderer (rejected — frozen interface, cannot change), strip tangents and pad at upload (same result, less clear).

---

## Decision 5: Inertia Tensor Computation

**Decision**: Compute body-space inverse inertia tensor at entity spawn time using **uniform-density box approximation** from AABB half-extents: `I_x = m/3 * (hy² + hz²)`, etc. Store `inv_inertia_body: mat3` as a cached value on the RigidBody. Recompute `inv_inertia_world` each physics substep from rotation.  
**Rationale**: Spec clarification confirms: "Uniform-density box approximation from AABB half-extents." This is fast (3 divides at spawn) and physically adequate for box-like asteroids. The world-space inverse must be rotated each substep: `R * I_body_inv * R^T`.  
**Alternatives considered**: Sphere inertia (incorrect for non-spherical AABBs), mesh-derived tensors (out of scope per spec), user-supplied tensors (spec says no).

---

## Decision 6: Collision Contact Normal

**Decision**: Use the **minimum-penetration SAT axis** for AABB-vs-AABB contact normal. The axis (±X, ±Y, or ±Z) with the smallest overlap gives the most stable contact direction.  
**Rationale**: AABB collision is a separating-axis test on 3 axes. The axis with minimum overlap is the best contact normal — it minimizes positional correction distance and avoids jitter from axis switching between frames.  
**Alternatives considered**: Always use the relative-velocity direction (incorrect for resting contacts), center-to-center direction (fails for grazing collisions).

---

## Decision 7: Force and Torque Accumulation

**Decision**: Forces and torques are accumulated into **per-step local vectors** (stack-allocated in the substep function), not stored in the RigidBody component. `apply_force()` and `apply_impulse_at_point()` write into a **per-entity map** (`std::unordered_map<entt::entity, ForceAccum>`) that is consumed and cleared each substep.  
**Rationale**: Force accumulators are transient — they only matter for one substep. Storing them in the ECS component wastes storage for the 90%+ of frames where no external force is applied. A separate map keeps the RigidBody component slim.  
**Alternatives considered**: Store forces in RigidBody (wastes 24 bytes per entity per tick), separate ForceAccum ECS component (structural changes on force application violate mid-view safety rules — rejected).

---

## Decision 8: Input Bridge Architecture

**Decision**: Engine registers a single `InputCallback` with the renderer via `renderer_set_input_callback(cb, user_data)`. Inside the callback, the engine updates its polled state table (`key_states[256]`, `mouse_pos`, `mouse_delta`, `mouse_buttons[3]`) and pushes events into a ring buffer. Game code reads polled state via `key_down()`, `mouse_delta()`, `mouse_button()` and drains the event buffer via an optional callback.  
**Rationale**: The renderer's frozen interface exposes exactly one input callback slot. The engine fills it; game code queries the engine's processed state. This avoids coupling game code to sokol_app event types.  
**Alternatives considered**: Pass sokol_app events through to game (rejected — couples game to renderer internals), multiple callback slots (rejected — renderer API has one slot).

---

## Decision 9: Entity Destruction — Deferred Pattern

**Decision**: `destroy_entity(e)` marks the entity with a `DestroyPending` tag component. At the end of each `engine_tick()`, a sweep pass collects all `DestroyPending` entities and calls `registry.destroy()` on them.  
**Rationale**: entt forbids structural changes (create/destroy/emplace/remove) during view iteration. Game code may call `destroy_entity()` during collision callbacks or gameplay logic that runs inside a view. Deferring ensures safety.  
**Alternatives considered**: Immediate destroy with iterator invalidation handling (UB per entt rules — rejected), command buffer pattern (overengineered for hackathon — rejected).

---

## Decision 10: Camera Matrix Computation

**Decision**: Engine computes view and projection matrices from the active camera entity's `Transform` and `Camera` components each frame. View = `glm::inverse(TRS_matrix(camera_transform))`. Projection = `glm::perspective(radians(cam.fov), aspect, cam.near, cam.far)`. Both are pushed to the renderer via `renderer_set_camera(RendererCamera{view, projection})`.  
**Rationale**: The renderer's frozen interface takes pre-computed `float[16]` matrices. The engine owns the Transform and Camera components and is responsible for the math. Aspect ratio comes from the renderer's window dimensions.  
**Alternatives considered**: Renderer computes from camera entity (rejected — renderer doesn't know about entt entities), game computes (rejected — engine owns the camera system).

---

## Decision 11: Restitution Blending for Collisions

**Decision**: Use `e = min(rb_A.restitution, rb_B.restitution)` for the combined restitution coefficient.  
**Rationale**: For the MVP, all asteroids have `restitution = 1.0` (perfectly elastic). Using `min()` ensures that any future non-elastic body (e.g., a wall with e=0.5) produces the lower restitution. This is physically reasonable and consistent with the physics-euler SKILL recommendation.  
**Alternatives considered**: Product `e_A * e_B` (standard in some engines, slightly different behavior — acceptable alternative), average (non-physical — rejected), hardcoded 1.0 (rejected — breaks when game adds non-elastic bodies).

---

## Decision 12: Positional Correction for Penetration

**Decision**: Use **Baumgarte stabilization** with `k_slop = 0.005` and `k_baumgarte = 0.3` to push overlapping bodies apart after impulse resolution.  
**Rationale**: Without positional correction, bodies sink into each other over time despite correct impulse response. Baumgarte is cheap (one multiply + one add per body pair) and sufficient for AABB contacts. It prevents sinking without velocity artifacts.  
**Alternatives considered**: Direct projection (velocity artifacts, rejected), no correction (sinking, rejected), split impulse (overcomplicated for hackathon, rejected).

---

## Open-Question Resolutions

| Question | Resolution |
|----------|-----------|
| Physics dt strategy | Fixed 120 Hz substep + dt-cap accumulator (supersedes spec's dt-cap-only clarification; flagged for supervisor) |
| entt::registry access for game | Direct `entt::registry&` exposed; game iterates with `registry.view<Components...>()` per spec clarification |
| Collision layer masks | Stretch goal; game filters query results manually for MVP per spec clarification |
| Logging mechanism | `fprintf(stderr, "[ENGINE] ...")` with prefix tag per spec clarification |
| Inertia tensor model | Uniform-density box from AABB half-extents per spec clarification |
| Renderer Vertex struct tangent field | Zero-filled by engine at upload for MVP tiers (Unlit/Lambertian ignore it) |
| Aspect ratio for projection | Obtained from `sapp_widthf()` / `sapp_heightf()` via sokol_app (engine includes sokol_app transitively through renderer) |
| Window dimensions access | Engine calls `sapp_widthf()` / `sapp_heightf()` directly (sokol_app is transitively available) |
