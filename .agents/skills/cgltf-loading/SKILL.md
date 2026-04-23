---
name: cgltf-loading
description: Use when implementing or reviewing glTF/GLB asset import in the game-engine workstream. Covers cgltf single-header setup, the mandatory parse→load_buffers→walk lifecycle, accessor unpacking for positions/normals/UVs/indices, the mesh-upload bridge to the renderer's upload_mesh, and GLB vs glTF path handling. Activated when writing or reviewing code in asset_import.{cpp,h} or asset_bridge.{cpp,h}. Do NOT use for OBJ loading (tinyobjloader skill) or renderer mesh building (renderer-specialist).
compatibility: Portable across heterogeneous agents (Claude, Copilot, Gemini, GLM, local Qwen).
metadata:
  author: hackathon-team
  version: "1.1"
  project-stage: pre-implementation
  role: library-reference
  activated-by: cgltf_parse_file or cgltf_load_buffers usage
---

# cgltf-loading Skill

Reference for `cgltf.h` v1.14+ used in the game-engine workstream (`src/engine/`).
**Task scope: load up to 10 GLB assets from `ASSET_ROOT`. No animation, no skinning, no morph targets — those are hard-cut from scope.**

Use per-aspect references under `references/` when you need deeper detail. Read only what the task requires.

---

## 1. Core Rules (must know)

1. **Parse does NOT load mesh data.** `cgltf_parse_file` reads only the JSON header. **You must call `cgltf_load_buffers` before accessing any buffer data** — skipping it yields silent NULL dereferences or garbled geometry with no error code.
2. **Single-header, STB style.** One `.cpp` file must `#define CGLTF_IMPLEMENTATION` before the include. All other files include without the macro.
3. **`cgltf_free` is always required.** Call it on a non-NULL `cgltf_data*` even if an intermediate step fails. Never free the individual buffers; `cgltf_free` handles them.
4. **`ASSET_ROOT` macro for all paths.** Never hardcode relative paths. Compose paths as `std::string(ASSET_ROOT) + "/filename.glb"`.
5. **GLB (binary) and glTF (JSON) are both supported** by the same API. GLB embeds its buffer; glTF references external `.bin` files resolved relative to the provided base path. For this project, prefer GLB.
6. **Accessor unpacking is the correct extraction path.** Do not index `buffer_view->buffer->data` directly with hand-computed strides — use `cgltf_accessor_unpack_floats` and `cgltf_accessor_unpack_indices` (or `cgltf_accessor_read_index`) which handle sparse accessors, stride, and component-type conversion.
7. **Only triangles.** Filter primitives by `prim.type == cgltf_primitive_type_triangles`. Skip others silently; log if any are skipped.
8. **Always check `cgltf_result`.** Compare against `cgltf_result_success` (== 0) after every call.

---

## 2. Include Setup

```cpp
// asset_import.cpp — exactly one translation unit
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

// All other files that need the types
#include "cgltf.h"
```

---

## 3. Load Lifecycle

```cpp
#include "cgltf.h"
#include <string>

// Returns nullptr on failure; caller must call cgltf_free on success.
cgltf_data* cgltf_open(const char* path) {
    cgltf_options opts = {};          // zero-init → defaults (heap allocator, no extensions)
    cgltf_data* data   = nullptr;

    cgltf_result r = cgltf_parse_file(&opts, path, &data);
    if (r != cgltf_result_success) {
        // data is NULL here; no free needed
        return nullptr;
    }

    // MANDATORY: parse only reads the JSON header; buffers (geometry) are not loaded yet.
    r = cgltf_load_buffers(&opts, data, path);
    if (r != cgltf_result_success) {
        cgltf_free(data);
        return nullptr;
    }

    return data;  // caller owns; must call cgltf_free(data)
}
```

**Path for GLB:** pass the `.glb` file path as both `path` arguments — `cgltf_load_buffers` recognizes the embedded BIN chunk and needs the file path only to resolve any external URIs (which GLB has none of).

---

## 4. Data Extraction Quick Reference

| Task | Pattern |
|------|---------|
| **Mesh count** | `data->meshes_count` |
| **Get primitive** | `data->meshes[i].primitives[j]` |
| **Find attribute** | `cgltf_find_accessor(prim, cgltf_attribute_type_position, 0)` |
| **Read floats (bulk)** | `cgltf_accessor_unpack_floats(acc, buffer, count)` |
| **Read indices (bulk)** | `cgltf_accessor_unpack_indices(acc, buffer, sizeof(uint32_t), count)` |
| **Read single index** | `cgltf_accessor_read_index(acc, i)` |
| **Component count** | `cgltf_num_components(acc->type)` (VEC3 → 3, VEC2 → 2, …) |
| **Global transform** | `cgltf_node_transform_world(node, matrix_float16)` |
| **Buffer view data** | `cgltf_buffer_view_data(bv)` (returns `const void*`) |

**Attribute types:** `cgltf_attribute_type_position`, `_normal`, `_texcoord`, `_color`, `_tangent`.
**Primitive type:** `cgltf_primitive_type_triangles` (standard; filter others).

---

## 5. Walking Meshes and Primitives

```cpp
void walk_meshes(const cgltf_data* data) {
    for (size_t m = 0; m < data->meshes_count; ++m) {
        const cgltf_mesh& mesh = data->meshes[m];
        for (size_t p = 0; p < mesh.primitives_count; ++p) {
            const cgltf_primitive& prim = mesh.primitives[p];
            if (prim.type != cgltf_primitive_type_triangles) continue; // skip non-tri
            // process prim — see §6 and §7
        }
    }
}
```

A single glTF mesh can have multiple primitives (sub-meshes), each with its own material. For MVP, produce one renderer mesh handle per primitive.

---

## 6. Extracting Vertex Attributes

```cpp
struct VertexData {
    std::vector<float> positions; // flat: x,y,z,...
    std::vector<float> normals;   // flat: nx,ny,nz,... (same count as positions)
    std::vector<float> uvs;       // flat: u,v,...       (same count, or empty)
};

VertexData extract_vertices(const cgltf_primitive& prim) {
    VertexData out;
    cgltf_accessor* pos_acc  = nullptr;
    cgltf_accessor* norm_acc = nullptr;
    cgltf_accessor* uv_acc   = nullptr;

    for (size_t a = 0; a < prim.attributes_count; ++a) {
        const cgltf_attribute& attr = prim.attributes[a];
        switch (attr.type) {
            case cgltf_attribute_type_position: pos_acc  = attr.data; break;
            case cgltf_attribute_type_normal:   norm_acc = attr.data; break;
            case cgltf_attribute_type_texcoord: uv_acc   = attr.data; break; // TEXCOORD_0
            default: break;
        }
    }
    // Alternatively use the helper: cgltf_find_accessor(&prim, cgltf_attribute_type_position, 0)

    if (!pos_acc) return out;  // degenerate primitive; caller skips

    size_t n = pos_acc->count;

    // Positions — vec3
    out.positions.resize(n * 3);
    cgltf_accessor_unpack_floats(pos_acc, out.positions.data(), n * 3);

    // Normals — vec3; synthesize flat normals downstream if absent
    if (norm_acc && norm_acc->count == n) {
        out.normals.resize(n * 3);
        cgltf_accessor_unpack_floats(norm_acc, out.normals.data(), n * 3);
    }

    // UVs — vec2
    if (uv_acc && uv_acc->count == n) {
        out.uvs.resize(n * 2);
        cgltf_accessor_unpack_floats(uv_acc, out.uvs.data(), n * 2);
    }

    return out;
}
```

`cgltf_accessor_unpack_floats` signature:
```c
cgltf_size cgltf_accessor_unpack_floats(const cgltf_accessor* accessor,
                                         cgltf_float* out,
                                         cgltf_size float_count);
// Returns number of floats written; 0 on error.
// float_count must equal accessor->count * cgltf_num_components(accessor->type).
```

---

## 7. Extracting Indices

Prefer the bulk `cgltf_accessor_unpack_indices` over the per-element loop for performance:

```cpp
// Preferred: bulk unpack (handles UBYTE, USHORT, UINT component types transparently)
std::vector<uint32_t> extract_indices(const cgltf_primitive& prim) {
    std::vector<uint32_t> idx;
    if (!prim.indices) return idx;  // unindexed mesh; caller generates sequential indices

    size_t n = prim.indices->count;
    idx.resize(n);
    cgltf_accessor_unpack_indices(prim.indices, idx.data(), sizeof(uint32_t), n);
    return idx;
}

// Alternative: per-element loop (cgltf_accessor_read_index returns size_t)
for (size_t i = 0; i < n; ++i)
    idx[i] = static_cast<uint32_t>(cgltf_accessor_read_index(prim.indices, i));
```

Both handle `UNSIGNED_BYTE`, `UNSIGNED_SHORT`, and `UNSIGNED_INT` component types transparently.

---

## 8. Mesh-Upload Bridge (cgltf → renderer)

The engine is responsible for converting `VertexData` into the interleaved layout the renderer's `upload_mesh` expects. The renderer's exact layout is defined in `docs/interfaces/renderer-interface-spec.md` — read that spec before implementing the bridge. The pattern below assumes an interleaved `{position(3), normal(3), uv(2)}` layout; adjust to the frozen spec.

```cpp
// asset_bridge.cpp
#include "asset_bridge.h"
#include "asset_import.h"     // VertexData, extract_vertices, extract_indices
#include <renderer/renderer.h> // renderer.upload_mesh(...)

mesh_handle upload_primitive_to_renderer(Renderer& renderer,
                                          const cgltf_primitive& prim) {
    auto vdata = extract_vertices(prim);
    auto idata = extract_indices(prim);

    if (vdata.positions.empty()) return INVALID_MESH_HANDLE;

    size_t n = vdata.positions.size() / 3;

    // Build interleaved buffer matching the renderer's expected layout.
    // Adjust stride/offsets to the frozen renderer-interface-spec.
    struct Vertex { float x,y,z, nx,ny,nz, u,v; };
    std::vector<Vertex> verts(n);
    for (size_t i = 0; i < n; ++i) {
        verts[i].x  = vdata.positions[i*3+0];
        verts[i].y  = vdata.positions[i*3+1];
        verts[i].z  = vdata.positions[i*3+2];
        verts[i].nx = vdata.normals.size()  == n*3 ? vdata.normals[i*3+0] : 0.f;
        verts[i].ny = vdata.normals.size()  == n*3 ? vdata.normals[i*3+1] : 1.f;
        verts[i].nz = vdata.normals.size()  == n*3 ? vdata.normals[i*3+2] : 0.f;
        verts[i].u  = vdata.uvs.size()      == n*2 ? vdata.uvs[i*2+0]     : 0.f;
        verts[i].v  = vdata.uvs.size()      == n*2 ? vdata.uvs[i*2+1]     : 0.f;
    }

    return renderer.upload_mesh(verts.data(), verts.size() * sizeof(Vertex),
                                idata.data(), idata.size());
}
```

**Return on failure:** return an invalid/sentinel mesh handle (defined by the renderer's API). Never crash; log the failure.

---

## 9. `load_gltf` Public API — Full Pattern

```cpp
// src/engine/asset_import.cpp
mesh_handle Engine::load_gltf(const char* relative_path) {
    std::string full_path = std::string(ASSET_ROOT) + "/" + relative_path;

    cgltf_data* data = cgltf_open(full_path.c_str());
    if (!data) {
        // log error
        return INVALID_MESH_HANDLE;
    }

    // For MVP: return the mesh handle from the first triangle primitive found.
    // Multi-primitive / multi-mesh glTF: extend to return a list of handles.
    mesh_handle result = INVALID_MESH_HANDLE;
    for (size_t m = 0; m < data->meshes_count && result == INVALID_MESH_HANDLE; ++m) {
        for (size_t p = 0; p < data->meshes[m].primitives_count; ++p) {
            const auto& prim = data->meshes[m].primitives[p];
            if (prim.type != cgltf_primitive_type_triangles) continue;
            result = upload_primitive_to_renderer(renderer_, prim);
            break;
        }
    }

    cgltf_free(data);  // always free after upload; GPU owns the data now
    return result;
}
```

`spawn_from_asset(path, pos, rot)` composes `load_gltf` + `create_entity()` + emplace `Transform`, `Mesh`, `Material` components. Implemented in `scene_api.cpp`, not `asset_import.cpp`.

---

## 10. File Ownership (E-M2 Parallel Group)

Per `Game Engine Concept and Milestones.md` E-M2 and AGENTS.md §7:

| File | Parallel group | Notes |
|------|---------------|-------|
| `src/engine/asset_import.{cpp,h}` | PG-E-M2-A | cgltf path: `cgltf_open`, `extract_vertices`, `extract_indices` |
| `src/engine/asset_bridge.{cpp,h}` | PG-E-M2-A | Upload bridge: `upload_primitive_to_renderer` |
| `src/engine/obj_import.{cpp,h}` | PG-E-M2-B | tinyobjloader path — disjoint from cgltf files |
| `src/engine/scene_api.cpp` | BOTTLENECK | `spawn_from_asset` wires both loaders; touches scene API owned by E-M1 |

cgltf (PG-A) and OBJ (PG-B) are file-disjoint → safe to run in parallel. The `spawn_from_asset` entry point in `scene_api.cpp` is a BOTTLENECK because it calls both loaders and touches entt.

---

## 11. Gotchas

- **`cgltf_load_buffers` must be called.** This is the #1 source of silent bugs — parse succeeds, accessor data is NULL, first `cgltf_accessor_unpack_floats` writes nothing. Always call `load_buffers` before any accessor read.
- **GLB embeds the buffer.** When loading a `.glb`, pass the GLB file path as `base_path` in `cgltf_load_buffers`; it uses it only to resolve external URIs (absent in GLB). Do not pass NULL.
- **`cgltf_accessor_unpack_floats` float count.** Must be exactly `accessor->count * cgltf_num_components(accessor->type)`. Under-counting produces partial reads; over-counting writes garbage past the buffer.
- **Missing normals.** Many exported GLBs from Blender omit normals if the mesh has smooth shading baked. Detect missing normals and synthesize flat normals (cross product of triangle edges) or log a warning. The Lambertian shader will render solid black otherwise.
- **UV handedness.** glTF UV origin is top-left; OpenGL textures conventionally bottom-left. Flip V: `v_gl = 1.0 - v_gltf`. Apply at extraction time, not in the shader.
- **`cgltf_free` frees everything.** The `cgltf_data` owns all parsed strings, buffers, and sub-structures. After calling it, all pointers into the data tree are invalid. Upload to GPU before freeing.
- **No animation / skinning.** `data->skins_count`, `data->animations_count` — do not read or log these; they are out of scope and accessing them wastes context. Hard-cut per blueprint §5.
- **Multi-mesh GLBs.** A single GLB can contain multiple `cgltf_mesh` nodes. For MVP, load only the first triangle primitive. The engine public API returns one mesh handle per `load_gltf` call; multi-mesh support can be deferred to `spawn_from_asset` returning multiple entities.
- **Never use direct pointer arithmetic on buffer data.** Do not compute `buffer_view->buffer->data + offset` by hand; use the unpack functions to handle stride, sparse accessors, and component-type differences.

---

## 12. Deeper References

Open only the reference relevant to your current task:

| Aspect | File | When to open |
|--------|------|-------------|
| Accessor unpacking — advanced | `references/cgltf-mesh-extraction.md` | Sparse accessors, non-float component types, tangents, Sokol layout mapping |
| Material data | `references/cgltf-material-data.md` | Base color factor, metallic-roughness, alpha modes, texture index extraction (Desirable R-M4) |
