# Visual Improvements — Implementation Audit
*Audited: 2026-05-07*  
*Branch: feature/renderer*  
*Reference spec: `pre_planning_docs/next-gen-tasks/visual-improvements.md`*

---

## Result Summary

| Task | File(s) | Status | Notes |
|------|---------|--------|-------|
| T01 | `shaders/game/shield.glsl` | ✅ PASS | — |
| T02 | `shaders/game/plasma_ball.glsl` | ✅ PASS | — |
| T03 | `src/renderer/renderer.cpp`, `renderer.h` | ⚠️ ISSUES | Three defects — see below |
| T04 | `src/game/components.h` | ✅ PASS | — |
| T05 | `src/game/constants.h` | ⚠️ MINOR | Dead constant not removed |
| T06 | `src/game/game_shaders.h/.cpp` | ✅ PASS | — |
| T07 | `src/game/weapons.cpp` | ⚠️ MINOR | Variable shadowing; dead code block |
| T08 | `src/game/spawn.cpp` | ✅ PASS | — |
| T09 | `src/game/vfx.h/.cpp` | ✅ PASS | — |
| T10 | `src/game/shield_vfx.h/.cpp` | ⚠️ MINOR | Camera position 1-frame stale; spec divergence |
| T11 | `src/game/damage.cpp`, `weapons.cpp` | ✅ PASS | — |
| T12 | `src/game/game.cpp` | ✅ PASS | — |

---

## Detailed Findings

### T01 — `shaders/game/shield.glsl` ✅ PASS

Matches spec (Section 3.3.1) exactly.

- VS binding 0: standard 3-matrix block (`mvp`, `model`, `normal_mat`) ✅
- VS inputs `position`, `normal`; outputs `v_normal`, `v_world_pos` ✅
- FS `shield_fs_params` has all three vec4 fields (`shield_color`, `view_pos_w`, `fresnel_params`) ✅
- Fresnel fragment logic is verbatim from the spec ✅

---

### T02 — `shaders/game/plasma_ball.glsl` ✅ PASS

Matches spec (Section 3.3.2) exactly.

- VS binding 0: standard 3-matrix block ✅
- VS output `v_obj_pos` (object-space position for noise) present ✅
- FS `plasma_ball_fs_params` has all five vec4 fields ✅
- Appendix A `hash31` / `vnoise` functions included verbatim ✅
- Fragment logic matches spec ✅

---

### T03 — renderer.cpp / renderer.h ⚠️ PASS WITH ISSUES

#### What is correct

- `PipelineCacheEntry pipeline_cache[64]` replaces the old named pipeline fields ✅
- Magenta fallback pre-inserted at index 0, `shader_id=0` sentinel ✅
- `renderer_create_shader` / `renderer_builtin_shader` implemented ✅
- `renderer_set_time` stores `state.current_time` ✅
- Generic draw loop with stable `render_queue` sort ✅
- BlinnPhong per-frame patching (light direction, view position) ✅
- Custom shaders: FS uniform blob forwarded verbatim ✅
- `pipeline_line_quad_additive` created with `ONE/ONE` blend, depth write off ✅
- `renderer_enqueue_line_quad` stores per-command blend mode ✅
- Convenience factories (`renderer_make_unlit_material` etc.) updated to use `material_set_uniforms<T>` ✅

#### Issues

**ISSUE T03-A — Unlit shader not updated to the 3-matrix VS convention**

Spec Section 3.2.2 states:

> Every mesh shader … **must** have VS binding 0 as `{ mat4 mvp; mat4 model; mat4 normal_mat; }`.
> Shaders that don't use `model`/`normal_mat` (unlit) still declare them; they are simply ignored.
> The renderer always sends this block at binding 0 for every mesh draw.

`shaders/renderer/unlit.glsl` still only declares `{ mat4 mvp; }` (64 bytes). The renderer branches on `shader.id == 1` and sends only `sizeof(mvp)` = 64 bytes to binding 0 for Unlit draws. The promised "renderer always sends 192 bytes at binding 0 for every mesh draw" invariant is violated for the built-in Unlit shader.

**ISSUE T03-B — `UnlitFSParams::flags` never reaches the GPU**

`UnlitFSParams` in `renderer.h` is 32 bytes (`color` + `flags`). However `apply_draw_uniforms` for `shader.id == 1` calls:

```cpp
sg_range r = { &fs->color, sizeof(fs->color) };  // 16 bytes only
sg_apply_uniforms(1, &r);
```

Only the `color` field is uploaded. Any `flags.x = use_texture` value written into a material blob via `material_set_uniforms<UnlitFSParams>` is silently discarded. The actual `unlit.glsl` FS only declares `vec4 base_color`, so behaviour is self-consistent at runtime. However, the `UnlitFSParams` struct as documented implies both fields are meaningful, and any future expansion of the unlit shader to read `flags` would break silently.

**ISSUE T03-C — Line-quad interleaved blend dispatch is logically incorrect**

`renderer_end_frame` (lines 926–948) dispatches additive vs. opaque line quads using `additive_start`/`additive_end` and `opaque_start`/`opaque_end` tracking variables. These variables record only the first and last index of each type, not contiguous ranges. If quads of different blend modes are interleaved in the queue (e.g. additive at 0, opaque at 1, additive at 2), the resulting `sg_draw` calls cover wrong index ranges, causing quads to be drawn with the wrong pipeline and potentially drawn twice.

*Current exposure:* The game only ever submits `BlendMode::Additive` line quads (from `laser_update` / `laser_charging`), so this bug does not manifest today. If any other system (debug overlays, HUD line segments, etc.) submits a non-additive line quad in the same frame, incorrect rendering will occur.

---

### T04 — `components.h` ✅ PASS

All four new components present with correct fields.

- `LaserCharge { charge_start, charge_time }` ✅
- `LaserBeam { fire_time, fire_duration, fade_duration, origin, end, last_hit_entity }` ✅
- `ShieldSphere { owner, radius }` ✅
- `VFXData { initial_scale, final_scale, initial_alpha, final_alpha, r, g, b }` ✅
- `WeaponState::laser_damage` renamed to `laser_dps`, initialized from `constants::laser_dps` ✅

---

### T05 — `constants.h` ⚠️ MINOR

All 16 new constants present with correct values.

| Constant | Expected | Actual |
|----------|----------|--------|
| `laser_charge_time` | 0.8f | 0.8f ✅ |
| `laser_fire_duration` | 3.0f | 3.0f ✅ |
| `laser_fade_duration` | 0.5f | 0.5f ✅ |
| `laser_dps` | 20.0f | 20.0f ✅ |
| `laser_impulse_per_second` | 30.0f | 30.0f ✅ |
| `laser_core_width` | 0.3f | 0.3f ✅ |
| `laser_halo_width` | 2.5f | 2.5f ✅ |
| `laser_core_color` | {1,1,0.9,1} | {1,1,0.9,1} ✅ |
| `laser_halo_color` | {0.3,0.85,1,1} | {0.3,0.85,1,1} ✅ |
| `shield_sphere_scale` | 1.45f | 1.45f ✅ |
| `shield_max_alpha` | 0.55f | 0.55f ✅ |
| `shield_fresnel_exp` | 3.0f | 3.0f ✅ |
| `shield_rim_intensity` | 1.2f | 1.2f ✅ |
| `plasma_impact_duration` | 0.5f | 0.5f ✅ |
| `laser_impact_duration` | 0.4f | 0.4f ✅ |
| `shield_impact_duration` | 0.25f | 0.25f ✅ |

**Issue:** `laser_damage = 10.0f` (line 48) remains alongside `laser_dps = 20.0f`. The spec called for renaming the field on `WeaponState` (done), but the old constant was not removed from `constants.h`. It is unreferenced dead code.

---

### T06 — `game_shaders.h` / `game_shaders.cpp` ✅ PASS

Matches spec (Section 3.4) exactly.

- `g_shield_shader` and `g_plasma_shader` declared as `extern RendererShaderHandle` ✅
- `game_shaders_init()` calls `renderer_create_shader` with the correct sokol-shdc descriptors (`shield_shader_desc`, `plasma_ball_shader_desc`) ✅

---

### T07 — `weapons.cpp` ⚠️ MINOR ISSUES

#### What is correct

- Old one-shot `laser_fire()` removed; replaced by `laser_update()` + `laser_charging()` ✅
- **Charging phase:** dim aiming beam, widths scaled by 0.5 / 0.7 per spec, opacity ramp, early-release cancel with no cooldown ✅
- **Firing phase:** raycast from `muzzle_origin` (ship forward + nose offset), `laser_dps * dt` damage, asteroid impulse, early-release sets `lb.fire_duration = fire_age` ✅
- Full-brightness opacity formula with flicker (`1 + 0.04 * sin(age * 25)`) matches spec ✅
- **Fading phase:** uses `lb.origin` for p0, `lb.end` stays fixed, removes `LaserBeam` when `fade_t >= 1.0` ✅
- `sphere_intersect_t` helper present and correct ✅
- `weapon_update()` detects right-mouse press-edge, adds `LaserCharge` when weapon is ready ✅
- `laser_charging` and `laser_update` called unconditionally each frame ✅

#### Issues

**ISSUE T07-A — Variable shadowing in `laser_update`**

At line 97, the firing phase declares:

```cpp
float t = sphere_intersect_t(muzzle_origin, ray_dir, target_t->position, shield_r);
```

This `float t` shadows the outer `auto& t = view.get<Transform>(e)` declared at the top of the loop. There is no functional bug (the Transform reference is not used after this inner block), but the compiler will emit `-Wshadow` warnings and the code is confusing to read.

**ISSUE T07-B — Dead code block in `weapon_update`**

Lines 285–287:

```cpp
if (!switch_plasma && !switch_laser && !fire_pressed) {
    // Still need to run laser_update/laser_charging even without new input.
}
```

This is an empty if-block that compiles to nothing. It is an artifact of a refactor and should be removed.

---

### T08 — `spawn.cpp` ✅ PASS

- `spawn_shield_sphere()` implemented per Section 3.7.2 ✅
- Uses `g_shield_shader`, `BlendMode::AlphaBlend`, `CullMode::Off`, `depth_write=false`, `render_queue=2` ✅
- `ShieldFSParams` filled with correct defaults (`shield_color`, `view_pos_w`, `fresnel_params`) ✅
- Called at end of `spawn_player()` with `k_player_half_extent` ✅
- Called at end of `spawn_enemy()` with `k_enemy_half_extent` ✅
- `spawn_projectile()` replaced with `g_plasma_shader` material using `PlasmaBallFSParams` per Section 3.8 ✅

---

### T09 — `vfx.h` / `vfx.cpp` ✅ PASS

- `PlasmaBallFSParams` struct declared in `vfx.h`, layout matches `plasma_ball.glsl` ✅
- `vfx_update()` iterates `Lifetime + VFXData + Transform + EntityMaterial` ✅
- Scale and alpha interpolated correctly via `material_uniforms_as<UnlitFSParams>` ✅
- All three spawn helpers present with impact parameters matching the spec table:

| Type | radius | lifetime | initial_scale | final_scale | initial_alpha | rgb |
|------|--------|----------|---------------|-------------|---------------|-----|
| Plasma | 1.0 | 0.5s | 1.0 | 4.0 | 0.9 | 1.0, 0.5, 0.1 ✅ |
| Laser | 1.0 | 0.4s | 0.5 | 3.5 | 1.0 | 1.0, 0.2, 0.05 ✅ |
| Shield | 1.0 | 0.25s | 1.5 | 3.0 | 0.9 | 0.5, 0.7, 1.0 ✅ |

---

### T10 — `shield_vfx.h` / `shield_vfx.cpp` ⚠️ MINOR

#### What is correct

- `ShieldFSParams` struct matches `shield_fs_params` in `shield.glsl` (3 × vec4) ✅
- `shield_vfx_update()` syncs position to owner, sets scale to `ss.radius`, resets rotation to identity ✅
- Patches `view_pos_w` and `shield_color.a` into the uniform blob each frame ✅
- Hides sphere (scale → 0) when `shield_alpha < 0.01` ✅
- Handles destroyed owner gracefully (scales sphere to 0) ✅

#### Issues

**ISSUE T10-A — Camera position is 1-frame stale**

`shield_vfx_update(dt)` runs at step 13b in `game_tick`, before `camera_rig_finalize(dt)` at step 14. The camera `Transform` read to populate `view_pos_w` therefore reflects the camera's position from the previous frame. The Fresnel effect will lag the true view direction by one tick. In practice the visual error is imperceptible at normal frame rates, but it is worth noting.

**ISSUE T10-B — Spec divergence: camera source**

Spec Section 3.7.3 says to read camera position from *"CameraRigState of player entity"*. The implementation instead queries `reg.view<Camera, Transform>()` to find the camera entity's Transform. Functionally equivalent, but the implementation diverges from the documented approach.

---

### T11 — `damage.cpp` + `weapons.cpp` ✅ PASS

**`damage.cpp`:**
- Shield boundary check uses sphere-overlap (`dist < proj_r + shield_r`) before AABB ✅
- `spawn_shield_impact(pt.position)` called on shield hit ✅
- `spawn_plasma_impact(pt.position)` called on hull hit ✅
- Projectile marked `DestroyPending` on first hit ✅

**`weapons.cpp` (laser firing path, T11's contribution):**
- Checks `Shield.current > 0` on hit entity ✅
- Computes shield surface hit point via `sphere_intersect_t()` when shielded ✅
- Sets `lb.end` to shield surface point ✅
- Spawns `spawn_shield_impact` or `spawn_laser_impact` correctly ✅
- VFX dedup via `lb.last_hit_entity` — spawns once per new target ✅

---

### T12 — `game.cpp` ✅ PASS

All integration wiring present and in correct order.

- `game_shaders_init()` called in `game_init()` before any spawn function ✅
- `renderer_set_time(engine_now())` is the first call in `game_tick()` (step 0) ✅
- `shield_vfx_update(dt)` called at step 13b ✅
- `vfx_update(dt)` called at step 13c ✅
- `weapon_update(dt)` called at step 7; internally calls `laser_charging` and `laser_update` unconditionally ✅

---

## Prioritized Fix List

| Priority | ID | Location | Description |
|----------|----|----------|-------------|
| **Should fix** | T03-C | `renderer.cpp` lines 926–948 | Line-quad interleaved blend dispatch will misrender if any non-additive line quad is ever submitted alongside additive ones. Replace start/end tracking with a sort pass or per-quad pipeline switch. |
| **Should fix** | T03-A | `shaders/renderer/unlit.glsl`, `renderer.cpp` | Unlit shader does not follow the 3-matrix VS binding 0 convention. Update `unlit.glsl` to declare `model` and `normal_mat` (ignored), and remove the `shader.id == 1` special-case in `apply_draw_uniforms`. |
| **Should fix** | T03-B | `renderer.cpp` `apply_draw_uniforms` | Send full `sizeof(UnlitFSParams)` = 32 bytes for Unlit FS uniforms, not just 16. Update `unlit.glsl` FS to declare the `flags` field so the struct contract is honoured. |
| **Cleanup** | T05 | `constants.h` line 48 | Remove dead `laser_damage = 10.0f` constant. |
| **Cleanup** | T07-A | `weapons.cpp` line 97 | Rename inner `float t` to `float shield_t` to remove shadowing. |
| **Cleanup** | T07-B | `weapons.cpp` lines 285–287 | Remove the empty `if`-block. |
| **Cleanup** | T10-A/B | `shield_vfx.cpp` | Move `shield_vfx_update` call to after `camera_rig_finalize` (step 14), or accept 1-frame lag as negligible. |
