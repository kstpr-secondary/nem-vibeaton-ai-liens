# Setup, Libraries, and Tasks (Fixed Iter 9)

This document outlines the external libraries used in the project, their sources, and the process for deep examination and interface extraction, updated for the **OpenGL 3.3 Core** and **sokol-shdc** workflow.

## External Libraries

| Library | GitHub Repository | Samples / Sources | Role in Project |
| :--- | :--- | :--- | :--- |
| **sokol** | [floooh/sokol](https://github.com/floooh/sokol) | [sokol-samples](https://github.com/floooh/sokol-samples) | Core graphics (**OpenGL 3.3 Core only**), windowing (App), and time. |
| **sokol-shdc** | [floooh/sokol-tools](https://github.com/floooh/sokol-tools) | [sokol-tools-bin](https://github.com/floooh/sokol-tools-bin) | **Mandatory** shader cross-compiler. Precompiles GLSL into C headers. |
| **EnTT** | [skypjack/entt](https://github.com/skypjack/entt) | [EnTT Wiki](https://github.com/skypjack/entt/wiki) | Entity Component System (ECS) for engine logic. |
| **cgltf** | [jkuhlmann/cgltf](https://github.com/jkuhlmann/cgltf) | [cgltf.h](https://github.com/jkuhlmann/cgltf/blob/master/cgltf.h) | Single-file glTF 2.0 loader. |
| **tinyobjloader**| [tinyobjloader/tinyobjloader](https://github.com/tinyobjloader/tinyobjloader) | [tiny_obj_loader.h](https://github.com/tinyobjloader/tinyobjloader/blob/master/tiny_obj_loader.h) | Wavefront OBJ loader (fallback). |
| **GLM** | [g-truc/glm](https://github.com/g-truc/glm) | [GLM Manual](https://github.com/g-truc/glm/blob/master/manual.md) | OpenGL Mathematics (header-only). |
| **Dear ImGui** | [ocornut/imgui](https://github.com/ocornut/imgui) | [imgui_demo.cpp](https://github.com/ocornut/imgui/blob/master/imgui_demo.cpp) | Immediate mode UI for HUD and debugging. |
| **stb_image** | [nothings/stb](https://github.com/nothings/stb) | [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h) | Image loading and decoding. |
| **Catch2** | [catchorg/Catch2](https://github.com/catchorg/Catch2) | [Catch2 Examples](https://github.com/catchorg/Catch2/tree/devel/examples) | Unit testing framework. |

## Library Examination and Interface Extraction Steps

To ensure seamless integration and maintainable code, follow these steps for each library:

### 1. Source Analysis & Capability Mapping
- **Locate Headers:** Identify the primary header files. For shaders, identify the `.glsl` source and the generated `.glsl.h` header location.
- **Review Build Requirements:** All shaders must use `sokol-shdc` annotations (`@vs`, `@fs`, `@block`, etc.).
- **Map to Milestones:** Focus strictly on **OpenGL 3.3 Core** capabilities. Vulkan-specific features are cut.

### 2. Deep Interface Extraction
- **Type Identification:** Extract core structs. For `sokol_gfx`, focus on `sg_shader_desc` and pipeline creation via generated shader headers.
- **Function Inventory:** Inventory functions like `sg_setup`, `sg_make_shader` (using `shd_xxx_shader_desc`), and `sg_make_pipeline`.
- **Memory Management:** Ensure `cgltf_free` and `sg_shutdown` patterns are clear.
- **Error Handling:** `sokol_gfx` pipeline failures should log errors and fall back to a magenta error shader (§3.3).

### 3. Usage Pattern Extraction
- **Study Samples:** Focus on samples using `sokol-shdc`.
- **Snippet Creation:** Verify `sokol-shdc` output inclusion and usage: `#include "shaders/xxx.glsl.h"`.
- **Identify Bottlenecks:** Note that shader loading is now a compile-time dependency, not a runtime one.

### 4. Interface Definition (The "Bridge")
- **Create Shims:** Design C++ wrappers that hide library-specific verbosity.
- **Path Resolution:** Use the `ASSET_ROOT` macro (from generated `paths.h`) for all runtime file loading (textures, meshes). **Never** use relative paths.
- **Public API Exposure:** `renderer` and `engine` are static libs. Standalone drivers (`renderer_app`, `engine_app`) test these APIs in isolation.

### 5. Validation & Mocking
- **Mock Generation:** Create mock versions of the `renderer` and `engine` public APIs for downstream parallel work.
- **Integration Test:** Use the standalone driver apps to verify library integration without the full game loop.

## Library Skill Creation Tasks (Fixed Iter 9)

As per §8.10 of the **Hackathon Master Blueprint**, every major library requires a `SKILL.md` and per-aspect reference files to minimize context usage.

| ID | Task | Tier | Status | Owner | Depends_on | Milestone | Parallel_Group | Validation | Notes |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| LIB-01 | Create `sokol-api/SKILL.md` and aspect references | HIGH | TODO | Systems Architect | NONE | PRE-HACK | PG-LIB-A | REVIEW | Files: descriptors, buffers, render-passes, draw-calls, app-time |
| LIB-02 | Create `entt-ecs/SKILL.md` and aspect references | MED | TODO | Systems Architect | NONE | PRE-HACK | PG-LIB-A | SELF-CHECK | Files: registry-basics, component-views, entity-lifecycle |
| LIB-03 | Create `cgltf-loading/SKILL.md` and aspect references | MED | TODO | Systems Architect | NONE | PRE-HACK | PG-LIB-B | SELF-CHECK | Files: mesh-extraction, material-data |
| LIB-04 | Create `tinyobj-loading/SKILL.md` and aspect references | LOW | TODO | Systems Architect | NONE | PRE-HACK | PG-LIB-B | SELF-CHECK | Files: tinyobj-mesh-extraction |
| LIB-05 | Create `glm-math/SKILL.md` and aspect references | LOW | TODO | Systems Architect | NONE | PRE-HACK | PG-LIB-C | SELF-CHECK | Files: common-transforms, quaternions |
| LIB-06 | Create `imgui-hud/SKILL.md` and aspect references | MED | TODO | Systems Architect | NONE | PRE-HACK | PG-LIB-C | SELF-CHECK | Files: basic-widgets, sokol-integration |
| LIB-07 | Create `catch2-testing/SKILL.md` and aspect references | LOW | TODO | Systems Architect | NONE | PRE-HACK | PG-LIB-D | SELF-CHECK | Files: assertions-and-sections |
| LIB-08 | Create `stb-image-loading/SKILL.md` and reference | LOW | TODO | Systems Architect | NONE | PRE-HACK | PG-LIB-D | SELF-CHECK | Files: stb-image-usage |
| LIB-09 | Create `glsl-patterns/SKILL.md` and aspect references | MED | TODO | Systems Architect | NONE | PRE-HACK | PG-LIB-E | REVIEW | Files: pbr-basics, lighting-models |
| LIB-10 | Create `physics-euler/SKILL.md` and reference | MED | TODO | Systems Architect | NONE | PRE-HACK | PG-LIB-E | SELF-CHECK | Files: euler-integration-patterns |
| LIB-11 | Create `sokol-shdc/SKILL.md` | MED | TODO | Systems Architect | NONE | PRE-HACK | PG-LIB-E | REVIEW | Covers @vs, @fs, @block annotations and CMake usage |


