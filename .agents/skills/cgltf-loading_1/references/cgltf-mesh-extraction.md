# cgltf Mesh Extraction Reference

Use this reference when mapping glTF mesh primitives to GPU buffers (Sokol).

## 1. Finding Attributes

Use `cgltf_find_accessor` to locate specific attribute data in a primitive.

```c
const cgltf_accessor* pos_acc = NULL;
for (cgltf_size i = 0; i < prim->attributes_count; ++i) {
    if (prim->attributes[i].type == cgltf_attribute_type_position) {
        pos_acc = prim->attributes[i].data;
        break;
    }
}
// OR helper:
// cgltf_accessor* pos_acc = cgltf_find_accessor(prim, cgltf_attribute_type_position, 0);
```

## 2. Unpacking Strategy

Always prefer `unpack` functions over direct pointer arithmetic to handle normalized components or different bit depths automatically.

### Vertex Positions (VEC3)
```c
float* positions = (float*)malloc(pos_acc->count * 3 * sizeof(float));
cgltf_accessor_unpack_floats(pos_acc, positions, pos_acc->count * 3);
```

### Vertex Indices
```c
uint32_t* indices = (uint32_t*)malloc(prim->indices->count * sizeof(uint32_t));
cgltf_accessor_unpack_indices(prim->indices, indices, sizeof(uint32_t), prim->indices->count);
```

## 3. Mapping to Sokol Layout

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

## 4. Primitive Topology

Check the primitive type to ensure it matches your expected draw call (usually triangles).

```c
if (prim->type != cgltf_primitive_type_triangles) {
    // Handle or warn: points, lines, triangle_strip, etc.
}
```
