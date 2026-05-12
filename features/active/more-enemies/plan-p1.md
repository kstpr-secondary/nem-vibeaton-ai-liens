# Phase Plan: More Enemies — Phase 1: Spawn 3 + Peer Steering

**Gate**: Feature Brief approved (Phase 1)
**Roadmap alignment**: N/A (Quick)
**Build target**: `cmake --build build --target game`
**Groomer Verdict**: PASS

## Expected Outcome

Three enemy ships are present at match start, evenly spread around the origin. During play, no two enemies share the same screen position; each backs off any peer that enters within one ship-diameter of gap.

---

## Tasks

| ID | Status | Description | Files | Schedule | Acceptance |
|----|--------|-------------|-------|----------|------------|
| 1 | [x] | **Bump enemy count and spread spawn positions.** Change `constants::enemies_at_start` from 1 to 3 in `constants.h`. In `game.cpp`, replace the single hardcoded `spawn_enemy(glm::vec3(50.f, 10.f, -50.f))` + `s_match_state.enemies_remaining = 1` with a loop `for (int i = 0; i < constants::enemies_at_start; ++i)`. Compute each spawn position on a circle of radius 70 f at height 10 f: `angle = 2π·i / enemies_at_start`, `pos = {70·cos(angle), 10, 70·sin(angle)}`. Set `s_match_state.enemies_remaining = constants::enemies_at_start` after the loop. Include `<cmath>` if `std::cos`/`std::sin` aren't already in scope. | `src/game/constants.h`, `src/game/game.cpp` | ‖ | Build exits 0; three enemy ships visible at match start, none overlapping. |
| 2 | [x] | **Add peer-separation steering in `enemy_ai_update`.** At the end of the per-enemy loop (after seek velocity and firing are set), add a nested view over all other `EnemyTag + Transform` entities. For each peer where `self_entity != peer_entity`: compute `away = t.position - peer_t.position`, `dist = glm::length(away)`. Skip if `dist < 1e-6f`. If `dist < k_min_separation` (= 8.0 f; see note), accumulate `delta_v += (away / dist) * k_separation_speed * (1.0f - dist / k_min_separation)`. After the peer loop, clamp `delta_v` so `glm::length(delta_v) <= k_separation_speed` (prevents a 3-way pile-up from tripling the repulsion), then add it to `rb.linear_velocity`. Constants (file-scope `static constexpr`): `k_min_separation = 8.0f`, `k_separation_speed = 20.0f`. **Why 8.0 f**: enemy half-extent is 2.0 f, so two touching ships have center distance 4.0 f; "one diameter gap" adds another 4.0 f = 8.0 f center-to-center threshold. Edge cases: `dist < 1e-6f` → skip (no NaN); `dist == k_min_separation` → weight = 0 (no step discontinuity); 3-way pile-up → accumulated Δv clamped to `k_separation_speed`. | `src/game/enemy_ai.cpp` | ‖ | Build exits 0; enemies visibly spread during play; no NaN or erratic launches. |

**Schedule codes**:
- `‖` — both tasks are independent and touch disjoint files; may be executed in either order or in parallel.

---

## Fallbacks

- Task 2: if linear weight `(1 - dist/k_min_sep)` still causes jitter at the boundary, switch to quadratic `(1 - (dist/k_min_sep)²)`; no structural change needed.
- Task 2: if `k_separation_speed = 20.0f` is too aggressive relative to `pursuit_speed = 16.0f`, tune down to 12–14 f; the constant is isolated in `enemy_ai.cpp`.

---

## Human Checkpoint

**Run**: `cmake --build build --target game && ./build/game`
**Look for**: Three distinct enemy ships on screen at match start; during combat, enemies visibly repel each other when they get close instead of occupying the same point.
**Pass**: All three ships remain visually separate throughout the session; no two ships clip or stack; each backs off a peer that approaches within one diameter.
**Stop**: Any two ships permanently occupy the same visual position, one ship rockets erratically across the field on separation, or the build fails.
