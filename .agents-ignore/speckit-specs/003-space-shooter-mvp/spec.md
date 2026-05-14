# Feature Specification: Space Shooter MVP

**Feature Branch**: `003-create-feature-branch`  
**Created**: 2026-04-23  
**Status**: Draft  
**Input**: User description: "Act like a game engineer, you understand game workflows, game engine APIs and libraries, computer graphics, navigation and UX in the games. Check the game concept and milestones document, the game design folder, and the game engine interfaces folder."

## Clarifications

### Session 2026-04-23

- Q: What are the distinct match phases and their allowed transitions? → A: 4 states — Playing → PlayerDead / Victory → Restarting → Playing. Terminal states show overlay and start auto-countdown; Restarting clears entities and rebuilds the scene. Manual restart skips countdown.
- Q: What happens when the player and last enemy die in the same tick? → A: Player wins. If the last enemy and the player both reach 0 HP in the same tick, the match transitions to Victory because the player completed the objective.
- Q: Is there a collision damage threshold below which impacts deal zero damage? → A: No threshold. Every collision deals damage proportional to relative kinetic energy, even tiny amounts. Slow glances deal sub-1 damage; hard rams deal significant damage.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Fly and Survive in the Asteroid Field (Priority: P1)

As a player, I can control a ship in a bounded asteroid field, navigate with responsive flight controls, and survive collisions while managing health, shield, and boost resources.

**Why this priority**: This is the core moment-to-moment gameplay loop. Without movement, collision response, and survival resources, no combat or progression has value.

**Independent Test**: Can be fully tested by starting a match, flying through the field, colliding with asteroids and containment boundary, using boost until drained, and confirming resource depletion and regeneration behavior.

**Acceptance Scenarios**:

1. **Given** a new match with player resources at full values, **When** the player uses movement and boost controls, **Then** the ship moves as expected, boost consumption occurs while active, and boost regeneration resumes after use.
2. **Given** the player collides with asteroids at varying impact speeds, **When** shield is available, **Then** shield absorbs damage proportional to relative kinetic energy (no minimum threshold; slow taps deal sub-1 damage) and health remains unchanged.
3. **Given** the player has no shield remaining, **When** the player takes collision or combat damage, **Then** health decreases and player death is triggered at zero health.
4. **Given** an asteroid reaches or crosses the containment boundary, **When** boundary reflection is applied, **Then** the asteroid remains in play and its speed respects configured containment limits.

---

### User Story 2 - Engage Enemies with Two Distinct Weapons (Priority: P2)

As a player, I can switch between laser and plasma weapons, fire at enemies in real time, and use each weapon's distinct behavior to destroy all enemies.

**Why this priority**: Combat is the primary objective and differentiator of the experience. Distinct weapons create tactical choice and improve game feel.

**Independent Test**: Can be fully tested by spawning one enemy, switching active weapon, firing each weapon repeatedly, confirming cooldown rules and damage values, and verifying enemy elimination.

**Acceptance Scenarios**:

1. **Given** a live enemy target, **When** the player fires the laser, **Then** damage is applied to the first valid target in the shot path and laser cooldown is enforced before the next laser shot.
2. **Given** the plasma weapon is active, **When** the player holds fire input, **Then** rapid projectile shots are emitted at the expected cadence and each projectile expires on collision or timeout.
3. **Given** both weapons are available, **When** the player presses weapon-switch controls, **Then** active weapon changes immediately and subsequent shots use the selected weapon rules.
4. **Given** a weapon hit destroys an enemy, **When** enemy health reaches zero, **Then** the enemy is removed from active play and a visible death effect is shown.

---

### User Story 3 - Complete Match Flow with HUD and Restart (Priority: P3)

As a player, I can read combat state from the HUD, see win/death outcomes, and restart or quit without leaving the gameplay loop.

**Why this priority**: UI feedback and match flow are required for a complete demoable product and for players to understand status and outcomes.

**Independent Test**: Can be fully tested by playing a full match to both win and lose outcomes, checking HUD updates through combat, observing overlays, triggering automatic restart timing, and using manual restart and quit controls.

**Acceptance Scenarios**:

1. **Given** a match in progress, **When** player resources or weapon cooldown state changes, **Then** HUD indicators update within the same frame cycle.
2. **Given** all enemies are destroyed, **When** the win condition is evaluated, **Then** a win overlay appears and restart becomes available.
3. **Given** the player dies, **When** death is detected, **Then** a death overlay appears and automatic restart occurs after the configured delay.
4. **Given** the player presses restart or quit controls, **When** input is processed, **Then** the match resets cleanly or exits cleanly without undefined state.

### Edge Cases

- What happens when the player and the last enemy are destroyed in the same update tick? The player wins — Victory takes priority. The damage pipeline evaluates enemy deaths before player death within each tick so that the win condition is checked first.
- How does the system handle weapon fire input during cooldown? The input is accepted but no additional shot is emitted until cooldown allows it.
- How does the system handle containment interactions for very fast objects? Reflection must keep objects inside bounds and clamp excessive post-reflection speed.
- What happens if a target leaves line-of-sight exactly when an enemy decides to fire? The shot is canceled for that decision cycle and reevaluated in a later cycle.
- What happens when restart is requested while auto-restart countdown is active? Manual restart takes precedence and immediately resets match state.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The system MUST initialize a playable match with one player ship, one enemy ship, and an asteroid field of 200 asteroids contained within a spherical play boundary.
- **FR-002**: The system MUST provide player flight controls for thrust, reverse/deceleration, strafing, rotational aiming, boost activation, weapon switching, firing, restart, and quit.
- **FR-003**: The system MUST maintain player resources for health, shield, and boost within the range 0 to 100, with shield absorbing incoming damage before health.
- **FR-004**: The system MUST regenerate shield only after a no-damage delay and regenerate boost when boost is not actively consumed.
- **FR-005**: The system MUST apply collision damage proportional to relative kinetic energy on every impact (no minimum threshold) and apply combat damage from weapons, so that player death occurs immediately when health reaches zero.
- **FR-006**: The system MUST support two switchable weapons with distinct behavior: an instant-hit high-damage weapon with long cooldown and a projectile low-damage weapon with rapid cadence.
- **FR-007**: The system MUST enforce weapon cooldown and firing cadence rules consistently regardless of frame-rate variation.
- **FR-008**: The system MUST support enemy behavior that pursues the player and attempts ranged attacks when target range and line-of-sight conditions are satisfied.
- **FR-009**: The system MUST remove defeated enemies from active play and trigger a visible explosion effect at defeat time.
- **FR-010**: The system MUST detect and enforce the win condition when all active enemies are defeated.
- **FR-011**: The system MUST render an in-game HUD showing health, shield, boost, crosshair, active weapon, and weapon cooldown status.
- **FR-012**: The system MUST show terminal overlays for win and death outcomes and provide both automatic restart and manual restart paths.
- **FR-013**: The system MUST keep asteroids and player entities within containment bounds using reflection behavior and cap asteroid reflection speed to prevent runaway energy.
- **FR-014**: The system MUST keep the gameplay loop demoable at every milestone stage by preserving a playable state when progressing from flight to physics to combat to HUD/game flow.

### Key Entities *(include if feature involves data)*

- **MatchState**: Represents current game session state with four phases (Playing, PlayerDead, Victory, Restarting). Terminal phases (PlayerDead, Victory) display overlays and run auto-restart countdowns. Restarting clears entities and rebuilds the scene before transitioning back to Playing. Manual restart from any terminal phase skips the countdown. Tracks enemy remaining count and victory/defeat outcome.
- **PlayerState**: Represents player-controlled ship status, including position/orientation, movement intent, active weapon, health, shield, boost, and cooldown trackers.
- **EnemyState**: Represents each enemy ship, including pursuit intent, attack readiness, line-of-sight eligibility, health, shield, and alive/dead flag.
- **WeaponState**: Represents weapon configuration and runtime counters, including damage model, cooldown duration, last-fire timestamp, and projectile lifetime settings.
- **ProjectileState**: Represents plasma shots in flight, including spawn transform, velocity, remaining lifetime, and collision result.
- **AsteroidState**: Represents asteroid simulation objects, including size tier, relative mass class, movement state, and containment interaction metadata.
- **HUDState**: Represents user-visible combat indicators and overlays, including bars, crosshair visibility, weapon text, cooldown display, and terminal messaging.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: In 95% of test runs, players can start a match and control movement within 5 seconds of launch.
- **SC-002**: In 95% of test runs, weapon switching and firing inputs result in the expected weapon behavior and cooldown feedback within the next rendered frame.
- **SC-003**: In 100% of validated runs, match outcomes are deterministic for win/loss evaluation, with no contradictory terminal states.
- **SC-004**: At least 90% of test participants can correctly identify current health, shield, boost, and active weapon status from the HUD without external guidance.
- **SC-005**: In 95% of full-loop runs, players can complete a full gameplay cycle (start, engage combat, reach win or death, restart) without manual intervention or soft-lock.
- **SC-006**: Across a representative 10-minute play session, asteroid containment behavior prevents persistent out-of-bounds entities and visible runaway acceleration events.

## Assumptions

- The MVP baseline uses one enemy by default, with enemy-count scaling treated as post-MVP scope.
- The game workstream can begin against mock dependencies and later swap to real upstream interfaces without changing user-visible feature scope.
- Third-person camera is the default experience; first-person fallback is acceptable only if needed to preserve milestone predictability.
- A minimal explosion effect is sufficient for MVP as long as defeat feedback is clearly visible to players.
- The gameplay loop prioritizes behavioral correctness and demo reliability over advanced polish features.
- Audio, networking, particle systems, and post-processing are explicitly out of scope.
