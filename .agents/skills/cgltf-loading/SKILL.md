---
name: cgltf-loading
description: Use when loading glTF 2.0 assets (.gltf or .glb) using the cgltf library. Covers parsing, buffer loading, validation, and data extraction for meshes, materials, and nodes. Activated when implementing asset importers or bridge functions between glTF and the renderer/engine.
compatibility: Portable across heterogeneous agents (Claude, Copilot, Gemini, GLM, local Qwen).
metadata:
  author: hackathon-team
  version: "1.0"
  project-stage: pre-implementation
  role: library-reference
  activated-by: any cgltf_* function usage
---

# cgltf-loading Skill

Reference for `cgltf.h`. Lightweight, single-file glTF 2.0 loader.

Use per-aspect references under `references/` for deeper detail on mesh data extraction and materials.

---

## 1. Core Rules (must know)

1. **Header-only implementation.** Exactly one `.cpp` file must `#define CGLTF_IMPLEMENTATION` before including `cgltf.h`.
2. **Result checking.** Always check `cgltf_result` against `cgltf_result_success` (0).
3. **Buffer loading.** `cgltf_parse_file` only parses the JSON structure. You MUST call `cgltf_load_buffers` to access actual vertex/index data.
4. **Memory ownership.** `cgltf_free(data)` releases all memory associated with the glTF tree. Call it only after you have uploaded all necessary data to the GPU (Sokol buffers).
5. **Pointer stability.** `cgltf` data structures use direct pointers. Do not move or copy the structs if you rely on internal pointer relationships.

---

## 2. Basic Lifecycle

```c
cgltf_options options = {0};
cgltf_data* data = NULL;

// 1. Parse
cgltf_result res = cgltf_parse_file(&options, path, &data);
if (res != cgltf_result_success) return;

// 2. Load binary data
res = cgltf_load_buffers(&options, data, path);
if (res != cgltf_result_success) { cgltf_free(data); return; }

// 3. Process (meshes, materials, nodes...)
// ...

// 4. Cleanup
cgltf_free(data);
```

---

## 3. Data Extraction Cheatsheet

| Task | Pattern |
| :--- | :--- |
| **Get Mesh Count** | `data->meshes_count` |
| **Get Primitive** | `data->meshes[i].primitives[j]` |
| **Find Attribute** | `cgltf_find_accessor(prim, cgltf_attribute_type_position, 0)` |
| **Read Floats** | `cgltf_accessor_unpack_floats(acc, buffer, count)` |
| **Read Indices** | `cgltf_accessor_unpack_indices(acc, buffer, stride, count)` |
| **Global Transform** | `cgltf_node_transform_world(node, matrix_float16)` |

---

## 4. Common Types & Enums

- **Attribute Types:** `cgltf_attribute_type_position`, `_normal`, `_texcoord`, `_color`.
- **Alpha Modes:** `cgltf_alpha_mode_opaque`, `_mask`, `_blend`.
- **Primitive Types:** `cgltf_primitive_type_triangles` (standard).

---

## 5. Deeper References

For detailed extraction logic, open the relevant reference:

| Aspect | File | When to open |
| :--- | :--- | :--- |
| Mesh Extraction | `references/cgltf-mesh-extraction.md` | Extracting vertex buffers, indices, and mapping to renderer layouts |
| Material Data | `references/cgltf-material-data.md` | PBR parameters, texture URIs, and embedded image buffers |
