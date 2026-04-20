This document explains the tentative milestones and MVPs for the renderer, game engine and the game outlined in the blueprint document.
This is a first working iteration. Note the following:
- The first version is written by a human which is not an expert in sokol gfx and low-level rendering.
- Milestones below don't list concrete actual tasks, but visual or high-grained criteria for milestone completion.
- The actual concrete tasks will be the artefact of the SpecKit's planning cycle, though some pre-refinement is expected. E.g. for the "- Forward rendering" criteria a description of the potential task can be added (setup buffers, setup render target, prepare render pass, etc.)

The milestones below are to be analyzed deeply for feasibility. 

## Rendering engine

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
- API for the game engine to enqueue which meshes to be drawn.
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
- Deferred or clustered forward rendering, multiple lights (10s to 1000s)
- Ambient occlusion
- Should be able to render a large number of objects (10s or 100s of 1000s), CPU techniques - GPU-instancing, draw call batching, OR potentially GPU-driven (MultiDrawIndirect), GPU frustum culling

> **Human comment** - Investigate if some of these optimizations can be slipped in in prior milestones (e.g. GPU-instancing, draw call batching).
**Expected outcome:** Not expected to reach, as noted some optimizations like GPU-instancing and batching can be added to other milestones if easy enogh to implement.

---

Implementation not covering the MVP is NOT acceptable, Milestones 3 and 4 are highly desirable, from 5 on - most likely unrealistic but still desirable. Consider all those milestones **extremely tentative** and **a subject to change**.


## Game engine MVP
**Milestones in progress**

### Milestone 1
- Game loop (update loop) in sync with the rendering loop 
- Way to create simple procedural shapes - spheres, cubes, capsules for usage in the game consumers. 
- Simple assets importing pipeline - import assets in a given format (obj, gltf). Ingest using some open source library like assimp or cgltf. 
- A scene should be controlled by a scene graph or ECS
- Ingested asset to scene object pipeline
- Way to associate a transform with mesh/material (scene node vs ECS association)
- Dynamic vs static objects in the scene


Expected outcome: After milestone 1 assets can be ingested and a procedural scene with a scene graph/ECS controlled and exposed by the engine can be built and manipulated by a consumer (game). IF Render engine Milestone 1 is ready at this point the scene can be visualized. 


### Milestone 2
- Move player (camera) - input handling and translation to camera transform
- Have one directional light
- Interactable vs uninteractable objects
- Raycasting and hit testing


Expected outcome: Input controlled camera in the procedural scene (ingested assets). IF Render engine Milestone 2 is ready at this point the lit scene can be visualized.  Hit testing is working.


### Milestone 3
- Simplest physics (Newton's 2nd law, applying forces, etc)
- Time tracking (time class)


### Milestone 4 - game engine optional:
- Have multiple other lights (used for explosions, space ship light beams?)
- Pathfinding/AI
- More complex physics


No animation system (bones, skins, etc.), no networking, no post-processing, no particle systems.



## Game Concept


The game should be a space shooter (spaceships shoot each other), entirely 3D, happening in an asteroid field. Asteroids are with different sizes. Space ships have 3 attacks:
- Laser beam - instantaneous hit, mid damage, sniper-like, very slow recharge (10s), visualization is flashing and then fading straight line. Can give impulse to small asteroids. Default is zero laser guns, can be upgraded to 2 alternating ones with power-ups and damage can be increased by 50% (per individual beam).
- Plasma gun - shoots high-intensity burst of low damage plasma (glowing) projectiles. Weakest gun, plasma projectiles travel fast, but not instantaneous, fast enemies can try to avoid them. Default is just one gun, can be upgraded to 4 with power-ups. Damage can be increased by 50% per indivudual gun
- Rocket launcher - slow, but high damage, mid recharge time (2 sec). Up to 2 launchers by power ups, default is 0. More easily dodged than plasma gun fire.


Health and shields:
- Every ship has a number of hit points
- Hit points can be increased by power-ups
- Damage by enemy hits subtracts hit points from the total amount
- Hit points can be regenerated in several ways - small space stations which when approached regenerate hit points and cast an unpenetrable shield on the ship - e.g. regenerate for 15 sec (fixed num points) then shield is down on the ship and it's vulnerable again. Another way to regenerate hit points is by passing through a consumable - targeting some asteroids that can drop power-ups can drop a hit points consumable.
- Shield points are analogous.


Dynamics:
- The asteroids in the field can be moved by explosions and can cause damage to ships. Large asteroids are much harder to move.
- The asteroids are contained by a spherical (hexagonal tiles sphere, ghostly glow if nearby) containment field that's large enough for the game to feel in space. 
- If an asteroid hits the containment hex tile it's reflected back. Thus the total energy in the system is increasing, so there are damper objects in the play area - when hit by an asteroid they absorb 50% of the kinetic energy slowing it down. 
- If a ship hits the field it's reflected back a bit without damage to the ship.


Power-ups and other aspects are to be discussed later. 
MVP and milestones are to be discussed later. 
