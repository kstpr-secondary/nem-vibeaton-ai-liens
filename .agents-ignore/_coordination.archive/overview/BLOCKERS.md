# BLOCKERS.md — Cross-Workstream

> Log blockers here when they cannot be resolved within 90 seconds. Include: what, who, impact, status.

## Active Blockers

| ID | Description | Reported By | Impact | Status | Notes |
|---|---|---|---|---|---|
| B-001 | `vibeaton_collect_sources` EXCLUDE_REGEX `/mocks/` also filters mock files when MOCK_ROOT is active (path still contains `/mocks/`), so `USE_ENGINE_MOCKS=ON` falls back to bootstrap stub instead of `engine_mock.cpp`. Needs CMakeLists fix: skip exclude filter when using MOCK_ROOT, or place mock file outside a `/mocks/` path segment. | claude@laptopB | Game workstream cannot use engine mocks until fixed | OPEN | CMakeLists.txt owned by Renderer/Systems Architect. Engine workstream mock file `src/engine/mocks/engine_mock.cpp` is ready. |

## Resolved Blockers

| ID | Description | Resolution | Notes |
|---|---|---|---|
| | | | |
