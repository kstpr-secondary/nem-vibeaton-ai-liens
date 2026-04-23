# cgltf Material Data Reference

Use this reference when extracting PBR parameters and texture information.

## 1. PBR Metallic Roughness

Standard glTF material workflow.

```c
cgltf_material* mat = prim->material;
if (mat && mat->has_pbr_metallic_roughness) {
    cgltf_pbr_metallic_roughness* pbr = &mat->pbr_metallic_roughness;
    
    // Base Color (RGBA)
    float* base_color = pbr->base_color_factor;
    
    // Metalness/Roughness
    float metal = pbr->metallic_factor;
    float rough = pbr->roughness_factor;
}
```

## 2. Textures and Images

### File-based Textures
```c
if (pbr->base_color_texture.texture) {
    cgltf_image* img = pbr->base_color_texture.texture->image;
    if (img->uri) {
        // Load file via STB: ASSET_ROOT + "/" + img->uri
    }
}
```

### Embedded (GLB) Images
```c
if (img->buffer_view) {
    // Data is embedded in the glTF buffer
    const uint8_t* ptr = (const uint8_t*)img->buffer_view->buffer->data + img->buffer_view->offset;
    cgltf_size size = img->buffer_view->size;
    // Pass ptr and size to stbi_load_from_memory
}
```

## 3. Alpha and Double Sided

```c
bool double_sided = mat->double_sided;
if (mat->alpha_mode == cgltf_alpha_mode_blend) {
    // Use alpha blending in Sokol pipeline
} else if (mat->alpha_mode == cgltf_alpha_mode_mask) {
    // Use alpha testing (discard in shader)
    float cutoff = mat->alpha_cutoff;
}
```
