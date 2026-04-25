# PROGRESS.md — Game Workstream

> Updated by game developer as milestones complete. See `_coordination/overview/PROGRESS.md` for cross-workstream status.
> Live state on `feature/game` branch; cross-workstream view becomes visible only after milestone merge to `main`.

## Milestones

| Milestone | Status | Owner | Notes |
|---|---|---|---|
| SETUP — main.cpp + game.h/.cpp skeleton | TODO | — | G-001..G-002; window opens, exits on Esc |
| FOUNDATION — components / constants / spawn / render_submit | TODO | — | G-003..G-006; BLOCKS all G-M*; SPEC-VALIDATE on G-003 (cross-cutting types) |
| G-M1 — Flight + Scene + Camera | TODO | — | G-007..G-010; third-person camera with first-person fallback; 200 procedurally-placed asteroids; SPEC-VALIDATE on G-010 |
| G-M2 — Physics, Containment, Asteroid Dynamics | TODO | — | G-011..G-014; runs on real engine E-M4 (FROZEN v1.2); SPEC-VALIDATE+REVIEW on G-012 (FR-005 KE damage) |
| G-M3 — Weapons + Enemies + HP/Shield | TODO | — | G-015..G-022; laser, plasma, one game-local enemy AI, minimal explosion VFX; SPEC-VALIDATE+REVIEW on G-020 (damage routing) |
| G-M4 — HUD + Game Flow + Restart | TODO | — | G-023..G-027; Game MVP complete; SPEC-VALIDATE+REVIEW on G-024 (4-state machine, simultaneous death → Victory) |
| POLISH — Tuning + Acceptance + E2E | TODO | — | G-028..G-030; final pre-demo gate on `rtx3090` |
| G-M5 — AI Upgrade & Scaling | TODO | — | Desirable; depends on engine E-M5 |
| G-M6 — Visual Polish | TODO | — | Desirable; depends on renderer R-M5 |
| G-M7 — Feel Tuning Pass | TODO | — | Desirable |

## Per-Milestone Acceptance Outcomes

- **SETUP**: `./build/game` launches, clears to color, exits on Esc.
- **FOUNDATION**: build succeeds; `render_submit()` iterates an empty view; no behavioral milestone.
- **G-M1**: Player flies through 200 asteroids; camera follows. (US1.1 partially; FR-001/002/014.)
- **G-M2**: Field feels alive; ship + asteroids reflect off boundary; speed cap holds; player takes shield→HP damage; boost drains/regens. (US1.2/1.3/1.4; FR-003/004/005/013.)
- **G-M3**: Full combat with one enemy; weapons feel distinct; enemy death VFX. (US2.1–2.4; FR-006/007/008/009.)
- **G-M4**: HUD shows all combat state; win/death overlays; auto + manual restart; Esc quits. (US3.1–3.4; FR-010/011/012.) **Game MVP complete.**
- **POLISH**: All spec acceptance scenarios validated; quickstart end-to-end pass on demo machine.

## Notable Watchpoints (from Game SpecKit)

- **Match state evaluation order**: Victory checked before PlayerDead — simultaneous death yields Victory (clarification 2026-04-23).
- **No collision damage threshold**: every contact deals KE-proportional damage.
- **Game-local enemy AI for MVP**: do not depend on engine E-M5 for G-M3.
- **`sapp_request_quit` exposure**: G-026 may require a renderer-side `renderer_request_quit()` helper if `renderer.h` does not transitively expose `sokol_app.h`. Watch during implementation; file BLOCKER if needed (blueprint §8.5 rule 11).

## Cross-References

- Live tasks: `_coordination/game/TASKS.md`
- Architecture: `docs/architecture/game-architecture.md`
- Interface contract: `docs/interfaces/game-interface-spec.md`
- Game design + scope: `docs/game-design/GAME_DESIGN.md`
- Workstream design: `docs/game-design/game-workstream-design.md`
- SpecKit source: `docs/planning/speckit/game/`
