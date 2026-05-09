# Feature Brief: Freelancer-Style Navigation

**Type**: Phased  
**Branch**: feature/freelancer-navigation  
**Workstreams**: game  
**Frozen interfaces affected**: none  

## Goal

Replace the current decoupled-orientation flight model with a Freelancer-inspired system where the ship's velocity vector tracks the nose direction during mouse steering. The player steers by dragging LMB — the nose turns and velocity follows — so the ship always flies where it's pointing unless the "engine kill" mode (Z) is engaged, which restores the current thruster-only behavior.

## Success Criteria

- [ ] **W/S thrust**: holding W/S in Normal mode accelerates/decelerates along the current nose direction (rig_rotation forward). The ship does not drift sideways when W is held with a static orientation.
- [ ] **LMB velocity coupling**: hold W to reach cruising speed, then drag LMB left. The ship banks into the turn and the flight path curves — the ship is visually flying in the new nose direction, not sliding sideways.
- [ ] **Release LMB**: release the mouse button while flying at speed. The ship continues in a straight line along its current nose direction; no sideways drift, no gradual realignment — it is already aligned.
- [ ] **Z toggle — Engine Kill**: press Z while flying; the Z indicator appears on the HUD (or no HUD change is needed — see Scope). Now drag LMB — the nose rotates but the velocity vector does not change. The ship visually slides sideways relative to its nose (old behavior). Press Z again to exit — the velocity snaps to nose-forward at current speed.
- [ ] **Strafe**: A/D behavior matches whichever option is chosen in Phase 1 (see Key Unknowns). The behavior is consistent and does not produce unexpected drift.
- [ ] **Build**: `cmake --build build --target game` exits 0.

## Phase Plan

**Phase 1 — Core navigation model**: NavigationMode enum, Z toggle, velocity-orientation coupling in camera_rig_input, EngineKill→Normal transition, strafe design decision.  
**Phase 2 — Visual cosmetics**: `visual_pitch` component field (mirrors `visual_bank`), vertical drag produces pitch tilt on model, both pitch and bank spring back to zero on LMB release or when not steering.

## Scope

**In**:
- Normal flight mode: velocity direction tracks nose; W/S thrust along nose
- Engine Kill mode (Z): orientation-only LMB drag, velocity untouched (current behavior)
- EngineKill→Normal transition behavior (see unknowns)
- A/D strafe with decided behavior (see unknowns)
- `visual_pitch` field and spring (Phase 2)

**Out (explicit)**:
- HUD indicator for engine-kill state — not in this feature; can be added separately
- Enemy AI navigation — unchanged
- Camera rig structure, lag, offset constants — unchanged
- Boost (Space) behavior — unchanged; Space still scales forward thrust
- Roll/yaw physics during collisions — unchanged

## Key Unknowns

| Unknown | Risk if wrong | How to verify | Decision |
|---------|--------------|---------------|---------|
| **Strafe (A/D) behavior in Normal mode** | Wrong feel — strafe that persists kills the nose-forward contract; strafe that's disabled may frustrate players trying to dodge | Playtest after P1 | Three options below — pick before P1 plan |
| **EngineKill→Normal transition** | Snapping velocity feels abrupt at high speed; converging may feel unresponsive or "glidey" | Playtest the snap first; add rate-constant K if needed | Snap is default; flag for P1 checkpoint |

### Strafe options

**Option A — Natural realignment (recommended)**: A/D add lateral acceleration exactly as today. When LMB steering is next used (or immediately if LMB is held), the velocity-coupling mechanism rotates the composite velocity toward nose direction. Strafe is most effective at low forward speed or in Engine Kill mode. No new code path needed for strafe itself.

**Option B — Strafe disabled in Normal mode**: A/D do nothing in Normal mode; they only work in Engine Kill mode. Clean, zero-ambiguity, but loses the dodge capability.

**Option C — Decoupled lateral speed**: A/D maintain a separate lateral velocity component that decays back to zero when neither A nor D is held. The forward velocity component is decoupled and tracks nose independently. Extra state, extra constants, but gives an arcade "slide" feel.

## Known Risks

- **Frame ordering**: `player_update` (step 1) runs before `camera_rig_input` (step 2). Velocity direction rotation must happen inside `camera_rig_input` alongside the `rig_rotation` update — both must apply the same quaternion delta so they stay synchronized. Do not apply the velocity redirect in `player_update` with a stale rig orientation.
- **Speed preservation on coupling**: when rotating the velocity vector to follow the nose, magnitude must be preserved exactly. Use quaternion rotation on the velocity vector, not recomposition from speed * forward (which would introduce floating-point drift across frames).
- **visual_bank is horizontal-only**: `camera_rig_finalize` uses `engine_mouse_delta().x` for bank target. Phase 2 adds `visual_pitch` driven by `engine_mouse_delta().y`, using the same spring pattern. The combined rotation is: `rig_rotation * pitch_quat * bank_quat * k_ship_base_orientation`.
- **Z key is not yet mapped**: confirm no other system uses SAPP_KEYCODE_Z before adding the toggle.
