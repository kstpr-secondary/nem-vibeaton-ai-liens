# Setup, Libraries, and Tasks

This document outlines the external libraries used in the project, their sources, and the process for deep examination and interface extraction.

## External Libraries

| Library | GitHub Repository | Samples / Sources | Role in Project |
| :--- | :--- | :--- | :--- |
| **sokol** | [floooh/sokol](https://github.com/floooh/sokol) | [sokol-samples](https://github.com/floooh/sokol-samples) | Core graphics (GFX), windowing (App), and time. |
| **sokol-shdc** | [floooh/sokol-tools](https://github.com/floooh/sokol-tools) | [sokol-tools-bin](https://github.com/floooh/sokol-tools-bin) | Shader cross-compiler for precompiling GLSL. |
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
- **Locate Headers:** Identify the primary header files (especially for STB-style libraries like `cgltf` or `sokol`).
- **Review Build Requirements:** Check if the library is header-only or requires a specific compilation step (e.g., `#define XXX_IMPLEMENTATION`).
- **Map to Milestones:** Identify which project milestones (e.g., R-M1, E-M2) rely on specific features of the library.

### 2. Deep Interface Extraction
- **Type Identification:** Extract core structs, enums, and typedefs. Map them to internal engine/renderer types if a wrapper is needed.
- **Function Inventory:** List the "entry point" functions (e.g., `sg_setup`, `cgltf_parse`, `entt::registry::create`).
- **Memory Management:** Determine who owns the memory (library vs. engine) and how to safely free resources (e.g., `cgltf_free`).
- **Error Handling:** Identify return codes or error callback mechanisms provided by the library.

### 3. Usage Pattern Extraction
- **Study Samples:** Analyze official samples (e.g., `sokol-samples`) to see the library "in action" within a similar context.
- **Snippet Creation:** Create minimal standalone snippets (or "sanity tests") to verify assumptions about the API before full integration.
- **Identify Bottlenecks:** Look for performance-critical paths or thread-safety limitations mentioned in the documentation.

### 4. Interface Definition (The "Bridge")
- **Create Shims:** If the library API is too verbose or leaks implementation details, design a thin C++ wrapper or "bridge" (as specified in the Engine/Renderer concept docs).
- **Public API Exposure:** Decide which parts of the library should be visible to the `game` layer and which should be hidden inside `engine` or `renderer`.
- **Documentation:** Briefly document the "why" and "how" of the integration in the corresponding `internal-spec.md` or `interface-spec.md`.

### 5. Validation & Mocking
- **Mock Generation:** For libraries used in downstream workstreams (like `sokol` for the `engine`), create mock versions of the interfaces to allow parallel development.
- **Integration Test:** Verify the extracted interface against a real asset or scenario (e.g., loading a sample glTF).
