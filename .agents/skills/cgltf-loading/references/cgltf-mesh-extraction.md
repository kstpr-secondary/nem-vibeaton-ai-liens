# cgltf — Mesh Extraction (Advanced Reference)

Extend this file when the SKILL.md §5–6 accessor patterns are insufficient.

---

## Sparse Accessors

cgltf transparently resolves sparse accessors in `cgltf_accessor_unpack_floats` and `cgltf_accessor_read_index`. No special handling is needed in the engine — use the same unpack calls.

## Non-Float Component Types for Positions / Normals

glTF 2.0 requires `FLOAT` for positions and normals. If a file uses a packed type (`BYTE`, `SHORT`), it is non-conformant. `cgltf_accessor_unpack_floats` will return 0 and write nothing. Log and skip the primitive.

## Tangents

`cgltf_attribute_type_tangent` → `vec4` (xyz = tangent direction, w = handedness ±1). Used only if normal maps are enabled (Desirable R-M7). Extraction mirrors the normal path with `n * 4` floats.

## Color Attributes

`cgltf_attribute_type_color` → `vec3` or `vec4`. Not used in this project's MVP shaders. Skip.

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

## Unindexed Meshes

When `prim.indices == nullptr`, the mesh is unindexed (sequential index 0, 1, 2, …). Generate a sequential index buffer:
```cpp
std::vector<uint32_t> idx(n);
std::iota(idx.begin(), idx.end(), 0u);
```
