# Phase Plan: Freelancer Navigation — Phase 2: Visual Cosmetics

**Gate**: `checkpoint-p1.md` exists  
**Roadmap alignment**: N/A (Phased)  
**Build target**: `cmake --build build --target game`  
**Groomer Verdict**: PASS

## Expected Outcome

Dragging LMB up/down visually tilts the ship model nose-up/nose-down (matching the `visual_bank` pattern for horizontal drag). Both the bank and pitch tilts spring back to zero when the mouse button is released. No change to actual flight physics.

---

## Tasks

| ID | Status | Description | Files | Schedule | Acceptance |
|----|--------|-------------|-------|----------|------------|
| 1 | [x] | **Add `visual_pitch` field and pitch constants.** In `CameraRigState` add `float visual_pitch = 0.f;` immediately after `visual_bank`. In `constants.h`, alongside the three `visual_bank_*` constants, add three mirrored pitch constants: `visual_pitch_gain = 0.25f`, `visual_pitch_max = 0.35f`, `visual_pitch_spring = 5.0f`. These are starting values — tuned by feel at checkpoint. Both additions are pure data; no logic changes yet. | `src/game/components.h`, `src/game/constants.h` | GATE | `cmake --build build --target game` exits 0; no behavior change visible yet |
| 2 | [x] | **Spring `visual_pitch` and extend the rotation chain in `camera_rig_finalize`.** In Step A (after the `visual_bank` spring block), add a parallel spring for `visual_pitch`: `pitch_target = -engine_mouse_delta().y * constants::visual_pitch_gain` when LMB held (note the negation — dragging down gives positive `delta.y`, but `angleAxis(positive, vec3(1,0,0))` tilts the nose UP; negating produces nose-DOWN to match the spec), clamped to `±visual_pitch_max`, otherwise `pitch_target = 0.f`; spring `crs.visual_pitch` toward target using `visual_pitch_spring * dt` (identical pattern to `visual_bank`). In Step C, insert `pitch_quat` into the rotation chain per the brief spec: `pitch_quat = glm::angleAxis(crs.visual_pitch, glm::vec3(1.f, 0.f, 0.f))`, then change the assignment to `t.rotation = crs.rig_rotation * pitch_quat * roll_quat * k_ship_base_orientation`. | `src/game/camera_rig.cpp` | → | Dragging LMB vertically produces a visible nose tilt that springs back to zero on mouse release; horizontal bank is unaffected |

**Schedule codes**:
- `GATE` — runs alone first; Task 2 depends on it
- `→` — sequential, runs after the GATE

---

## Deferred from checkpoint-p1

**`player_nav_align_rate`**: checkpoint-p1 flagged that LMB steering feels nearly instantaneous and recommended a named constant (degrees/s or slerp factor) to cap how fast the velocity vector rotates toward the nose, enabling per-ship agility differentiation. This is a gameplay/physics concern, not a visual cosmetics concern, so it is out of scope for this phase. It should be the first task of a follow-on tuning phase.

---

## Fallbacks

- **Task 2, rotation order**: if the combined `pitch_quat * roll_quat` produces unexpected gimbal interaction (nose tilt and bank fighting each other during diagonal drags), swap the order to `roll_quat * pitch_quat` and re-verify at checkpoint. The brief specifies `pitch * bank` as default; reverting to `bank * pitch` is acceptable if it reads better visually.
- **Task 2, pitch axis**: `glm::vec3(1.f, 0.f, 0.f)` is the local +X (ship right) axis in the post-`rig_rotation` frame. If the model tilts sideways instead of nose-up/down, the axis or the local-frame assumption is off — double-check by temporarily logging `crs.visual_pitch` vs. observed tilt direction.

---

## Human Checkpoint

**Run**: `cmake --build build --target game && ./build/game`

**Look for**:
1. Hold W to reach cruising speed. Drag LMB downward — the ship model nose should visibly dip toward the camera (nose-down tilt on the model). Drag LMB upward — nose tilts away (nose-up). Release the mouse button — the pitch springs back to level. No change to actual flight path or rig orientation.
2. Drag LMB diagonally (down-left). Both the bank tilt (left) and pitch tilt (down) should appear simultaneously without fighting each other. Releasing the mouse springs both back to zero.
3. Fly normally with W/S only, no LMB — no pitch tilt at rest.

**Pass**: All three behaviors clearly visible. Bank and pitch tilts are independent, visually pleasing, and spring cleanly to zero on release.

**Stop**: Pitch tilt goes in the wrong direction (negate `delta.y` and restart), or pitch and bank rotations visibly fight each other and produce a strange compound tilt during diagonal drags (swap rotation order per fallback and re-verify). If pitch gain/max constants need significant tuning beyond a constant edit, note the preferred values in the checkpoint.
