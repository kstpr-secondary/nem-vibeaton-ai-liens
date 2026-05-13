# Graph Report - .  (2026-05-14)

## Corpus Check
- 116 files · ~73,816 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 745 nodes · 1066 edges · 48 communities detected
- Extraction: 72% EXTRACTED · 28% INFERRED · 0% AMBIGUOUS · INFERRED: 296 edges (avg confidence: 0.81)
- Token cost: 0 input · 0 output

## Community Hubs (Navigation)
- [[_COMMUNITY_Asset Import Pipeline|Asset Import Pipeline]]
- [[_COMMUNITY_Architecture & Design Docs|Architecture & Design Docs]]
- [[_COMMUNITY_Game Camera & Control|Game Camera & Control]]
- [[_COMMUNITY_Engine Runtime & Camera Rig|Engine Runtime & Camera Rig]]
- [[_COMMUNITY_Scene Management & Debug|Scene Management & Debug]]
- [[_COMMUNITY_App Entry & Frame Loop|App Entry & Frame Loop]]
- [[_COMMUNITY_Spatial Containment & Line Rendering|Spatial Containment & Line Rendering]]
- [[_COMMUNITY_Camera & Collision Systems|Camera & Collision Systems]]
- [[_COMMUNITY_Gameplay Tuning & Combat|Gameplay Tuning & Combat]]
- [[_COMMUNITY_Convex Hull Geometry|Convex Hull Geometry]]
- [[_COMMUNITY_Game ECS Components|Game ECS Components]]
- [[_COMMUNITY_Asteroid Field|Asteroid Field]]
- [[_COMMUNITY_AABB Collision Detection|AABB Collision Detection]]
- [[_COMMUNITY_Game App Lifecycle|Game App Lifecycle]]
- [[_COMMUNITY_Physics Test Suite|Physics Test Suite]]
- [[_COMMUNITY_Human Supervisor Protocol|Human Supervisor Protocol]]
- [[_COMMUNITY_Renderer Planning Docs|Renderer Planning Docs]]
- [[_COMMUNITY_Asteroid Spawn Helpers|Asteroid Spawn Helpers]]
- [[_COMMUNITY_Resource Shutdown|Resource Shutdown]]
- [[_COMMUNITY_Collision Research Decisions|Collision Research Decisions]]
- [[_COMMUNITY_Asteroid Init & Game Init|Asteroid Init & Game Init]]
- [[_COMMUNITY_Renderer Mock & Quickstart|Renderer Mock & Quickstart]]
- [[_COMMUNITY_Force Accumulation|Force Accumulation]]
- [[_COMMUNITY_Match State|Match State]]
- [[_COMMUNITY_Engine Delta Time|Engine Delta Time]]
- [[_COMMUNITY_Engine Time Now|Engine Time Now]]
- [[_COMMUNITY_Force Application|Force Application]]
- [[_COMMUNITY_Draw Queue Research|Draw Queue Research]]
- [[_COMMUNITY_Gameplay Constants Namespace|Gameplay Constants Namespace]]
- [[_COMMUNITY_Player Flight Constants|Player Flight Constants]]
- [[_COMMUNITY_Camera Rig Constants|Camera Rig Constants]]
- [[_COMMUNITY_Renderer Camera Struct|Renderer Camera Struct]]
- [[_COMMUNITY_Directional Light Struct|Directional Light Struct]]
- [[_COMMUNITY_Mesh Handle|Mesh Handle]]
- [[_COMMUNITY_Shader Handle|Shader Handle]]
- [[_COMMUNITY_Pipeline State|Pipeline State]]
- [[_COMMUNITY_Blend Mode|Blend Mode]]
- [[_COMMUNITY_Builtin Shader|Builtin Shader]]
- [[_COMMUNITY_Renderer Config|Renderer Config]]
- [[_COMMUNITY_Engine Config|Engine Config]]
- [[_COMMUNITY_Raycast Hit|Raycast Hit]]
- [[_COMMUNITY_Collider Component|Collider Component]]
- [[_COMMUNITY_Camera Component|Camera Component]]
- [[_COMMUNITY_Light Component|Light Component]]
- [[_COMMUNITY_Camera Active Tag|Camera Active Tag]]
- [[_COMMUNITY_Collision Flash|Collision Flash]]
- [[_COMMUNITY_Out Of Bounds Tag|Out Of Bounds Tag]]
- [[_COMMUNITY_Ray Hit|Ray Hit]]

## God Nodes (most connected - your core abstractions)
1. `engine_now()` - 23 edges
2. `game_tick()` - 21 edges
3. `on_frame()` - 20 edges
4. `frame_cb()` - 19 edges
5. `Renderer Interface Spec (FROZEN v1.2)` - 15 edges
6. `Engine Interface Spec (FROZEN v1.3)` - 15 edges
7. `game_tick()` - 15 edges
8. `renderer_handle_valid()` - 13 edges
9. `Game Design Document` - 13 edges
10. `setup_scene()` - 12 edges

## Surprising Connections (you probably didn't know these)
- `Renderer Research: Camera-Facing Billboard Line Quads` --semantically_similar_to--> `Laser weapon constants (dps, range, charge, impulse, colors)`  [INFERRED] [semantically similar]
  docs/planning/speckit/renderer/research.md → src/game/constants.h
- `Engine Research: Deferred Entity Destruction (DestroyPending)` --conceptually_related_to--> `projectile_update() implementation — uses engine_registry(), engine_now()`  [INFERRED]
  docs/planning/speckit/engine/research.md → src/game/projectile.cpp
- `camera_rig_input()` --calls--> `make_view_matrix()`  [INFERRED]
  game/camera_rig.cpp → engine/math_utils.h
- `handle_restart_quit_input()` --calls--> `engine_key_down()`  [INFERRED]
  game/game.cpp → engine/mocks/engine_mock.cpp
- `game_init()` --calls--> `engine_create_entity()`  [INFERRED]
  game/game.cpp → engine/mocks/engine_mock.cpp

## Hyperedges (group relationships)
- **Three Workstreams Connected by Frozen Interface Contracts** — renderer_interface_spec, engine_interface_spec, game_interface_spec [EXTRACTED 1.00]
- **11-Step Game Tick System Execution Order** — game_workstream_tick_order, game_interface_system_order_rationale, game_interface_lifecycle [EXTRACTED 1.00]
- **Game SpecKit Planning Artifact Chain (spec→research→data-model→plan→tasks)** — speckit_game_spec, speckit_game_research, speckit_game_data_model, speckit_game_plan, speckit_game_tasks [EXTRACTED 0.95]
- **Physics Simulation Pipeline: RigidBody + Collider + Baumgarte** — engine_datamodel_rigidbody, engine_datamodel_collider, engine_research_baumgarte [INFERRED 0.85]
- **Per-Frame Game Systems: game_tick drives enemy_ai, projectile, damage, containment** — game_h_game_tick, enemy_ai_h_enemy_ai_update, projectile_h_projectile_update, damage_h_damage_resolve [INFERRED 0.80]
- **ECS Entity Lifecycle: Transform + Mesh + EntityMaterial as renderable composition** — engine_datamodel_transform, engine_datamodel_mesh, engine_datamodel_entitymaterial [EXTRACTED 0.95]
- **game_tick Orchestrates All Per-Frame Systems** — game_game_tick, player_player_update, enemy_ai_enemy_ai_update, weapons_weapon_update, damage_damage_resolve, camera_rig_camera_rig_finalize, shield_vfx_shield_vfx_update, vfx_vfx_update, hud_hud_render, game_render_submit [EXTRACTED 1.00]
- **Shield Health → ShieldSphere VFX Update Pipeline** — components_shield, components_shieldsphere, shield_vfx_shield_vfx_update [EXTRACTED 1.00]
- **Weapon Firing: Cursor Ray → Projectile/Laser Spawn** — camera_rig_camera_rig_cursor_ray, weapons_weapon_update, spawn_spawn_projectile [EXTRACTED 1.00]
- **Renderer Pipeline Creation Pattern (magenta fallback)** — pipeline_unlit_create_pipeline_unlit, pipeline_lambertian_create_pipeline_lambertian, pipeline_blinnphong_create_pipeline_blinnphong [EXTRACTED 0.95]
- **Texture Store Lifecycle (insert, get, shutdown)** — texture_texture_store_insert, texture_texture_get, texture_texture_store_shutdown [EXTRACTED 0.92]
- **Per-Frame Renderer Pipeline (begin → enqueue → end)** — renderer_renderer_begin_frame, renderer_renderer_enqueue_draw, renderer_renderer_end_frame [EXTRACTED 0.95]
- **Physics Simulation Pipeline: substep, integrate, resolve** — physics_physics_substep, physics_integrate_linear, physics_integrate_angular, collision_response_resolve_all_collisions [EXTRACTED 0.95]
- **Collision Detection and Response Pipeline** — collider_detect_collisions, collision_response_resolve_collision, collision_response_positional_correction [EXTRACTED 0.95]
- **Asset Loading Pipeline: import, hull, upload** — scene_api_engine_load_gltf_impl, convex_hull_compute_convex_hull, asset_bridge_asset_bridge_upload [EXTRACTED 0.92]
- **engine_tick() Orchestration Pipeline** — engine_engine_tick, physics_physics_system_tick, camera_camera_update, engine_time_time_set_delta [EXTRACTED 0.95]
- **Mesh Asset Import Pipeline (glTF + OBJ + texture cache)** — asset_import_asset_import_gltf, obj_import_asset_import_obj, asset_import_texture_cache [INFERRED 0.85]
- **Physics Fixed-Timestep Substep Loop** — physics_physics_system_tick, physics_physics_substep, collision_response_resolve_all_collisions [INFERRED 0.85]

## Communities

### Community 0 - "Asset Import Pipeline"
Cohesion: 0.04
Nodes (70): asset_bridge_upload(), asset_bridge_upload() (header), asset_extract_textures(), asset_import_gltf(), ExtractedTexture, ImportedMesh, texture_cache() (static), camera_update() (+62 more)

### Community 1 - "Architecture & Design Docs"
Cohesion: 0.04
Nodes (66): Cross-Workstream Dependency Table, Cross-Workstream Architecture, Shared Technology Decisions, System Integration Diagram (sokol_app→Renderer→Engine→Game), Hackathon Presentation Cheat Sheet, AI Agent Stack (9 instances, 6 providers), Domain Skills (distilled API cheat-sheets), Mock System (CMake USE_*_MOCKS flags) (+58 more)

### Community 2 - "Game Camera & Control"
Cohesion: 0.06
Nodes (63): camera_rig_cursor_ray(), camera_rig_finalize(), camera_rig_init(), camera_rig_input(), AsteroidData Component, AsteroidTag Component, Boost Component, CameraRigState Component (+55 more)

### Community 3 - "Engine Runtime & Camera Rig"
Cohesion: 0.07
Nodes (46): camera_rig_cursor_ray(), camera_rig_finalize(), camera_rig_get_vp(), camera_rig_init(), camera_rig_input(), aabb_overlap(), apply_damage(), damage_resolve() (+38 more)

### Community 4 - "Scene Management & Debug"
Cohesion: 0.05
Nodes (44): asset_extract_textures(), asset_import_gltf(), scene_flush_pending_destroys(), debug_draw_compute_billboard(), mesh_aabb_get(), mesh_ibuf_get(), mesh_index_count_get(), mesh_store_free() (+36 more)

### Community 5 - "App Entry & Frame Loop"
Cohesion: 0.07
Nodes (44): frame_cb(), main(), on_frame(), ring_pos(), setup_scene(), spawn_shape(), update_camera(), update_highlight() (+36 more)

### Community 6 - "Spatial Containment & Line Rendering"
Cohesion: 0.05
Nodes (38): containment_update(), reflect_off_boundary() (static), debug_draw_compute_billboard(), LineQuadCommand (struct), LineQuadVertex (struct), renderer_upload_mesh(), MeshAABB (struct), create_pipeline_blinnphong() (+30 more)

### Community 7 - "Camera & Collision Systems"
Cohesion: 0.07
Nodes (20): camera_update(), positional_correction(), resolve_all_collisions(), resolve_collision(), engine_init(), engine_on_collision(), engine_shutdown(), engine_tick() (+12 more)

### Community 8 - "Gameplay Tuning & Combat"
Cohesion: 0.07
Nodes (36): containment_update() — reflect out-of-bounds entities, cap speed, Enemy AI constants (health, shield, speed, fire range, cooldown), Laser weapon constants (dps, range, charge, impulse, colors), Plasma weapon constants (damage, cooldown, speed, lifetime, radius), apply_damage() — shield-first damage absorption, damage_resolve() — AABB overlap damage resolution per frame, enemy_ai_update(float dt) — seek player + fire plasma, Engine Public API Contract (engine.h shape, FROZEN) (+28 more)

### Community 9 - "Convex Hull Geometry"
Cohesion: 0.14
Nodes (19): compute_convex_hull(), compute_face_normals(), compute_half_extents(), gw_pivot(), hull_cache_get(), hull_cache_insert(), tetra_vol6(), tri_normal() (+11 more)

### Community 10 - "Game ECS Components"
Cohesion: 0.11
Nodes (24): Game SpecKit Data Model, Boost Component (drain_rate, regen_rate), EnemyAI Component (pursuit_speed, fire_range, fire_cooldown), FieldConfig Singleton (radius=1000, asteroid_count=200), Health Component, MatchState Singleton (phase, enemies_remaining), ProjectileData Component (owner, damage, lifetime), Shield Component (regen_rate, regen_delay) (+16 more)

### Community 11 - "Asteroid Field"
Cohesion: 0.19
Nodes (18): asteroid_field_init(), containment_update(), random_direction(), random_point_in_shell(), reflect_off_boundary(), spawn_tier(), asteroid_color_variation(), box_inv_inertia() (+10 more)

### Community 12 - "AABB Collision Detection"
Cohesion: 0.26
Nodes (9): aabb_overlap(), compute_contact(), compute_world_aabb(), detect_collisions(), hull_vs_aabb_sat(), engine_overlap_aabb(), engine_raycast(), ray_vs_aabb() (+1 more)

### Community 13 - "Game App Lifecycle"
Cohesion: 0.32
Nodes (7): AppState, frame_callback() (static), main(), on_frame(), on_input(), ShadingOverride, spawn_shape()

### Community 14 - "Physics Test Suite"
Cohesion: 0.38
Nodes (3): kinetic_energy_angular(), kinetic_energy_linear(), total_ke()

### Community 15 - "Human Supervisor Protocol"
Cohesion: 0.29
Nodes (7): Human Supervisor Runbook, AI Tool Priority and Escalation, Milestone Merge Protocol, Scope-Cut Contingency Order, Source Control Protocol, Task Claiming Protocol, Validation and Review Rules

### Community 16 - "Renderer Planning Docs"
Cohesion: 0.5
Nodes (5): Renderer SpecKit Contracts README, Renderer SpecKit Plan, Renderer Research: OpenGL 3.3 Core Backend Decision, Renderer Research: sokol-shdc Build-Time Shader Pipeline, Renderer SpecKit Tasks (Guide Copy)

### Community 17 - "Asteroid Spawn Helpers"
Cohesion: 0.67
Nodes (3): random_direction() (static), random_point_in_shell() (static), spawn_tier() (static)

### Community 18 - "Resource Shutdown"
Cohesion: 0.5
Nodes (1): renderer_shutdown()

### Community 21 - "Collision Research Decisions"
Cohesion: 0.67
Nodes (3): Engine Research: Baumgarte Stabilization for Positional Correction, Engine Research: Minimum-Penetration SAT Axis Contact Normal, Engine Research: Restitution Blending (min strategy)

### Community 22 - "Asteroid Init & Game Init"
Cohesion: 0.67
Nodes (2): asteroid_field_init() — spawn all asteroids, Asteroid size tier constants (small, medium, large scale/mass/speed/spin)

### Community 27 - "Renderer Mock & Quickstart"
Cohesion: 1.0
Nodes (2): Renderer Quickstart Build Guide, Renderer Research: Mock Surface (USE_RENDERER_MOCKS)

### Community 28 - "Force Accumulation"
Cohesion: 1.0
Nodes (2): Runtime: ForceAccumulator (transient per-step, not ECS component), Engine Research: Per-Step Force Accumulation Map

### Community 29 - "Match State"
Cohesion: 1.0
Nodes (2): Match state constants (enemies_at_start, restart delays), MatchState struct (phase, phase_enter_time, auto_restart_delay, enemies_remaining)

### Community 30 - "Engine Delta Time"
Cohesion: 1.0
Nodes (1): engine_delta_time() impl

### Community 31 - "Engine Time Now"
Cohesion: 1.0
Nodes (1): engine_now() impl

### Community 32 - "Force Application"
Cohesion: 1.0
Nodes (1): ForceAccum

### Community 69 - "Draw Queue Research"
Cohesion: 1.0
Nodes (1): Renderer Research: 1024-Entry Fixed Draw Queue

### Community 70 - "Gameplay Constants Namespace"
Cohesion: 1.0
Nodes (1): constants namespace — all gameplay tuning values

### Community 71 - "Player Flight Constants"
Cohesion: 1.0
Nodes (1): Player flight and resource constants (thrust, drag, shield, boost)

### Community 72 - "Camera Rig Constants"
Cohesion: 1.0
Nodes (1): Camera rig constants (offset, lag, turn rate, bank/pitch spring)

### Community 74 - "Renderer Camera Struct"
Cohesion: 1.0
Nodes (1): RendererCamera (struct)

### Community 75 - "Directional Light Struct"
Cohesion: 1.0
Nodes (1): DirectionalLight (struct)

### Community 76 - "Mesh Handle"
Cohesion: 1.0
Nodes (1): RendererMeshHandle (struct)

### Community 77 - "Shader Handle"
Cohesion: 1.0
Nodes (1): RendererShaderHandle (struct)

### Community 78 - "Pipeline State"
Cohesion: 1.0
Nodes (1): PipelineState (struct)

### Community 79 - "Blend Mode"
Cohesion: 1.0
Nodes (1): BlendMode (enum)

### Community 80 - "Builtin Shader"
Cohesion: 1.0
Nodes (1): BuiltinShader (enum)

### Community 81 - "Renderer Config"
Cohesion: 1.0
Nodes (1): RendererConfig (struct)

### Community 89 - "Engine Config"
Cohesion: 1.0
Nodes (1): EngineConfig

### Community 90 - "Raycast Hit"
Cohesion: 1.0
Nodes (1): RaycastHit

### Community 102 - "Collider Component"
Cohesion: 1.0
Nodes (1): Collider

### Community 103 - "Camera Component"
Cohesion: 1.0
Nodes (1): Camera

### Community 104 - "Light Component"
Cohesion: 1.0
Nodes (1): Light

### Community 105 - "Camera Active Tag"
Cohesion: 1.0
Nodes (1): CameraActive (tag)

### Community 106 - "Collision Flash"
Cohesion: 1.0
Nodes (1): CollisionFlash

### Community 107 - "Out Of Bounds Tag"
Cohesion: 1.0
Nodes (1): OutOfBounds (tag)

### Community 110 - "Ray Hit"
Cohesion: 1.0
Nodes (1): RayHit

## Knowledge Gaps
- **133 isolated node(s):** `Task Claiming Protocol`, `Validation and Review Rules`, `AI Tool Priority and Escalation`, `Scope-Cut Contingency Order`, `Source Control Protocol` (+128 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **Thin community `Resource Shutdown`** (4 nodes): `mesh_store_shutdown()`, `renderer_shutdown()`, `skybox_shutdown()`, `texture_store_shutdown()`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Asteroid Init & Game Init`** (3 nodes): `asteroid_field_init() — spawn all asteroids`, `Asteroid size tier constants (small, medium, large scale/mass/speed/spin)`, `game_init()`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Renderer Mock & Quickstart`** (2 nodes): `Renderer Quickstart Build Guide`, `Renderer Research: Mock Surface (USE_RENDERER_MOCKS)`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Force Accumulation`** (2 nodes): `Runtime: ForceAccumulator (transient per-step, not ECS component)`, `Engine Research: Per-Step Force Accumulation Map`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Match State`** (2 nodes): `Match state constants (enemies_at_start, restart delays)`, `MatchState struct (phase, phase_enter_time, auto_restart_delay, enemies_remaining)`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Engine Delta Time`** (2 nodes): `engine_delta_time()`, `engine_delta_time() impl`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Engine Time Now`** (2 nodes): `engine_now()`, `engine_now() impl`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Force Application`** (2 nodes): `engine_apply_force()`, `ForceAccum`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Draw Queue Research`** (1 nodes): `Renderer Research: 1024-Entry Fixed Draw Queue`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Gameplay Constants Namespace`** (1 nodes): `constants namespace — all gameplay tuning values`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Player Flight Constants`** (1 nodes): `Player flight and resource constants (thrust, drag, shield, boost)`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Camera Rig Constants`** (1 nodes): `Camera rig constants (offset, lag, turn rate, bank/pitch spring)`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Renderer Camera Struct`** (1 nodes): `RendererCamera (struct)`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Directional Light Struct`** (1 nodes): `DirectionalLight (struct)`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Mesh Handle`** (1 nodes): `RendererMeshHandle (struct)`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Shader Handle`** (1 nodes): `RendererShaderHandle (struct)`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Pipeline State`** (1 nodes): `PipelineState (struct)`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Blend Mode`** (1 nodes): `BlendMode (enum)`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Builtin Shader`** (1 nodes): `BuiltinShader (enum)`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Renderer Config`** (1 nodes): `RendererConfig (struct)`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Engine Config`** (1 nodes): `EngineConfig`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Raycast Hit`** (1 nodes): `RaycastHit`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Collider Component`** (1 nodes): `Collider`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Camera Component`** (1 nodes): `Camera`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Light Component`** (1 nodes): `Light`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Camera Active Tag`** (1 nodes): `CameraActive (tag)`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Collision Flash`** (1 nodes): `CollisionFlash`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Out Of Bounds Tag`** (1 nodes): `OutOfBounds (tag)`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.
- **Thin community `Ray Hit`** (1 nodes): `RayHit`
  Too small to be a meaningful cluster - may be noise or needs more connections extracted.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `frame_cb()` connect `App Entry & Frame Loop` to `Asset Import Pipeline`, `Convex Hull Geometry`, `Engine Runtime & Camera Rig`?**
  _High betweenness centrality (0.089) - this node is a cross-community bridge._
- **Why does `engine_tick()` connect `Asset Import Pipeline` to `App Entry & Frame Loop`?**
  _High betweenness centrality (0.073) - this node is a cross-community bridge._
- **Are the 22 inferred relationships involving `engine_now()` (e.g. with `projectile_update()` and `enemy_ai_update()`) actually correct?**
  _`engine_now()` has 22 INFERRED edges - model-reasoned connections that need verification._
- **Are the 15 inferred relationships involving `game_tick()` (e.g. with `renderer_set_time()` and `engine_now()`) actually correct?**
  _`game_tick()` has 15 INFERRED edges - model-reasoned connections that need verification._
- **Are the 17 inferred relationships involving `on_frame()` (e.g. with `renderer_make_sphere_mesh()` and `renderer_make_cube_mesh()`) actually correct?**
  _`on_frame()` has 17 INFERRED edges - model-reasoned connections that need verification._
- **Are the 14 inferred relationships involving `frame_cb()` (e.g. with `renderer_begin_frame()` and `engine_tick()`) actually correct?**
  _`frame_cb()` has 14 INFERRED edges - model-reasoned connections that need verification._
- **What connects `Task Claiming Protocol`, `Validation and Review Rules`, `AI Tool Priority and Escalation` to the rest of the system?**
  _133 weakly-connected nodes found - possible documentation gaps or missing edges._