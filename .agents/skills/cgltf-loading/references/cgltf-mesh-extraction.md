# cgltf — Mesh Extraction (Advanced Reference)

Extend this file when the SKILL.md §6–7 accessor patterns are insufficient.

---

## Finding Attributes

Two equivalent patterns for locating a specific attribute in a primitive:

```cpp
// Manual loop (more explicit):
const cgltf_accessor* pos_acc = nullptr;
for (cgltf_size i = 0; i < prim->attributes_count; ++i) {
    if (prim->attributes[i].type == cgltf_attribute_type_position) {
        pos_acc = prim->attributes[i].data;
        break;
    }
}

// Helper function (cleaner):
const cgltf_accessor* pos_acc = cgltf_find_accessor(prim, cgltf_attribute_type_position, 0);
// Third arg is the index within the same type (0 for TEXCOORD_0, 1 for TEXCOORD_1, etc.)
```

Always prefer `unpack` functions over direct pointer arithmetic to handle normalized components or different bit depths automatically.

---

## Unpacking Strategy

### Vertex Positions (VEC3)

```cpp
float* positions = (float*)malloc(pos_acc->count * 3 * sizeof(float));
cgltf_accessor_unpack_floats(pos_acc, positions, pos_acc->count * 3);
```

### Vertex Indices (bulk — preferred)

```cpp
uint32_t* indices = (uint32_t*)malloc(prim->indices->count * sizeof(uint32_t));
cgltf_accessor_unpack_indices(prim->indices, indices, sizeof(uint32_t), prim->indices->count);
// stride parameter (sizeof(uint32_t)) controls output element size; handles UBYTE/USHORT/UINT input.
```

---

## Mapping to Sokol Layout

```c
sg_pipeline_desc pip_desc = {
    .layout.attrs = {
        [ATTR_vs_position] = { .format = SG_VERTEXFORMAT_FLOAT3 },
        [ATTR_vs_normal]   = { .format = SG_VERTEXFORMAT_FLOAT3 },
        [ATTR_vs_texcoord] = { .format = SG_VERTEXFORMAT_FLOAT2 }
    }
    // ...
};
```

---

## Primitive Topology

Always check the primitive type before processing — only triangles are in scope for this project:

```c
if (prim->type != cgltf_primitive_type_triangles) {
    // log skip: points, lines, triangle_strip are not used
    continue;
}
```

---

## Sparse Accessors

cgltf transparently resolves sparse accessors in `cgltf_accessor_unpack_floats` and `cgltf_accessor_read_index`/`cgltf_accessor_unpack_indices`. No special handling is needed in the engine — use the same unpack calls.

---

## Non-Float Component Types for Positions / Normals

glTF 2.0 requires `FLOAT` for positions and normals. If a file uses a packed type (`BYTE`, `SHORT`), it is non-conformant. `cgltf_accessor_unpack_floats` will return 0 and write nothing. Log and skip the primitive.

---

## Tangents

`cgltf_attribute_type_tangent` → `vec4` (xyz = tangent direction, w = handedness ±1). Used only if normal maps are enabled (Desirable R-M7). Extraction mirrors the normal path with `n * 4` floats.

---

## Color Attributes

`cgltf_attribute_type_color` → `vec3` or `vec4`. Not used in this project's MVP shaders. Skip.

---

## cgltf_num_components

```c
// Returns the number of floats per element for a given accessor type:
// VEC2 → 2, VEC3 → 3, VEC4 → 4, MAT4 → 16, SCALAR → 1
cgltf_size cgltf_num_components(cgltf_type type);
```

Use this to compute the correct `float_count` for `cgltf_accessor_unpack_floats`:
```cpp
size_t float_count = accessor->count * cgltf_num_components(accessor->type);
```

---

## Unindexed Meshes

When `prim.indices == nullptr`, the mesh is unindexed (sequential index 0, 1, 2, …). Generate a sequential index buffer:
```cpp
std::vector<uint32_t> idx(n);
std::iota(idx.begin(), idx.end(), 0u);
```
