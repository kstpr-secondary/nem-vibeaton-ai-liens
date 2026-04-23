# Quickstart: Space Shooter MVP

**Date**: 2026-04-23
**Feature**: [spec.md](./spec.md)

---

## Prerequisites

- Ubuntu 24.04 LTS
- Clang, CMake ≥ 3.24, Ninja
- Repository cloned with all submodules / FetchContent deps cached in `build/_deps/`
- Renderer and engine milestones landed (or mocks enabled)

---

## Build

```bash
# Full build (after milestone merges)
cmake --build build

# Game-only iterative build
cmake --build build --target game
```

---

## Run

```bash
./build/game
```

---

## Controls

| Input | Action |
|-------|--------|
| Mouse move + Left click hold | Aim (pitch + yaw) |
| Right click | Fire active weapon |
| W / S | Thrust forward / reverse |
| A / D | Strafe left / right |
| Space (hold) | Boost (2× thrust) |
| Q | Switch to Plasma |
| E | Switch to Laser |
| Enter | Restart match |
| Esc | Quit |

---

## Gameplay Loop

1. **Start**: Match begins in Playing phase. Player ship at field center, 1 enemy, 200 asteroids.
2. **Fly**: Navigate with WASD + mouse aim. Boost with Space for speed bursts.
3. **Fight**: Switch weapons (Q/E), fire (right click). Laser = instant hit, 10 dmg, 5s cooldown. Plasma = rapid projectiles, 0.5 dmg each, 0.1s cooldown.
4. **Survive**: Shield absorbs damage first, then HP. Shield regens after 3s no-damage. Avoid asteroids — collisions deal kinetic energy damage.
5. **Win**: Destroy all enemies → "YOU WIN" overlay → auto-restart after 3s or press Enter.
6. **Die**: HP reaches 0 → "YOU DIED" overlay → auto-restart after 2s or press Enter.

---

## Milestone Progression

| Milestone | What's playable |
|-----------|----------------|
| G-M1 | Fly through 200 static asteroids. Camera follows. No physics/combat. |
| G-M2 | Asteroids drift + tumble. Ship bounces off asteroids and boundary. Boost works. |
| G-M3 | Full combat: laser + plasma weapons, 1 enemy, damage/death. |
| G-M4 | **MVP complete**: HUD, win/death overlays, restart flow. |

---

## Development with Mocks

Before upstream milestones land, build with mocks:

```bash
cmake -B build -DUSE_ENGINE_MOCKS=ON -DUSE_RENDERER_MOCKS=ON -G Ninja
cmake --build build --target game
```

Mock behavior:
- Renderer mocks: all draw calls are no-ops; handle functions return valid sentinels
- Engine mocks: all API calls are no-ops; entity creation returns valid sentinels; queries return empty results

Swap to real implementations by reconfiguring without mock flags after upstream milestones merge.

---

## Key Files

| File | Purpose |
|------|---------|
| `src/game/main.cpp` | Entry point — init, callbacks, run |
| `src/game/game.h` / `game.cpp` | Game lifecycle + match state machine |
| `src/game/player.cpp` | Flight controller + boost |
| `src/game/weapons.cpp` | Laser + plasma fire logic |
| `src/game/enemy_ai.cpp` | Seek + shoot AI |
| `src/game/hud.cpp` | ImGui HUD + overlays |
| `src/game/constants.h` | All tuning values |
