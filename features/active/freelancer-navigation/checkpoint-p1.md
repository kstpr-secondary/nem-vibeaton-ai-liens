# Checkpoint: Freelancer Navigation — Phase 1

<!--
  This file records human-observed verification results.
  Feature Planner may draft it from free-form human feedback; human confirms before writing.
  See feature-planner/SKILL.md "Checkpoint drafting" for the process.
-->

**Date**: 2026-05-10
**Verified by**: kstpr

## Result: PASS

## What was tested

Built and ran `cmake --build build --target game && ./build/game`. Verified all four behaviors from the Human Checkpoint: (1) W to cruise speed then LMB-drag curves the ship nose-and-velocity together with no sideways drift; (2) Z toggles engine-kill so LMB-drag rotates the nose while velocity slides independently; (3) second Z press snaps velocity to the current nose direction, stopping the slide; (4) A/D adds a side impulse in Normal mode that does not persist as a permanent sideways drift.

## Observations

All four checkpoint criteria passed. Two observations for Phase 2 consideration:

- **Steering over-responsiveness**: LMB drag feels nearly instantaneous — the velocity vector tracks the nose with no perceptible lag or rate limit. This is mechanically correct (full coupling per frame) but feels too agile. A tunable rate constant clamping how many degrees per frame the velocity vector can rotate toward the nose would let future ships be differentiated by agility. This should be a named constant (e.g., `player_nav_align_rate` in degrees/s or as a slerp factor) so it can be exposed per-ship.

- **EngineKill→Normal snap**: the velocity snap on the second Z press feels abrupt, especially at high speed. The plan flagged this as a potential stop condition, but the human prefers to keep it as-is for now — it has an interesting mechanical feel and can be revisited later if needed.

## Roadmap impact

*(Not applicable — Phased feature.)*

## Next step

Generate `plan-p2.md` (visual cosmetics: `visual_pitch` component, pitch tilt on vertical LMB drag, spring-back on release).
