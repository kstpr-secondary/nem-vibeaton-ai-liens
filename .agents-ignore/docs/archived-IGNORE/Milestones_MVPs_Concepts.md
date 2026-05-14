This document explains the tentative milestones and MVPs for the renderer, game engine and the game outlined in the blueprint document.
This is a first working iteration. Note the following:
- The first version is written by a human which is not an expert in sokol gfx and low-level rendering.
- Milestones below don't list concrete actual tasks, but visual or high-grained criteria for milestone completion.
- The actual concrete tasks will be the artefact of the SpecKit's planning cycle, though some pre-refinement is expected. E.g. for the "- Forward rendering" criteria a description of the potential task can be added (setup buffers, setup render target, prepare render pass, etc.)

The milestones below are to be analyzed deeply for feasibility. 


## Rendering engine

A simple, but effective to demo rendering engine with capabilities or around early 2010s.

---

### Milestone Group MVP - simple rendering and culling

### Milestone 0 - Low level Setup and Preparation
Tasks in this milestone should result to working:
- Window management is clear
- Resolution is fixed, fullscreen rendering
- Single platform - Vulkan experimental 
- Setup of a sokol frame - devices, descriptors, buffers, swap chains (whichever is applicable) and so on
- Render loop and input handling from sokol

**Expected outcome:** After milestone 0 actual graphics work can begin. The app should render a black screen. Milestone 0 can be skipped as a sync point.

### Milestone 1 - Working Rendering
Tasks in this milestone should result to working:
- Forward rendering
- Perspective camera
- Shader objects compilation
- Unlit solid color shading
- Opaque objects render queue
- API for the game engine to enqueue meshes to be drawn with given visual characteristics
- Procedural scene - **low effort only, no sophisticated scene generation** - a bunch of (hardcoded number) of colored unlit spheres, cubes, capsules with different sizes and orientations should be generated in front of a fixed camera.

**Expected outcome:** After milestone 1 an unoptimized (unsorted, no culling) procedural scene (spheres, cubes, capsules) should be rendered. During this milestone it will be decided if the experimental Vulkan backend is viable or we should fall back to OpenGL. Milestone 1 is a sync point for the game engine. After syncing the game engine can pass meshes/mesh ids with transforms so they are rendered by the engine for each frame.

### Milestone 2 - Basic Lighting and Shading MVP
- Frustum culling
- Scene front to back sorting 
- Directional light source
- Simple shading - Lambertian, material base color parameter
- Should be able to render a decent number of colored objects (high 100s, low 1000s) - stress test
- Render lines (e.g. laser beams)
- Background skybox (from equilateral image or cubemap) rendering

**Expected outcome:** After milestone 2 procedural scene of simply-shaded objects correctly shaded by a directional light should be visualized. Simple animated camera movement (e.g. circular over the scene, looking at the scene) in the procedural scene shows that shading works as expected, skybox is visualized. Stress test - increasing number of test objects (hardcoded, not via UI) to test fps with increasing number of draw calls and geometry primitives. MVP Complete - Milestone 2 is the MVP! Game can render background, objects and laser lines.

--- 

### Milestone Group Desirable Goals - Beyond Basic Graphcs + optimization

### Milestone 3 - First Non-basic Material Model
- Simple Blinn-Phong shading. Material models. Variying of smoothness/roughness.
- Texture mapping - diffuse only textures (albedo)
- Being able to apply custom shaders from the game engine (e.g. for explosions, plasma pulses, fresnel shading, special effects, etc). These shaders should be applied to a quad or a simple object - cube, sphere. No shaders are expected to actually be generated in this task.

**Expected outcome:** After milestone 3 objects with different reflectance and textures should be rendered over a static skybox in the procedural scene. Depth-tested lines should be rendered in the scene with unlit shader.

### Milestone 4 - Transparency, Detail and Optimization
- LOD meshes
- Alpha cutout (alpha-tested) and transparency (alpha-blended) render queues
- Better skybox - procedural sky / night sky (stars, nebulae) generated in a shader
- Normal maps

**Expected outcome:** After milestone 4 meshes should change LODs when distant enough. Test with procedural scene with low and high poly spheres for LODs. Procedural scene wtih alpha-tested and alpha-blended materials on top of diffuse textures and materials with various reflectivity is visualized. A nicer procedural skybox is rendered. Objects with normal maps show proper surface detail. 
> **Human comment:** Feel free to downgrade the procedural sky to a stretch goal if another objective seems to be higher priority/more realistic.

---

### Milestone Group Stretch Goals - Last Minute Rush or Unrealistic

### Milestone 5 - Last Realistic Visual Improvements 
- Should be able to render shadows (usually advised agains, but a very simple sample exists in Sokol's github, can be adapted to our needs)
- Basic PBR - Cook-Torrence, no subsurface scattering, no cloth
- Environment lighting
- Environment reflections via IBL

**Expected outcome:** Better materials are visualized, shadows are cast by objects in the scene. Objects reflect and are affected by the skybox. 

> **Human comment**: I expect to be able to explore at least some of those goals in the last hour of the hackathon, but only in the case of everything else working, if there's still agent quota left, and if other workstreams are not requiring resource (human + agnets) reschedule. Somewhat realistic - shadows from the shadows sample adaptation, PBR model.

### Milestone 6 - Unrealistic goals
- MSAA levels
- Deferred or clustered forward rendering, multiple lights (10s to 100s)
- Ambient occlusion
- Should be able to render a large number of objects (10s or 100s of 1000s), CPU techniques - GPU-instancing, draw call batching, OR potentially GPU-driven (MultiDrawIndirect), GPU frustum culling

> **Human comment** - Investigate if some of these optimizations can be slipped in in prior milestones (e.g. GPU-instancing, draw call batching).
**Expected outcome:** Not expected to reach, as noted some optimizations like GPU-instancing and batching can be added to other milestones if easy enogh to implement.

---

Implementation not covering the MVP is NOT acceptable, Milestones 3 and 4 are highly desirable, from 5 on - most likely unrealistic but still desirable. Consider all those milestones **extremely tentative** and **a subject to change**.

---

## Game Engine

A simple game engine catering to the space shooter in an asteroid field game. Should be able to load assets and build a procedural scene. Should be able to run a simulation of N physical bodies (asteroids) that have mass and velocity, float in zero gravity and collide with each other. Should be able to do very simple pathfinding (e.g. for enemies). What the engine won't do - no animation system (bones, skins, etc.), no networking, no post-processing, no particle systems.

---

### Milestone Group MVP

### Milestone 1 Assets Ingestion to Scene Building
- Game loop (update loop) in sync with the rendering loop - comes from sokol; potentially 
- Way to create a simple procedural shapes - spheres, cubes, capsules for usage in the game consumers. 
- Simple assets importing pipeline - import assets in a given format (obj, gltf) from a relative path (e.g. `/assets` folder in the root of the project). Ingest assets using cgltf, simple obj as a fallback. Assets will be gathered in advance. 
- A scene should be controlled by an ECS by the entt library.
- Have one directional light and one perspective camera
- A component combination schema should be proposed for expected objects: moving objects with mass (e.g. mesh, transform (position, rotation), mass, collider, linear velocity, anglular velocity, moment of inertia), immovable object (mesh, transform), player object (spaceship with mass, controlled by thrust). Only component schemas are created. 
- An API should expose the scene to the game. Component schema abstraction should be exposed (movable with mass, immovable, etc.)
- Ingested asset to scene object pipeline
- A procedural scene using a combination of ingested assets and procedural shapes should be built.

> **Human comment** - The component schemas are highly speculative, a better ones should be proposed. API should be specified if it should expose entt components or only game engine abstractions. A procedural scene with simple procedural shapes is also used in the renderer. It should be decided if some of the functionality can/should be reused or if more friction is added if reusing - simply duplicated. 

**Expected outcome:** After Milestone 1 assets can be ingested and a scene with a ECS controlled and exposed (abstraction or raw) by the engine that can be built and manipulated by a consumer (game). A test procedural scene should be built in the game engine for local tests. If Render Engine Milestone 1 is ready at this point the scene can be visualized. Every other render engine milestone after 1 adds only better visualization/optimizations. 

### Milestone 2 Movement, Raycasting and Collision detection
- Move player (camera) - input handling and translation to camera transform
- Interactable vs uninteractable objects
- Dynamic vs static objects in the scene
- Simple Colliders creation (AABB), 
- Raycasting and hit testing: ray-collider hit detection
- Collision detection algorithms in place - collider-collider hit detection

**Expected outcome:** Input controlled camera in the procedural scene build with ingested assets and procedural shapes. Collision between camera and objects and hit testing in the scene - camera collider can't pass through objects. Hit testing - click on an object translates to ray-object hit test. If render engine Milestone 2 is ready at this point the lit scene can be visualized. 

### Milestone 3 Physics and Navigation
- Simplest physics engine - Euler - moving objects, Newton's 2nd law, applying forces to each other, ellastic collision acting on all entities with physics-compatible components
- Time tracking by sokol, API access to time for the game
- Pathfinding/AI - object seeks end position in a dynamic scene algorihm, simplest possible (enemies homing at player)

**Expected outcome**: Input controlled camera in the procedural scene where dynamic ingested objects collide and bounce from each other in a reallistic way and with static objects (procedural shapes) in the scene. 

---

### Milestone Group - Stretch Goals

### Milestone 4 - Pathfinding 

- Advanced pathfinding and obstacle avoidance algorithms
- Better colliders - convex hull colliders, collision algorithms improved
- Better collision detection - brute force to spatial partitioning optimized, acceleration structures
- Have multiple other point lights in the scene (e.g. used for explosions in the game)

**Expected outcome**: Stretch goals, unrealistic in the time frame

---

> **Human comment** - most likely some concepts should be cut from the MVP and moved to stretch goals

---

## Game Concept


The game should be a space shooter (spaceships shoot each other), entirely 3D, happening in an asteroid field. Asteroids are with different sizes. The game will be a third-person shooter. The spaceship is visible. The space navigation, shooting mechanics and to some extent - visuals, should be influenced by the game Freelancer from 2003.

Space ships - several asset models. Same initial health and shield.

### Stats
- Health - O to 100. If shield is 0 it's drained when shot at or in collisions with asteroids. Otherwise the shield is drained. Very slowly regenerates. If power ups are used it can be regenerated faster.
- Shield - 0 to 100. Drained when shot at or in collisions with asteroids. If power ups are used it can be regenerated.
- Boost Speed - 0 to 100. Move 2x faster if a button trigger is pressed. Drains to 0 when consumed, drains for 5 seconds. Then regenerates at 4x time as its drained (20s). Used mostly for evading enemies.

### Attacks

Space ships have 2 or more attacks, one at a time:
- Laser beam (railgun) - instantaneous hit, mid damage, sniper-like, like railgun in Quake 2 and Quake 3. Very slow recharge cooldown (10s), visualization is flashing and then fading straight line. Can give impulse to small asteroids. Default is zero laser guns if power-ups will be implemented or one if not.
- Plasma gun - shoots a high-intensity burst of low damage plasma (glowing) projectiles. Plasma gun from quake is similar. Weakest gun, plasma projectiles travel fast, but not instantaneous, fast enemies can try to avoid them. Default is just one gun if no power-ups and up to 4 if power-ups are implemented.

Optional:
- Rocket launcher - slow, but high damage, mid recharge time (2 sec), shoots homing missiles. Somewhat easier to avoid than plasma gun. One launcher if no power ups, 2 if power ups are implemented.  

### Power-ups (optional)
Power-ups are consumables that can be randomly found with some probability when enemies are killed or are spawned randomly in the scene. Power-ups are gathered by passing through them.

Visualization: Can be simple glowing spheres with different color. 

- Laser gun - Can be upgraded to 2 alternating guns by taking a power-up and damage can be increased by 50% (per individual beam) by next passing through the same power-up. Potential visualization: Green glowing sphere.
- Plasma gun - Two power-ups can be upgraded to 4 with power-ups. Damage can be increased by 50% per indivudual gun. Potential visualization: Cyan glowing sphere.
- Rocket launcher -  Up to 2 launchers by power ups.
- Health - +20 health (e.g. if on 15 health add +10 health to 25 health). Cannot exceed max health.
- Extra health - + 10 health over max (100). Rarer than normal health. Extra health can increase the health maximum just when taken, but not permanently. If damage is taken to below 100 health and standard health is taken then it can't go pass 100 again.
- Shield and extra shield - similar to health.
- Extra Boost Speed - same effect on speed as normal boost, but normal boost is consumed 2x as slow / is regenerated 2x as fast for the first 20 seconds after the power up is used (e.g. if boost speed is continuously used right after the power up it's taking 10s to use up instead of 5. Then it's regenerated 2x as fast for the next 10 seconds.).

### Dynamics
- Some asteroids in the field have linear and angular velocity. Smaller asteroids have higher velocity.
- The asteroids in the field can be moved by explosions (rocket launcher or when an enemy is destroyed) and direct fire like plasma and laser (albeit slowly) and can cause damage to ships if not avoided. Large asteroids are much harder to move.
- The asteroids are contained by a spherical (optional ghostly glow if nearby) containment field that's large enough for the game to feel in space, e.g. 1 km. 
- If an asteroid hits the containment field it's reflected back. Thus the total energy in the system is increasing, so there are damper objects in the play area - when hit by an asteroid they absorb 50% of the kinetic energy slowing it down. 
- If a ship hits the field it's reflected back a bit without damage to the ship.

### Game Mechanics
- Spaceship can move (strafe) by WASD. Forward vector control - left mouse click + hold.
- Space to boost speed.
- Right mouse to shoot weapon.
- Weapons buttons - Q switches to plasma, E switches to laser/railgun, R switches to rocket launcher (if implemented).
- Esc exits the game.
- Enter restarts the game.
- If the player dies the game is also restarted.

### UI
Minimum UI
- Health bar, 0 to 100, red color, if extra health can be extended to 200
- Shield bar, 0 to 100, blue color, if extra shield can be extended to 200
- Speed boost bar, 0 to 100, yellow. Light blue if boost speed power-up is consumed
- Crosshair on cursor position.