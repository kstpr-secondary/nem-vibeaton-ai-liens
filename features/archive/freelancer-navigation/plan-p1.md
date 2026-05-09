# Phase Plan: Freelancer Navigation — Phase 1: Core Navigation Model

**Gate**: Feature Brief approved  
**Roadmap alignment**: N/A (Phased)  
**Build target**: `cmake --build build --target game`

## Expected Outcome

Flying with W, then dragging LMB, curves the ship's flight path; releasing LMB leaves it flying straight nose-forward. Pressing Z toggles "engine kill" — LMB drag rotates only the nose (old behavior), velocity unchanged. Pressing Z again snaps velocity back to nose-forward direction at current speed.

---

## Phase decisions

**A/D Strafe in Normal mode**: Option A — natural realignment. A/D add lateral acceleration exactly as today (`player_strafe` constant). When LMB steering is next used the velocity-coupling mechanism in Task 3 rotates the composite velocity toward the nose. No new code path needed for strafe; this is a non-decision from the implementation standpoint.

**`SAPP_KEYCODE_Z` conflict check**: confirmed unused in the game workstream. Existing non-movement bindings are L, C (renderer app), Q, E (weapons, engine app). Z is free.

## Tasks

| ID | Description | Files | Schedule | Acceptance |
|----|-------------|-------|----------|------------|
| 1 | **Add `NavigationMode` enum and extend `CameraRigState`**. Add `enum class NavigationMode : uint8_t { Normal = 0, EngineKill = 1 };` to `components.h`. Add `NavigationMode nav_mode = NavigationMode::Normal;` as a field on `CameraRigState`. No other changes — this is a pure data addition. | `src/game/components.h` | GATE | `cmake --build build --target game` exits 0; no other files need to change yet |
| 2 | **Z key toggle in `player_update`**. Declare `static bool s_prev_z = false;` at function scope in `player_update`, outside the entity loop (same placement pattern as `s_prev_enter` in `handle_restart_quit_input` in `game.cpp`). On rising edge of `SAPP_KEYCODE_Z`: flip `crs.nav_mode`. On the `Normal → EngineKill` transition: no velocity change. On the `EngineKill → Normal` transition: snap `rb.linear_velocity` to `glm::length(rb.linear_velocity) * (crs.rig_rotation * glm::vec3(0.f, 0.f, -1.f))` — magnitude preserved, direction set to current nose. Guard against zero-length velocity before snapping (if speed ≈ 0, skip snap). | `src/game/player.cpp` | ‖ | Press Z while flying in Normal mode: mode switches to EngineKill — no velocity change, ship continues on same path. Press Z again while sliding: mode returns to Normal — velocity snaps to nose-forward direction at current speed; sliding stops. |
| 3 | **Velocity coupling in `camera_rig_input`**. Extend the ECS view to include `RigidBody`: `reg.view<PlayerTag, Transform, CameraRigState, RigidBody>()`. Inside the `engine_mouse_button(0)` block, compute `q_delta = glm::normalize(q_yaw * q_pitch)` (rename from the existing inline application). Apply it to both `crs.rig_rotation` and `rb.linear_velocity` only when `crs.nav_mode == NavigationMode::Normal`. When `nav_mode == EngineKill`, apply `q_delta` only to `crs.rig_rotation` (existing behavior, no velocity touch). Quaternion rotation on a `glm::vec3` preserves magnitude exactly — do not recompose from `speed * forward`. | `src/game/camera_rig.cpp` | ‖ with 2 | Hold W to cruise speed, drag LMB left: velocity vector rotates with the nose — ship curves left, no sideways slide visible. In EngineKill mode (Z held): drag LMB — nose rotates, ship slides sideways. |

**Schedule codes**:
- `GATE` — runs alone first; tasks 2 and 3 depend on it
- `‖` — parallel; tasks 2 and 3 touch disjoint files

---

## Fallbacks

- **Task 2**: if placing `s_prev_z` as a function-local static inside `player_update` conflicts with the update loop structure (e.g., the toggle logic is hard to read mixed with physics), extract it into a dedicated `handle_engine_kill_input(CameraRigState&, RigidBody&)` free function in `player.cpp`, mirroring the `handle_restart_quit_input` pattern in `game.cpp`.
- **Task 3**: if adding `RigidBody` to the view in `camera_rig_input` conflicts with the cached `s_player` handle (e.g., entity doesn't have the component at init time), retrieve `RigidBody` via `reg.get<RigidBody>(s_player)` directly instead of through the view — the entity is guaranteed to have it after `spawn_player`.

---

## Human Checkpoint

**Run**: `cmake --build build --target game && ./build/game`

**Look for**:
1. Accelerate with W to cruising speed. While holding W, drag LMB left for ~1 second. **The ship should curve left** — nose and velocity move together; no sideways slide; ship continues straight nose-forward after releasing mouse.
2. Press Z. Accelerate with W again. Drag LMB left. **Nose should rotate but ship should slide sideways** (old behavior — engine kill).
3. While sliding sideways (Z/engine-kill active), press Z again. **Velocity should snap to the current nose direction** — the sliding stops and the ship continues in the new direction.
4. Verify A/D strafe: in Normal mode, tapping A or D should add a side impulse. That side component is absorbed and vanishes naturally (drag + next LMB steering will realign), not a persistent sideways drift.

**Pass**: All four behaviors above are clearly observable. No jitter, stutter, or unexpected velocity discontinuities except the intentional snap on EngineKill→Normal.

**Stop**: Velocity coupling produces perceptible jitter during LMB drag, or the snap on Z feels unacceptably harsh at high speed. If harsh snap: plan Phase 2 to add a convergence rate constant (`player_nav_align_rate`) rather than instant snap. If jitter: investigate floating-point accumulation in the quaternion rotation chain.
