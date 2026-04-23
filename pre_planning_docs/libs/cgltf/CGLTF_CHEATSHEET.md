# cgltf API Cheatsheet

`cgltf` is a lightweight, single-file glTF 2.0 loader for C/C++.

## 1. Lifecycle: Load and Free

Basic boilerplate for loading a `.gltf` or `.glb` file.

```c
#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

cgltf_options options = {0}; // Use defaults
cgltf_data* data = NULL;
cgltf_result result = cgltf_parse_file(&options, "model.gltf", &data);

if (result == cgltf_result_success) {
    // Load external binary buffers (.bin) or base64 data
    result = cgltf_load_buffers(&options, data, "model.gltf");
}

if (result == cgltf_result_success) {
    // Optional: Validate the glTF structure
    result = cgltf_validate(data);
}

// ... use data ...

cgltf_free(data);
```

## 2. Accessing Mesh Data

Meshes consist of **Primitives**. Each primitive contains **Attributes** (Position, Normal, etc.) and optionally **Indices**.

### Iterate over all meshes and primitives
```c
for (cgltf_size i = 0; i < data->meshes_count; ++i) {
    cgltf_mesh* mesh = &data->meshes[i];
    for (cgltf_size j = 0; j < mesh->primitives_count; ++j) {
        cgltf_primitive* prim = &mesh->primitives[j];
        
        // Handle Indices
        if (prim->indices) {
            // Read indices...
        }

        // Handle Attributes
        for (cgltf_size k = 0; k < prim->attributes_count; ++k) {
            cgltf_attribute* attr = &prim->attributes[k];
            if (attr->type == cgltf_attribute_type_position) {
                // Read positions...
            }
        }
    }
}
```

### Unpacking Vertex Attributes (Position, Normal, UV)
Use `cgltf_accessor_unpack_floats` for a convenient way to get raw float data regardless of the underlying component type (normalized shorts, half-floats, etc.).

```c
cgltf_accessor* acc = attr->data;
cgltf_size num_components = cgltf_num_components(acc->type); // e.g., 3 for VEC3
cgltf_size float_count = acc->count * num_components;
float* buffer = (float*)malloc(float_count * sizeof(float));

cgltf_accessor_unpack_floats(acc, buffer, float_count);
```

### Unpacking Indices
```c
cgltf_accessor* acc = prim->indices;
cgltf_size index_count = acc->count;
uint32_t* indices = (uint32_t*)malloc(index_count * sizeof(uint32_t));

cgltf_accessor_unpack_indices(acc, indices, sizeof(uint32_t), index_count);
```

## 3. Materials and Textures

`cgltf` supports the standard PBR Metallic-Roughness workflow.

```c
cgltf_material* mat = prim->material;
if (mat && mat->has_pbr_metallic_roughness) {
    cgltf_pbr_metallic_roughness* pbr = &mat->pbr_metallic_roughness;
    
    // Base Color Factor (RGBA)
    float r = pbr->base_color_factor[0];
    
    // Base Color Texture
    if (pbr->base_color_texture.texture) {
        cgltf_image* img = pbr->base_color_texture.texture->image;
        const char* uri = img->uri; // Path to texture file
        
        // If it's a GLB with embedded image:
        if (img->buffer_view) {
            const uint8_t* data_ptr = cgltf_buffer_view_data(img->buffer_view);
            cgltf_size size = img->buffer_view->size;
        }
    }
}
```

## 4. Scene Graph and Transforms

glTF uses a node hierarchy. Each node can have a local transform (TRS or Matrix) and a mesh.

### Global Transforms
`cgltf` provides a helper to compute the world-space matrix for any node.

```c
float world_matrix[16];
cgltf_node_transform_world(node, world_matrix);
```

### Finding Nodes with Meshes
```c
for (cgltf_size i = 0; i < data->nodes_count; ++i) {
    cgltf_node* node = &data->nodes[i];
    if (node->mesh) {
        // This node instances a mesh
    }
}
```

## 5. Helper Functions

| Function | Description |
| :--- | :--- |
| `cgltf_num_components(type)` | Returns number of components (e.g., `cgltf_type_vec3` -> 3). |
| `cgltf_accessor_read_float(...)` | Reads a single element from an accessor as floats. |
| `cgltf_accessor_read_index(...)` | Reads a single index value. |
| `cgltf_buffer_view_data(view)` | Helper to get the pointer to the raw data of a buffer view. |
| `cgltf_find_accessor(prim, type, index)` | Finds an attribute accessor in a primitive by type. |

## 6. Important Types

- **`cgltf_result`**: `cgltf_result_success` (0) is what you want.
- **`cgltf_attribute_type`**: `cgltf_attribute_type_position`, `_normal`, `_texcoord`, `_color`.
- **`cgltf_primitive_type`**: `cgltf_primitive_type_triangles` is most common.
- **`cgltf_alpha_mode`**: `cgltf_alpha_mode_opaque`, `_mask`, `_blend`.
