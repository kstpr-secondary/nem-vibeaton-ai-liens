# Feature Brief: Enemy Separation

**Type**: Quick
**Branch**: feature/more-enemies
**Workstreams**: game
**Frozen interfaces affected**: none

## Goal

Increase the enemy count to 3 and prevent the enemies from stacking on top of each other. Currently the homing algorithm drives all enemies to the same spot; enemies need to actively repel peers that are closer than one enemy diameter so each ship stays visually distinct during combat.

## Success Criteria

- [ ] Visual: Three enemy ships are present at the start of a match, spawned at distinct positions (not overlapping).
- [ ] Gameplay: During combat, no two enemies visually overlap or clip through each other; each enemy maintains at least one ship-diameter of clear space from its peers while still pursuing the player.
- [ ] Build: `cmake --build build --target game` exits 0.

## Scope

**In**: spawn count increase (constants + game.cpp loop), peer-separation steering in `enemy_ai_update`.
**Out (explicit)**: Formation flying, flocking, collision avoidance with asteroids, any change to enemy-player pursuit logic, any change to firing or damage systems.
