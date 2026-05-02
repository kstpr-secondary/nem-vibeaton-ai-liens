# Camera Rig V2 — Implementation Plan

**Status**: Drafted 2026-05-02. Ready for task claims.  
**Scope**: `src/game/camera_rig.h`, `camera_rig.cpp`, `player.cpp`, `game.cpp`, `components.h`, `constants.h`, `damage.cpp`, `weapons.cpp`, `hud.cpp`, `spawn.cpp`.  
**Branch**: `feature/game`

---

## 1. What We Are Fixing and Why

The current implementation has these confirmed bugs:

**Bug A — Camera breaks during vertical flight.**  
`camera_rig.cpp` calls `glm::lookAt(..., glm::vec3(0,1,0))` using hard-coded world-Y as the up vector. When the ship's forward direction gets close to world-Y (flying up or down steeply), the cross-product inside `lookAt` approaches zero. The camera flips violently.

**Bug B — Camera offset ignores ship orientation.**  
The camera's "above the ship" offset is `+ glm::vec3(0,f,constants::cam_offset_up,0.f)` — pure world-Y regardless of where the ship is pointing. When the ship pitches 90°, the camera ends up to the side, not above the cockpit.

**Bug C — Roll is stripped before rendering.**  
`strip_player_roll()` in `game.cpp` overwrites `t.rotation` every frame before `render_submit()`. The ship model never banks visually, not even during steering or after collisions.

**Bug D — Steering input modifies `Transform.rotation` directly.**  
`player.cpp` mouse code writes into `t.rotation`. The physics system, the camera, and the weapon system all then read an entangled quaternion that mixes "where the ship is trying to go" with "what physics has kicked it to". They need to be separate.

**Bug E — Weapons always fire along ship's nose, not toward the crosshair.**  
With the ship centered on screen, the player can't aim — the nose always points where the camera looks. The fix: fire toward the camera's look-target (a world-space point ahead of the ship), which becomes the natural crosshair.

**Bug F — Ship is screen-center, not lower-center.**  
The camera looks directly at the ship's position. In Freelancer the ship sits in the lower-center portion of the screen with the crosshair above it, giving more visibility ahead. This is achieved by offsetting the camera's look-target forward and slightly up.

**What can be salvaged without change:**
- Model loading, `k_ship_base_orientation`, scale in `spawn.cpp` — the ship visually faces forward with cockpit up. Do not change these.
- WASD thrust + strafe math in `player.cpp` (keep, but change the axis source — see Task 3).
- Shield/boost regen in `player.cpp` — untouched.
- Camera entity creation in `game_init()`.
- `engine_set_active_camera()` call.
- `damage_resolve()` structure in `damage.cpp` — extend it, don't rewrite.

---

## 2. Canonical Coordinate Convention

**Do not change this. Do not re-derive it. Just use it.**

In ship-local space, after `k_ship_base_orientation` is applied at spawn:

| Axis | World direction at rest | Meaning |
|---|---|---|
| Model `+Y` | World `-Z` (forward) | Ship nose, thrust direction |
| Model `-X` | World `-X` (left→right corrected) | Ship right |
| Model `-Z` | World `+Y` (up) | Cockpit-up / pilot's "sky" |

Code convention throughout: `forward = t.rotation * vec3(0,1,0)`, `right = t.rotation * vec3(-1,0,0)`, `cockpit_up = t.rotation * vec3(0,0,-1)`.

The **rig_rotation** introduced in this plan uses the same convention. `rig_forward = rig_rotation * vec3(0,1,0)`.

---

## 3. New Architecture

### 3.1 Core Principle

Maintain a `rig_rotation` quaternion that represents the ship's **travel direction** (yaw + pitch only, no roll). This is the master orientation state.

- **Camera** is always positioned and aimed using `rig_rotation`.
- **Thrust and velocity alignment** use `rig_forward` = `rig_rotation * vec3(0,1,0)`.
- **`Transform.rotation`** = `rig_rotation * roll_quat(visual_bank + collision_roll)`. This drives rendering only.
- Roll (`visual_bank`, `collision_roll`) is **never** baked into `rig_rotation`.

`roll_quat(angle)` = `glm::angleAxis(angle, vec3(0,1,0))` — rotation around the forward axis in model space.

### 3.2 New Component: CameraRigState

Added to `components.h`. Attached to the **player entity** (not the camera entity).

```cpp
struct CameraRigState {
    glm::quat rig_rotation      = glm::quat(1.f, 0.f, 0.f, 0.f);
    float     visual_bank       = 0.f;  // radians, transient — springs to 0
    float     collision_roll    = 0.f;  // radians, from last hit — springs to 0
    float     collision_roll_vel = 0.f; // for spring integration
    float     collision_roll_impulse = 0.f; // written by damage_resolve(), cleared each frame
};
```

### 3.3 Tick Order (must match exactly — do not reorder)

```
game_tick(dt):
  1. player_update(dt)          — WASD thrust/strafe only; NO rotation code
  2. camera_rig_input(dt)       — NEW: mouse → rig_rotation; writes t.rotation = rig_rotation * roll_quat
  3. engine_tick(dt)            — physics integration + collision resolution
  4. damage_resolve()           — writes collision_roll_impulse into CameraRigState
  5. containment_update()
  6. enemy_ai_update(dt)
  7. weapon_update(dt)
  8. projectile_update(dt)
  9. enemy_death_update()
 10. vfx_cleanup()
 11. match_state_update(dt)
 12. handle_restart_quit_input()
 13. camera_rig_finalize(dt)    — NEW: overwrite t.rotation (undo physics bake), spring bank/roll, position camera
 14. render_submit()
 15. hud_render()
```

`rb.angular_velocity = glm::vec3(0.f)` stays in `player_update()`. Collision rotation is expressed as a **visual** `collision_roll` float driven by `damage_resolve()`, not from physics integration.

---

## 4. Constants to Add

Add these to `src/game/constants.h` under a new section `// Camera rig V2`:

```cpp
constexpr float  cam_turn_rate          = 1.8f;   // rad/sec per unit of mouse delta (replaces player_turn_speed for yaw/pitch)
constexpr float  cam_look_ahead         = 25.0f;  // units ahead of ship that camera aims at
constexpr float  cam_look_up_bias       = 8.0f;   // units above look-ahead point (ships ship toward screen bottom)
constexpr float  visual_bank_gain       = 0.35f;  // radians of bank per pixel/sec of lateral steering
constexpr float  visual_bank_max        = 0.55f;  // radians (≈31°) maximum bank angle
constexpr float  visual_bank_spring     = 5.0f;   // spring rate for bank return (rad/sec²/rad)
constexpr float  collision_roll_gain    = 0.008f; // radians of roll per unit of KE impulse
constexpr float  collision_roll_max     = 1.0f;   // radians max collision roll
constexpr float  collision_roll_spring  = 3.5f;   // spring rate for collision roll return
```

Keep `cam_offset_back = 30.0f`, `cam_offset_up = 10.0f`, `cam_lag_factor = 8.0f` unchanged.

Remove (no longer used): `player_turn_speed`.

---

## 5. Detailed Tasks

Each task must leave the build green before the next task begins. Do not merge tasks.

---

### Task 0 — Switch player/enemy ship model to spaceship1_mod2.glb

**File**: `src/game/spawn.cpp`, line 11  
**Change**: `static constexpr const char* k_player_model = "spaceship1.glb";`  
→ `static constexpr const char* k_player_model = "spaceship1_mod2.glb";`  
Do the same for `k_enemy_model` on line 12 if the user confirms enemy should also update (check with user; if uncertain, change only player model).

**Why**: `assets/spaceship1_mod2.glb` exists and the user explicitly referenced it. The code still points to `spaceship1.glb`. This is a one-line fix that must happen first so all subsequent visual testing uses the correct model.

**Verification**: Build and run. Confirm the ship model changed visually.

---

### Task 1 — Add CameraRigState component and new constants

**Files**: `src/game/components.h`, `src/game/constants.h`

In `components.h`, add after the existing `Boost` struct:

```cpp
struct CameraRigState {
    glm::quat rig_rotation           = glm::quat(1.f, 0.f, 0.f, 0.f);
    float     visual_bank            = 0.f;
    float     collision_roll         = 0.f;
    float     collision_roll_vel     = 0.f;
    float     collision_roll_impulse = 0.f;  // written by damage_resolve, read+cleared by camera_rig_finalize
};
```

In `constants.h`, add the new constants listed in Section 4.  
Remove `player_turn_speed` from `constants.h`.

**Verification**: `cmake --build build --target game` with 0 errors. No behavior change yet.

---

### Task 2 — Rewrite camera_rig.h and camera_rig.cpp

**Files**: `src/game/camera_rig.h`, `src/game/camera_rig.cpp`

**New `camera_rig.h`**:
```cpp
#pragma once
#include <entt/entt.hpp>

void camera_rig_init(entt::entity camera_entity);

// Call from game_tick step 2: reads mouse input, updates rig_rotation,
// writes t.rotation = rig_rotation * roll_quat. Does NOT move the camera yet.
void camera_rig_input(float dt);

// Call from game_tick step 13: overwrites t.rotation (undoes physics bake),
// springs visual_bank and collision_roll, positions and orients the camera.
void camera_rig_finalize(float dt);
```

**`camera_rig.cpp` — full implementation spec:**

```
Static state (file-scope):
  entt::entity  s_camera       = entt::null;
  entt::entity  s_player       = entt::null;  // cached in camera_rig_init
  glm::vec3     s_cam_position = {0,0,0};     // smoothed camera position (for lag)

camera_rig_init(cam_e):
  s_camera = cam_e;
  s_player = find player entity (reg.view<PlayerTag, Transform, CameraRigState>)
  auto& crs = player's CameraRigState
  crs.rig_rotation = player Transform.rotation   // copy initial orientation
  engine_set_active_camera(s_camera);

camera_rig_input(dt):
  if s_player == null: return
  auto& t   = player Transform
  auto& crs = player CameraRigState
  
  // --- Steering: mouse delta drives yaw and pitch rates ---
  // LMB held: steer (delta-driven)
  // No LMB: no steering (free look is not implemented; mouse does nothing)
  if engine_mouse_button(0):
    glm::vec2 delta = engine_mouse_delta()
    
    // Yaw: rotate rig around rig_up axis (model -Z = cockpit-up)
    glm::vec3 rig_up = crs.rig_rotation * glm::vec3(0.f, 0.f, -1.f)
    glm::quat q_yaw = glm::angleAxis(-delta.x * constants::cam_turn_rate * dt, rig_up)
    
    // Pitch: rotate rig around rig_right axis (model -X = right)
    glm::vec3 rig_right = crs.rig_rotation * glm::vec3(-1.f, 0.f, 0.f)
    glm::quat q_pitch = glm::angleAxis(-delta.y * constants::cam_turn_rate * dt, rig_right)
    
    // Apply yaw first, then pitch; both in world space (pre-multiply)
    crs.rig_rotation = glm::normalize(q_yaw * q_pitch * crs.rig_rotation)
  
  // Write ship Transform — no roll yet, just rig orientation
  t.rotation = crs.rig_rotation;

camera_rig_finalize(dt):
  if s_player == null or s_camera == null: return
  auto& t   = player Transform
  auto& crs = player CameraRigState
  
  // --- Step A: spring visual_bank toward steering-based target ---
  float bank_target = 0.f
  if engine_mouse_button(0):
    bank_target = -engine_mouse_delta().x * constants::visual_bank_gain
    bank_target = glm::clamp(bank_target, -constants::visual_bank_max, constants::visual_bank_max)
  crs.visual_bank += (bank_target - crs.visual_bank) * constants::visual_bank_spring * dt
  
  // --- Step B: spring collision_roll toward 0 (damped spring) ---
  // Accumulate any new impulse from damage_resolve
  if crs.collision_roll_impulse != 0.f:
    float sign = (crs.collision_roll_impulse > 0.f) ? 1.f : -1.f
    float mag = glm::min(glm::abs(crs.collision_roll_impulse) * constants::collision_roll_gain,
                         constants::collision_roll_max)
    crs.collision_roll += sign * mag
    crs.collision_roll = glm::clamp(crs.collision_roll,
                                    -constants::collision_roll_max,
                                    constants::collision_roll_max)
    crs.collision_roll_impulse = 0.f
  // Spring decay: acceleration = -spring * position - damping * velocity
  const float damping = 2.f * glm::sqrt(constants::collision_roll_spring)  // critical damping
  crs.collision_roll_vel += (-constants::collision_roll_spring * crs.collision_roll
                              - damping * crs.collision_roll_vel) * dt
  crs.collision_roll += crs.collision_roll_vel * dt
  
  // --- Step C: overwrite t.rotation (undoes any physics-baked rotation) ---
  float total_roll = crs.visual_bank + crs.collision_roll
  glm::quat roll_quat = glm::angleAxis(total_roll, glm::vec3(0.f, 1.f, 0.f))
  t.rotation = crs.rig_rotation * roll_quat
  
  // --- Step D: compute camera position (smoothed) ---
  glm::vec3 rig_fwd = crs.rig_rotation * glm::vec3(0.f, 1.f, 0.f)
  glm::vec3 rig_up  = crs.rig_rotation * glm::vec3(0.f, 0.f, -1.f)
  
  glm::vec3 desired_cam_pos = t.position
      - rig_fwd * constants::cam_offset_back
      + rig_up  * constants::cam_offset_up
  float alpha = glm::clamp(constants::cam_lag_factor * dt, 0.f, 1.f)
  s_cam_position = glm::mix(s_cam_position, desired_cam_pos, alpha)
  
  // --- Step E: position camera; aim toward look-target above and ahead of ship ---
  // The look-target is in front of and above the ship in rig space.
  // This makes the ship appear in the lower-center portion of the screen.
  glm::vec3 look_target = t.position
      + rig_fwd * constants::cam_look_ahead
      + rig_up  * constants::cam_look_up_bias
  
  auto& cam_t = camera Transform
  cam_t.position = s_cam_position
  
  glm::vec3 to_target = look_target - s_cam_position
  if glm::dot(to_target, to_target) > 1e-6f:
    // Build view matrix with rig_up as camera up — no gimbal-lock risk because
    // rig_up is derived from rig_rotation, not from world-Y directly.
    // Safety: if rig_up and to_target are nearly parallel, fall back to rig_right.
    glm::vec3 cam_up = rig_up
    if glm::abs(glm::dot(glm::normalize(to_target), rig_up)) > 0.99f:
      cam_up = crs.rig_rotation * glm::vec3(-1.f, 0.f, 0.f)  // rig_right as fallback
    glm::mat4 view = glm::lookAt(s_cam_position, look_target, cam_up)
    cam_t.rotation = glm::quat_cast(glm::transpose(glm::mat3(view)))
  
  engine_set_active_camera(s_camera)
```

**Important note on `cam_turn_rate`:** The constant is in rad/sec, but `engine_mouse_delta()` returns pixels moved since last frame (not per-second). Multiply by `dt` when applying (as shown above: `-delta.x * cam_turn_rate * dt`). The old `player_turn_speed` did NOT multiply by dt — the new code does. Tune `cam_turn_rate` accordingly (start at `1.8`).

**Verification**: Build and run. Ship visually banks during turns. Camera no longer flips when flying up or down. Camera follows behind ship with cockpit-up stable. Ship appears in lower-center of screen.

---

### Task 3 — Thin player.cpp: remove rotation code, use rig_forward for thrust

**File**: `src/game/player.cpp`

**Remove entirely** (lines 23–43 in current file): the `if (engine_mouse_button(0)) { ... }` block that modifies `t.rotation`. Steering now happens in `camera_rig_input()`.

**Remove entirely** (lines 52–75 in current file): the velocity alignment block (`const glm::vec3 facing = ...`). Velocity alignment is replaced by always thrusting along `rig_forward`.

**Change the forward/right computation for thrust** (lines 80–93 in current file):

Old:
```cpp
const glm::vec3 forward = t.rotation * glm::vec3(0.f, 1.f, 0.f);
const glm::vec3 right   = t.rotation * glm::vec3(-1.f, 0.f, 0.f);
```
New:
```cpp
// Use rig_rotation for thrust direction — not t.rotation, which now includes visual bank.
auto& crs = view.get<CameraRigState>(e);
const glm::vec3 forward = crs.rig_rotation * glm::vec3(0.f, 1.f, 0.f);
const glm::vec3 right   = crs.rig_rotation * glm::vec3(-1.f, 0.f, 0.f);
```

Add `CameraRigState` to the view query at the top of `player_update()`:
```cpp
auto view = reg.view<PlayerTag, Transform, RigidBody, Boost, Shield, CameraRigState>();
```

Keep the `rb.angular_velocity = glm::vec3(0.f)` line — it stays.

**Verification**: Build and run. Ship steers from `camera_rig_input()` (it's now connected via rig_rotation). WASD still thrusts and strafes. No velocity drift toward old facing direction after a turn.

---

### Task 4 — Update game.cpp tick order

**File**: `src/game/game.cpp`

**Remove** `strip_player_roll()` function entirely (lines 49–88).

**Remove** the `clean_forward` variable and its use:
```cpp
const glm::vec3 clean_forward = strip_player_roll();
camera_rig_update(dt, clean_forward);
```

**Replace** `camera_rig_update(dt, clean_forward)` with the two new calls in the correct positions:

New `game_tick()` body (add/move only these lines; keep all other calls unchanged):
```cpp
void game_tick(float dt) {
    player_update(dt);
    camera_rig_input(dt);       // NEW — after player_update, before engine_tick
    engine_tick(dt);
    containment_update();
    enemy_ai_update(dt);
    weapon_update(dt);
    projectile_update(dt);
    damage_resolve();
    enemy_death_update();
    vfx_cleanup();
    match_state_update(dt);
    handle_restart_quit_input();
    camera_rig_finalize(dt);    // NEW — replaces old camera_rig_update
    render_submit();
    hud_render();
}
```

**Update `game_init()`**: After `spawn_player(...)`, add:
```cpp
auto player_view = engine_registry().view<PlayerTag, Transform, CameraRigState>();
if (!player_view.empty()) {
    auto& crs = player_view.get<CameraRigState>(*player_view.begin());
    auto& pt  = player_view.get<Transform>(*player_view.begin());
    crs.rig_rotation = pt.rotation;
    s_cam_position   = pt.position - (pt.rotation * glm::vec3(0,1,0)) * constants::cam_offset_back
                       + (pt.rotation * glm::vec3(0,0,-1)) * constants::cam_offset_up;
}
```
(The `s_cam_position` static variable is now in `camera_rig.cpp`; initialize it from `camera_rig_init()` instead if that's cleaner — see below.)

**Also update `camera_rig_init()`**: It should attach `CameraRigState` to the player entity if not already present:
```cpp
void camera_rig_init(entt::entity camera_entity) {
    s_camera = camera_entity;
    auto& reg = engine_registry();
    auto pv = reg.view<PlayerTag, Transform, CameraRigState>();
    if (!pv.empty()) {
        s_player = *pv.begin();
        auto& crs = pv.get<CameraRigState>(s_player);
        crs.rig_rotation = pv.get<Transform>(s_player).rotation;
        s_cam_position = pv.get<Transform>(s_player).position;
    }
    engine_set_active_camera(s_camera);
}
```

This requires `CameraRigState` to be added to the player entity in `spawn_player()` in `spawn.cpp`:
```cpp
// In spawn_player(), after adding other components:
engine_add_component<CameraRigState>(e);
```

**Verification**: Build and run. All behavior from Task 2 still works. No compile errors from the removed `strip_player_roll`.

---

### Task 5 — Feed collision roll from damage_resolve()

**File**: `src/game/damage.cpp`

In `damage_resolve()`, when a collision is applied to the player entity, write a `collision_roll_impulse` value into the player's `CameraRigState`. The sign encodes which direction the hit came from; the magnitude is the raw KE already computed.

Find the section in `damage_resolve()` where `apply_damage()` is called for player collision events. After that call, add:

```cpp
// Feed collision visual roll to camera rig.
if (engine_has_component<PlayerTag>(receiver) &&
    engine_has_component<CameraRigState>(receiver)) {
    auto& crs = engine_get_component<CameraRigState>(receiver);
    // contact_normal is the separation direction of the hit.
    // Project onto rig's right axis to get roll direction.
    glm::vec3 rig_right = crs.rig_rotation * glm::vec3(-1.f, 0.f, 0.f);
    float roll_sign = glm::dot(contact_normal, rig_right);
    crs.collision_roll_impulse += roll_sign * ke;  // ke = kinetic energy already computed
}
```

`contact_normal` is already available in `damage_resolve()` as the variable used for the approach-direction check. `ke` is the raw kinetic energy value used for damage scaling.

**Note**: `collision_roll_impulse` accumulates within a frame (multiple simultaneous hits). `camera_rig_finalize()` reads it, applies `collision_roll_gain`, clamps to `collision_roll_max`, and resets it to 0 each frame (as specified in Task 2).

**Verification**: Fly into a large asteroid. The ship briefly rolls in the direction of the hit, then springs back upright. The camera does not roll — it stays stable relative to `rig_rotation`.

---

### Task 6 — Weapons fire toward camera look-target

**File**: `src/game/weapons.cpp`

The look-target is `player_pos + rig_fwd * cam_look_ahead + rig_up * cam_look_up_bias`. Rather than recomputing the exact camera look-target in `weapons.cpp`, expose a helper from `camera_rig.cpp`.

**Add to `camera_rig.h`**:
```cpp
// Returns the world-space aim point (camera look-target) for the current frame.
// Used by weapons to fire toward the crosshair, not along the ship nose.
// Returns player_pos + rig_fwd * cam_look_ahead if called before finalize() runs.
glm::vec3 camera_rig_aim_point();
```

**Add to `camera_rig.cpp`**:
```cpp
static glm::vec3 s_aim_point = {0,0,0};  // set in camera_rig_finalize()
glm::vec3 camera_rig_aim_point() { return s_aim_point; }
```

In `camera_rig_finalize()`, after computing `look_target`, add:
```cpp
s_aim_point = look_target;
```

**In `weapons.cpp`, `laser_fire()` and `plasma_fire()`:**

Change the `forward` / aim direction from `t.rotation * vec3(0,1,0)` to aim toward the camera look-target:

Old (both functions):
```cpp
const glm::vec3 forward = t.rotation * glm::vec3(0.f, 1.f, 0.f);
```

New (both functions):
```cpp
const glm::vec3 aim_point = camera_rig_aim_point();
const glm::vec3 forward   = glm::normalize(aim_point - t.position);
```

Add `#include "camera_rig.h"` to `weapons.cpp`.

For the laser raycast: use `forward` as the direction from `origin` toward `aim_point`. The behavior is: the laser fires from the ship's nose in the direction of the screen crosshair, not rigidly along the ship's nose.

For the plasma projectile spawn position: the spawn offset should also use `forward`:
```cpp
const glm::vec3 spawn_pos = t.position + forward * (k_ship_half + constants::plasma_sphere_radius + 0.1f);
const glm::vec3 velocity  = forward * ws.plasma_speed;
```

**Verification**: The player can now aim by pointing the crosshair (screen center, looking at `look_target`). Firing at an asteroid directly ahead works. Firing while the ship is offset from center of screen should still hit approximately where the center of screen points.

---

### Task 7 — HUD: draw crosshair at screen center

**File**: `src/game/hud.cpp`

The screen center already has variables `cx` and `cy` defined in `hud_render()`. Add a simple crosshair glyph drawn with `ImGui::GetWindowDrawList()`:

```cpp
// In hud_render(), after existing ImGui setup, before EndFrame:
auto* dl = ImGui::GetForegroundDrawList();
const float cx = sapp_width()  * 0.5f;
const float cy = sapp_height() * 0.5f;
const float arm = 10.f;
const float gap = 3.f;
ImU32 col = IM_COL32(255, 255, 255, 200);
dl->AddLine({cx - arm - gap, cy}, {cx - gap, cy}, col, 1.5f);
dl->AddLine({cx + gap, cy}, {cx + arm + gap, cy}, col, 1.5f);
dl->AddLine({cx, cy - arm - gap}, {cx, cy - gap}, col, 1.5f);
dl->AddLine({cx, cy + gap}, {cx, cy + arm + gap}, col, 1.5f);
```

This draws a gap crosshair (four lines with a gap in the center) — visible against both bright and dark backgrounds. No additional state needed.

**Verification**: A white crosshair appears at screen center at all times during gameplay. It does not disappear during win/loss overlays (if desired, gate it on `s_match_state.phase == MatchPhase::Playing`).

---

## 6. Tuning Reference

After all tasks are done, these constants are likely to need tuning in a play session. Change only in `constants.h`:

| Constant | Start value | What it affects |
|---|---|---|
| `cam_turn_rate` | 1.8 | Steering speed (increase if too sluggish) |
| `cam_offset_back` | 30.0 | Camera distance behind ship |
| `cam_offset_up` | 10.0 | Camera height above ship |
| `cam_look_ahead` | 25.0 | How far ahead the camera aims (affects ship Y position on screen) |
| `cam_look_up_bias` | 8.0 | How high the look-target is above the look-ahead point |
| `cam_lag_factor` | 8.0 | Camera follow tightness (higher = snappier) |
| `visual_bank_gain` | 0.35 | Bank angle per pixel of mouse input |
| `visual_bank_max` | 0.55 | Maximum bank angle (~31°) |
| `visual_bank_spring` | 5.0 | How quickly bank returns to level |
| `collision_roll_gain` | 0.008 | How much KE turns into roll angle |
| `collision_roll_max` | 1.0 | Maximum collision roll (~57°) |
| `collision_roll_spring` | 3.5 | How quickly collision roll damps out |

---

## 7. What Is NOT in Scope

The following are Freelancer features we are NOT implementing to keep this tractable:

- **Free-look mode** (Spacebar toggle): the mouse always steers when LMB held.
- **Cruise engines** (Shift+W fast travel): not in game design.
- **Kill-engines drift** (Z key): not in game design.
- **Gimballed weapon aim** (weapons tracking independently of ship heading): firing toward `cam_aim_point` covers 90% of this.
- **Camera lag behind ship heading** (ship position shifts on screen during hard turns): out of scope for this iteration.

---

## 8. Files Changed Summary

| File | Change |
|---|---|
| `src/game/spawn.cpp` | Task 0: model filename; Task 4: add CameraRigState to player |
| `src/game/components.h` | Task 1: add CameraRigState |
| `src/game/constants.h` | Task 1: add V2 camera constants, remove player_turn_speed |
| `src/game/camera_rig.h` | Task 2: new API (init, input, finalize, aim_point) |
| `src/game/camera_rig.cpp` | Task 2: complete rewrite |
| `src/game/player.cpp` | Task 3: remove rotation and velocity-alignment blocks |
| `src/game/game.cpp` | Task 4: remove strip_player_roll, fix tick order |
| `src/game/damage.cpp` | Task 5: feed collision_roll_impulse |
| `src/game/weapons.cpp` | Task 6: fire toward camera_rig_aim_point() |
| `src/game/hud.cpp` | Task 7: draw crosshair |
