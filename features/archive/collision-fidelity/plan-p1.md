# Phase Plan: Collision Fidelity — Phase 1: Diagnose & Fix AABB Calibration

**Gate**: Feature Brief approved (Phase 1)
**Roadmap alignment**: N/A (Phased feature)
**Build target**: `cmake --build build --target game engine_tests`
**Groomer Verdict**: PASS

## Expected Outcome

AABB colliders on asteroid entities are accurately sized to the rendered mesh; a toggleable debug mode shows semi-transparent green boxes that visually hug each asteroid tier rather than a much-larger surrounding cube.

---

## Tasks

| ID | Status | Description | Files | Schedule | Acceptance |
|----|--------|-------------|-------|----------|------------|
| 1 | [X] | **Mesh diagnostic — engine-internal only, no interface change.** Add a `static float s_mesh_local_half_extent(const float* positions, size_t count)` helper (file-scope, not exported) inside `asset_import.cpp`; it takes the raw positions array (stride-3) and returns `max(|x|, |y|, |z|)` over all vertices. Inside `asset_import_gltf()`, immediately after the `cgltf_accessor_unpack_floats` call that populates `out.positions`, add a one-time print block guarded by a `static std::unordered_set<std::string> s_aabb_logged` (file-scope): if `relative_path` is not yet in the set, insert it and `fprintf(stderr, ...)` the model name, vertex count, local min/max across all three axes, and `local_half_extent`. No new exported symbols, no changes to `asset_import.h`, no changes to `engine.h`, no game-side changes. The diagnostic fires automatically the first time each unique model path is loaded. | `src/engine/asset_import.cpp` | GATE | Running `./build/game` prints one diagnostic block per model (Asteroid_1a.glb, Asteroid_1e.glb, Asteroid_2a.glb, Asteroid_2b.glb) on stderr at startup, listing vertex count, local AABB min/max, and computed half-extent. |
| 2 | [X] | **Fix `col.half_extents` calibration.** Using the diagnostic output from T1, determine `local_half_extent` for each model. Read T1's diagnostic output for all four models. **Policy**: if all four local half-extents agree within 10% of each other, treat them as a single shared value and replace `col.half_extents = glm::vec3(radius)` in each tier's `switch` case with `col.half_extents = glm::vec3(k_asteroid_scale_* * shared_half_extent)`. If they differ by more than 10%, use the maximum across all four models as a conservative bound (no asteroid will clip through its mesh; some will have a small over-estimate). Do not invent a per-model lookup: that belongs to Phase 2's per-model hull work. In either case, remove the diagnostic print block and `s_aabb_logged` guard from `asset_import.cpp`. Watch for: `compute_world_aabb` ignores `Transform.scale`; the half_extents must encode world-space half-sizes, so the correct formula is `render_scale × policy_half_extent`, where `policy_half_extent` is the shared value (if models agree within 10%) or the max across all four models (if they differ). | `src/game/spawn.cpp`, `src/engine/asset_import.cpp` | → | **If values changed**: `cmake --build build --target game engine_tests` exits 0 with no new test failures; updated half_extents in `spawn_asteroid()` match `k_asteroid_scale_* × policy_half_extent` (shared or max per the policy above). **If T1 shows the current values are already correct** (policy_half_extent ≈ 2.0f, so `radius == render_scale × 2.0f`): a comment is added above `col.half_extents = glm::vec3(radius)` in each tier case documenting the verified policy value (e.g., `// verified: policy half-extent ≈ 2.0 units (max across 4 models)`); build exits 0 with no test failures. Either way, the T1 diagnostic code is removed before this task is marked done. |
| 3 | [X] | **Debug AABB visualization.** Add a toggleable overlay that spawns a semi-transparent green box for each asteroid to visualise its AABB. Implementation sketch: (a) Add `static bool s_debug_aabb` and `static std::vector<std::pair<entt::entity,entt::entity>> s_debug_pairs` (asteroid → debug box) at file scope in `game.cpp`. (b) Add `aabb_debug_show()`: iterate `reg.view<Transform, Collider, AsteroidTag>()`, for each call `engine_spawn_cube(t.position, c.half_extents.x, debug_mat)` and push the pair; `debug_mat` is `renderer_make_blinnphong_material(green_rgb, 0.f, {})` with `mat.pipeline = {BlendMode::AlphaBlend, CullMode::Back, false, 2}` and `p->base_color.a = 0.25f` (Blinn-Phong FS outputs `base_color.a` as fragment alpha in solid-color mode). (c) Add `aabb_debug_hide()`: destroy all debug entities. (d) Add `aabb_debug_sync()`: iterate `s_debug_pairs`, update `debug_transform.position = asteroid_transform.position` for each live pair (skip if `reg.valid(asteroid)` is false). (e) Add `handle_debug_input()` (F3 edge-triggered) called from `game_tick()` alongside the existing `handle_restart_quit_input()` call at line 260 of `game.cpp`, toggling `s_debug_aabb` and calling show/hide. (f) Call `aabb_debug_sync()` each frame when `s_debug_aabb` is true; call `aabb_debug_hide()` in `scene_reset()` / on restart to avoid stale entities. | `src/game/game.cpp` | → | Pressing F3 in-game shows / hides semi-transparent green cubes around all asteroids; cubes do not rotate with the asteroid mesh (world-AABB is axis-aligned); no crash on toggle or on game restart. |

**Schedule codes**:
- `GATE` — runs alone first; everything after depends on it
- `→` — sequential, runs after all preceding tasks

---

## Fallbacks

- **T1**: If the `static std::unordered_set` guard causes link or ODR issues (unlikely but possible on some toolchains), replace with a plain `static bool s_aabb_logged_once[4] = {}` indexed by the position of the path in a hardcoded array of the four model names — same effect, no STL containers.
- **T3**: If `BlendMode::AlphaBlend` on Blinn-Phong produces Z-sort artefacts that make the boxes hard to read (600 overlapping translucent faces), fall back to `BlendMode::Additive` with a very dim color `{0.02f, 0.1f, 0.02f}` and set `base_color.a = 1.0f`.
- **T3**: `engine_spawn_cube` takes a uniform `half_extent float`. Asteroid AABBs are always uniform (`glm::vec3(radius)`), so this is fine for all tiers. If non-uniform half_extents are ever needed, use `renderer_make_cube_mesh(1.0f)` + `Transform.scale = c.half_extents` via a manual entity build.

---

## Human Checkpoint

**Run**: `cmake --build build --target game engine_tests && ./build/game`

**Look for**:
1. *(After T1 only)* Terminal output: one diagnostic block per model (Asteroid_1a.glb, Asteroid_1e.glb, Asteroid_2a.glb, Asteroid_2b.glb) showing vertex count, local AABB min/max, and computed local half-extent. (The corrected world-space `col.half_extents` is derived in T2 by the implementor as `k_asteroid_scale_* × local_half_extent`.)
2. *(After T2 + T3)* Press **F3** in-game: semi-transparent green boxes appear around every asteroid. The boxes should visibly hug the rocky mesh rather than floating well outside it. Press F3 again to hide. Navigate the asteroid field at normal speed and note whether invisible-wall collisions are noticeably reduced.

**Pass**: Debug boxes sit tightly around the asteroid mesh (a small gap from mesh irregularity is expected but the box should not extend visibly beyond open space around the asteroid); player navigation in a dense cluster feels materially less wall-bumpy than before.

**Stop**: Debug boxes are still clearly larger than the visible mesh after T2's fix, OR the fix produced incorrect values that break existing `engine_tests` (exit non-zero). Raise the discrepancy for review — do not proceed to Phase 2 planning.
