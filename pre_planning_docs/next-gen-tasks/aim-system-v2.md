# Aim System V2 — Aiming, Damage Fix, and Combat UI

## Context

Camera Rig V2 is complete. The ship now has stable orientation via `CameraRigState.rig_rotation`. This plan fixes damage, replaces the crosshair, adds cursor-driven aiming, and adds combat UI (enemy target frames, health bars).

**Root causes of current issues:**
- Laser never hits: `engine_raycast()` filters on `Interactable`; enemies and asteroids lack that tag.
- Crosshair fixed at screen center: hardcoded `sapp_width()*0.5f` in `hud.cpp`.
- Weapons fire along ship nose (`t.rotation * vec3(0,1,0)`), not toward cursor.
- Laser beam renders only to first hit (no hit = zero-length or clipped beam).
- Plasma TTK ~30 seconds (0.5 dmg × 10/sec vs 150 HP+shield).

---

## Tick order reference (do not change)

```
1.  player_update(dt)
2.  camera_rig_input(dt)      ← cursor ray MUST be computed here (weapons fire at step 7)
3.  engine_tick(dt)
4.  containment_update()
5.  damage_resolve()
6.  enemy_ai_update(dt)
7.  weapon_update(dt)
8.  projectile_update(dt)
9.  enemy_death_update()
10. vfx_cleanup()
11. match_state_update(dt)
12. handle_restart_quit_input()
13. camera_rig_finalize(dt)    ← VP matrix becomes valid here (do NOT use for aiming)
14. render_submit()
15. hud_render()
```

**Critical constraint:** The cursor ray for aiming must be stored in step 2 (`camera_rig_input`) so it is available at step 7 (`weapon_update`). Computing it in `camera_rig_finalize` (step 13) is too late.

---

## Task 0 — Fix laser/projectile hit detection: add Interactable to enemies and asteroids

**Files:** `src/game/spawn.cpp`

**Problem:** `engine_raycast()` in `src/engine/raycast.cpp` iterates only entities with
`view<Transform, Collider, Interactable>()`. Enemies and asteroids are spawned without
`Interactable`, so the laser ray never hits anything.

**Fix:** Add `engine_registry().emplace<Interactable>(e)` to both `spawn_enemy()` and
`spawn_asteroid()` immediately after the other components are emplaced.

`Interactable` is defined in `src/engine/components.h` as `struct Interactable {};`.
Include `<engine.h>` (already present in spawn.cpp) — `Interactable` is re-exported
through the engine public header or include `components.h` directly if not.

Verify `src/engine/components.h` is already included (or add it). Do not add a new
include if `engine.h` already brings it in — check with a quick grep.

**Validation:** After building, fire the laser at an enemy. The enemy should take damage.

---

## Task 1 — Cache VP matrices and compute cursor ray in camera_rig_input()

**Files:** `src/game/camera_rig.cpp`, `src/game/camera_rig.h`

### 1a — Static state additions in camera_rig.cpp

Add these module-level statics alongside the existing ones (`s_camera`, `s_player`, etc.):

```cpp
static glm::mat4 s_view_mat  = glm::mat4(1.f);
static glm::mat4 s_proj_mat  = glm::mat4(1.f);
static glm::vec3 s_cursor_ray_origin = glm::vec3(0.f);
static glm::vec3 s_cursor_ray_dir    = glm::vec3(0.f, 1.f, 0.f);
```

### 1b — Compute cursor ray at the END of camera_rig_input()

At the very end of `camera_rig_input()`, after `rig_rotation` has been updated,
reconstruct the view and projection matrices and unproject the mouse position:

```cpp
// --- Cache VP and cursor ray for this frame ---
{
    auto& reg = engine_registry();
    const auto& cam_t  = reg.get<Transform>(s_camera);
    const auto& cam_c  = reg.get<Camera>(s_camera);

    const float w = sapp_widthf();
    const float h = sapp_heightf();

    s_view_mat = make_view_matrix(cam_t);
    s_proj_mat = glm::perspective(glm::radians(cam_c.fov),
                                  (h > 0.f ? w / h : 1.f),
                                  cam_c.near_plane, cam_c.far_plane);

    // Mouse position in [0,w] x [0,h], origin top-left.
    const glm::vec2 mouse = engine_mouse_position();

    // NDC: x in [-1,1] left-to-right, y in [-1,1] bottom-to-top (flip y).
    const float ndcX = (mouse.x / w) * 2.f - 1.f;
    const float ndcY = 1.f - (mouse.y / h) * 2.f;

    // Unproject through inverse(proj) then inverse(view).
    const glm::mat4 inv_vp = glm::inverse(s_proj_mat * s_view_mat);
    const glm::vec4 near_h = inv_vp * glm::vec4(ndcX, ndcY, -1.f, 1.f);
    const glm::vec4 far_h  = inv_vp * glm::vec4(ndcX, ndcY,  1.f, 1.f);
    const glm::vec3 near_w = glm::vec3(near_h) / near_h.w;
    const glm::vec3 far_w  = glm::vec3(far_h)  / far_h.w;

    s_cursor_ray_origin = near_w;
    s_cursor_ray_dir    = glm::normalize(far_w - near_w);
}
```

**Important:** At step 2, the camera Transform has NOT yet been updated by
`camera_rig_finalize()` (step 13). The camera Transform reflects the PREVIOUS frame's
position. This is one frame of latency in the cursor ray, which is imperceptible at
60 fps. Do NOT restructure the tick order to eliminate this — it is not worth the
complexity.

**Required headers in camera_rig.cpp** (add only what is missing):
- `#include <sokol_app.h>` — for `sapp_widthf()`, `sapp_heightf()`
- `#include "components.h"` (game) — for `Camera` if not already present
- `#include <engine.h>` — already present

The engine's `make_view_matrix()` is in `src/engine/math_utils.h` — include it or use
`glm::inverse(make_model_matrix(cam_t))` if `make_view_matrix` is not exposed.

### 1c — New public API in camera_rig.h

```cpp
// Returns the world-space ray origin and direction corresponding to the
// current mouse cursor position. Computed in camera_rig_input(); valid
// from step 2 onward (used by weapons at step 7).
void camera_rig_cursor_ray(glm::vec3& out_origin, glm::vec3& out_dir);

// Returns the cached view and projection matrices from this frame's
// camera_rig_input() call. Used by hud.cpp for world-to-screen projection.
void camera_rig_get_vp(glm::mat4& out_view, glm::mat4& out_proj);
```

Implement both as trivial getters from the static state:

```cpp
void camera_rig_cursor_ray(glm::vec3& out_origin, glm::vec3& out_dir) {
    out_origin = s_cursor_ray_origin;
    out_dir    = s_cursor_ray_dir;
}
void camera_rig_get_vp(glm::mat4& out_view, glm::mat4& out_proj) {
    out_view = s_view_mat;
    out_proj = s_proj_mat;
}
```

**Update `camera_rig_aim_point()`** — it is currently used by `laser_fire()`. After Task 3
changes weapons to use `camera_rig_cursor_ray()`, this function can remain for backward
compatibility but its internal point (player_pos + rig_fwd * cam_look_ahead + rig_up *
cam_look_up_bias) is no longer used for weapons. No change needed unless it causes a
compile error.

---

## Task 2 — Cursor-following crosshair (circle + dot, OS cursor hidden)

**Files:** `src/game/hud.cpp`, `src/game/game.cpp`

### 2a — Hide the OS cursor on game start

In `game_init()` in `src/game/game.cpp`, after the player is spawned and before the
first frame:

```cpp
sapp_show_mouse(false);
sapp_lock_mouse(false);   // keep lock OFF — we need absolute cursor position
```

Do NOT call `sapp_lock_mouse(true)` — locking confines the cursor and removes absolute
position. We need the cursor to move freely around the screen.

On death/victory overlay or pause (if added later), restore with `sapp_show_mouse(true)`.

### 2b — Replace the existing crosshair in hud_render()

Remove the four `dl->AddLine(...)` calls for the existing static crosshair.

Replace with:

```cpp
// --- Cursor-following crosshair ---
const glm::vec2 mpos   = engine_mouse_position();
const float     mx     = mpos.x;
const float     my     = mpos.y;
auto*           dl     = ImGui::GetForegroundDrawList();

const float ring_r     = 14.f;   // outer circle radius
const float dot_r      = 2.5f;   // center dot radius
const ImU32 col_normal = IM_COL32(255, 255, 255, 210);
const ImU32 col_enemy  = IM_COL32(255,  50,  50, 230);
const ImU32 col        = s_crosshair_on_enemy ? col_enemy : col_normal;

dl->AddCircle    (ImVec2(mx, my), ring_r, col, 32, 1.5f);
dl->AddCircleFilled(ImVec2(mx, my), dot_r,  col);
```

The `s_crosshair_on_enemy` flag is defined and set in Task 5.

**Note on HiDPI:** `engine_mouse_position()` returns physical pixels on HiDPI displays.
`ImGui::GetIO().DisplaySize` returns logical pixels. If the crosshair appears offset,
divide `mpos` by `sapp_dpi_scale()` before using. Test on the target machine — Ubuntu
24.04 with X11 typically has `dpi_scale == 1.0`, so this is usually a no-op.

---

## Task 3 — Weapons fire along cursor ray

**Files:** `src/game/weapons.cpp`

### 3a — laser_fire()

Current (post camera-rig-V2):
```cpp
const glm::vec3 aim_point = camera_rig_aim_point();
const glm::vec3 forward   = glm::normalize(aim_point - t.position);
const auto hit = engine_raycast(origin, forward, constants::laser_max_range);
```

Replace with:
```cpp
glm::vec3 ray_origin, ray_dir;
camera_rig_cursor_ray(ray_origin, ray_dir);
// Fire along cursor ray; use player position as origin offset along ray
// (project player onto the ray to avoid shooting backward if cursor is behind player).
const glm::vec3 origin = t.position + glm::vec3(0.f, 0.f, 0.f); // keep existing origin offset
const auto hit = engine_raycast(origin, ray_dir, constants::laser_max_range);
```

The hit-point computation for the beam end point follows in Task 4.

Add `#include "camera_rig.h"` to `weapons.cpp` if not already present.

### 3b — plasma_fire()

Current: fires along ship forward `t.rotation * vec3(0,1,0)`.

Replace:
```cpp
glm::vec3 ray_origin, ray_dir;
camera_rig_cursor_ray(ray_origin, ray_dir);
// Spawn projectile from ship position, velocity along cursor ray.
rb.linear_velocity = ray_dir * constants::plasma_speed;
```

Keep the existing spawn offset (the projectile is spawned slightly in front of the ship
nose). Use `ray_dir` for the initial velocity direction; keep the spawn position as
`t.position + rig_forward * spawn_offset` (use `CameraRigState.rig_rotation * vec3(0,1,0)`
for rig_forward).

---

## Task 4 — Laser beam visual: always render to max range (pass-through look)

**Files:** `src/game/weapons.cpp` (or wherever the laser VFX line is drawn)

The laser should visually reach its full range regardless of whether it hit something.
This gives the "passes through enemy" aesthetic described in the requirements.

Find where the laser beam endpoint is computed (likely in `weapon_update()` or a
`laser_vfx` draw call). Change:

```cpp
// Before: beam ends at hit point or disappears if no hit
const glm::vec3 beam_end = hit.has_value() ? hit->point : origin + ray_dir * constants::laser_max_range;
```

To always use max range:
```cpp
const glm::vec3 beam_end = origin + ray_dir * constants::laser_max_range;
```

Damage is still only applied to the first entity along the ray (the `engine_raycast` hit
result). The visual decouples from the damage volume.

If the laser beam is rendered as a line via the debug draw system or a VFX component,
update that component's `end_point` field to always be `origin + ray_dir * laser_max_range`.

---

## Task 5 — Crosshair turns red when cursor ray intersects an enemy

**Files:** `src/game/hud.cpp`

### 5a — Static flag

At the top of `hud.cpp` (module-level):

```cpp
static bool s_crosshair_on_enemy = false;
```

### 5b — Hit test in hud_render(), before drawing crosshair

Perform a ray-AABB slab test against all EnemyTag entities. This is CPU-only (no
`engine_raycast` call — that would add Interactable noise and is overkill for UI):

```cpp
// --- Crosshair enemy hit test ---
{
    glm::vec3 ray_o, ray_d;
    camera_rig_cursor_ray(ray_o, ray_d);

    s_crosshair_on_enemy = false;
    auto& reg = engine_registry();
    auto ev = reg.view<EnemyTag, Transform, Collider>();
    for (auto ee : ev) {
        const auto& et = ev.get<Transform>(ee);
        const auto& ec = ev.get<Collider>(ee);

        // Ray-AABB slab test.
        const glm::vec3 inv_d = 1.f / ray_d;   // component-wise
        const glm::vec3 t_min = (et.position - ec.half_extents - ray_o) * inv_d;
        const glm::vec3 t_max = (et.position + ec.half_extents - ray_o) * inv_d;
        const glm::vec3 t1    = glm::min(t_min, t_max);
        const glm::vec3 t2    = glm::max(t_min, t_max);
        const float t_near    = glm::compMax(t1);
        const float t_far     = glm::compMin(t2);
        if (t_far >= t_near && t_far > 0.f) {
            s_crosshair_on_enemy = true;
            break;
        }
    }
}
```

Required headers: `#include "camera_rig.h"`, `#include <glm/gtx/component_wise.hpp>`.
`glm::compMax` and `glm::compMin` come from `gtx/component_wise.hpp`. If that extension
header is not available, replace with:

```cpp
const float t_near = std::max({t1.x, t1.y, t1.z});
const float t_far  = std::min({t2.x, t2.y, t2.z});
```

Place the hit test block BEFORE the crosshair draw block so `s_crosshair_on_enemy` is
set before the color is chosen.

---

## Task 6 — Enemy target frame and health bars

**Files:** `src/game/hud.cpp`

### 6a — World-to-screen helper (local to hud.cpp)

```cpp
// Returns false if the point is behind the camera (w <= 0 after clip).
static bool world_to_screen(const glm::vec3& world_pos,
                             const glm::mat4& vp,
                             float screen_w, float screen_h,
                             ImVec2& out_screen) {
    const glm::vec4 clip = vp * glm::vec4(world_pos, 1.f);
    if (clip.w <= 0.f)
        return false;
    const glm::vec3 ndc = glm::vec3(clip) / clip.w;
    out_screen.x = (ndc.x + 1.f) * 0.5f * screen_w;
    out_screen.y = (1.f - ndc.y) * 0.5f * screen_h;  // Y flipped
    return true;
}
```

Call `camera_rig_get_vp(view, proj)` and compute `vp = proj * view` once at the top of
`hud_render()`, before the target frame loop.

### 6b — Target frame and health bar loop

Add this block after the weapon info window and before the crosshair:

```cpp
// --- Enemy target frames ---
{
    glm::mat4 view, proj;
    camera_rig_get_vp(view, proj);
    const glm::mat4 vp      = proj * view;
    const float     screen_w = sapp_widthf();
    const float     screen_h = sapp_heightf();

    auto& reg = engine_registry();
    auto ev = reg.view<EnemyTag, Transform, Collider, Health>();

    // Optional Shield — use try_get.
    for (auto ee : ev) {
        const auto& et  = ev.get<Transform>(ee);
        const auto& ec  = ev.get<Collider>(ee);
        const auto& ehp = ev.get<Health>(ee);
        const auto* esh = reg.try_get<Shield>(ee);

        ImVec2 screen_center;
        if (!world_to_screen(et.position, vp, screen_w, screen_h, screen_center))
            continue;

        // Compute screen-space half-size from AABB half_extents (use largest axis).
        // Project a point offset by half-extents and measure pixel distance.
        ImVec2 screen_edge;
        const float half = glm::compMax(ec.half_extents);
        // Use the right-ward offset point.
        glm::vec4 clip_e = vp * glm::vec4(et.position + glm::vec3(half, 0.f, 0.f), 1.f);
        float px_half = 0.f;
        if (clip_e.w > 0.f) {
            float ex = ((clip_e.x / clip_e.w + 1.f) * 0.5f) * screen_w;
            px_half = std::abs(ex - screen_center.x);
        }
        if (px_half < 4.f) px_half = 4.f;   // minimum bracket size

        // Only draw target frame if enemy is between 3% and 20% of screen height.
        const float frac = px_half * 2.f / screen_h;
        if (frac < 0.03f || frac > 0.20f)
            continue;

        // Bracket corners.
        const float b  = px_half * 1.3f;   // bracket outer size
        const float bl = px_half * 0.4f;   // bracket leg length
        const ImU32 bc = IM_COL32(255, 220, 60, 220);
        auto* dl = ImGui::GetForegroundDrawList();

        // Top-left corner bracket
        const float x0 = screen_center.x - b, y0 = screen_center.y - b;
        const float x1 = screen_center.x + b, y1 = screen_center.y + b;
        dl->AddLine({x0,      y0},      {x0 + bl, y0},      bc, 1.5f);
        dl->AddLine({x0,      y0},      {x0,      y0 + bl}, bc, 1.5f);
        dl->AddLine({x1 - bl, y0},      {x1,      y0},      bc, 1.5f);
        dl->AddLine({x1,      y0},      {x1,      y0 + bl}, bc, 1.5f);
        dl->AddLine({x0,      y1 - bl}, {x0,      y1},      bc, 1.5f);
        dl->AddLine({x0,      y1},      {x0 + bl, y1},      bc, 1.5f);
        dl->AddLine({x1,      y1 - bl}, {x1,      y1},      bc, 1.5f);
        dl->AddLine({x1,      y1},      {x1 - bl, y1},      bc, 1.5f);

        // Health bar below bracket
        const float bar_w = b * 2.f;
        const float bar_h = 5.f;
        const float bx    = screen_center.x - b;
        const float by    = y1 + 6.f;
        const float hp_f  = ehp.current / ehp.max;
        dl->AddRectFilled({bx,          by}, {bx + bar_w,          by + bar_h},
                          IM_COL32(60, 10, 10, 180));
        dl->AddRectFilled({bx,          by}, {bx + bar_w * hp_f,   by + bar_h},
                          IM_COL32(220, 40, 40, 220));

        // Shield bar (above HP bar, if entity has Shield)
        if (esh && esh->max > 0.f) {
            const float sy   = by - bar_h - 2.f;
            const float sh_f = esh->current / esh->max;
            dl->AddRectFilled({bx,        sy}, {bx + bar_w,        sy + bar_h},
                              IM_COL32(10, 10, 60, 180));
            dl->AddRectFilled({bx,        sy}, {bx + bar_w * sh_f, sy + bar_h},
                              IM_COL32(60, 80, 220, 220));
        }
    }
}
```

Required headers: `#include "camera_rig.h"`, `#include <glm/gtx/component_wise.hpp>`
(for `glm::compMax`), or replace `glm::compMax` with `std::max({...})`.

---

## Task 7 — Tuning constants

**Files:** `src/game/constants.h`

Change these values:

| Constant | Old | New | Reason |
|---|---|---|---|
| `plasma_speed` | 200.0f | 80.0f | Lead-shooting requires slow enough projectile to miss if unaimed |
| `plasma_damage` | 0.5f | 3.0f | 30s TTK → ~5s TTK at 10 shots/sec with 150 HP+shield total |

Laser damage is instant and raycast-based; its per-hit value is already reasonable once
it actually hits (Task 0 fix). Verify laser TTK manually after Task 0.

No other constants need changing for this plan. Boost, shield regen, and collision damage
were validated by camera-rig-V2.

---

## Implementation order

Tasks are ordered by dependency. Do them in sequence:

```
0 → 1 → 3 → 2 → 4 → 5 → 6 → 7
```

Rationale:
- Task 0 is a prerequisite for all damage to work (tests become meaningful).
- Task 1 must precede Tasks 3 and 5 (cursor ray API).
- Task 3 fires along cursor ray (needs Task 1).
- Task 2 places the crosshair at cursor (can be done in parallel with 3, but 5 modifies hud.cpp too, so do 2 before 5 to avoid conflicts).
- Task 4 is independent of 5 and 6 (same file as 3 but different code block — safe in sequence).
- Task 5 modifies hud.cpp and reads the cursor ray (needs Task 1).
- Task 6 modifies hud.cpp (needs Task 1 for VP matrices).
- Task 7 is always last (pure constant tweak, rebuild and test).

---

## Build command

```bash
cmake --build build --target game
```

Run target `game` only — this plan touches `src/game/` exclusively (plus one line in
`src/game/spawn.cpp` which is game-layer code). Do NOT rebuild the full tree unless a
milestone merge is happening.

---

## Acceptance criteria

- [ ] Firing the laser at a stationary enemy destroys it (Task 0 + 3).
- [ ] Laser beam is always visible to max range (Task 4).
- [ ] Crosshair follows the mouse cursor (Task 2).
- [ ] Crosshair ring turns red when the mouse is over an enemy (Task 5).
- [ ] Enemy target brackets appear when an enemy is in the 3%-20% screen size range (Task 6).
- [ ] HP and shield bars are visible below/above the bracket (Task 6).
- [ ] Plasma projectile travels visibly slowly and misses if unaimed at a moving target (Task 7).
- [ ] OS cursor is hidden while in-game (Task 2).
