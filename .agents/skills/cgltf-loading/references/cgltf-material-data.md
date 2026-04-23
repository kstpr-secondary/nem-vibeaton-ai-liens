# cgltf — Material Data Reference

Relevant only for Desirable milestone R-M4 (diffuse textures) and beyond. Skip during MVP.

---

## Material Access

```c
// Each primitive has an optional material pointer:
const cgltf_material* mat = prim.material;  // may be NULL → use defaults
```

---

## PBR Metallic-Roughness (the standard glTF 2.0 workflow)

```c
if (mat && mat->has_pbr_metallic_roughness) {
    const cgltf_pbr_metallic_roughness& pbr = mat->pbr_metallic_roughness;

    // Base color factor (linear RGBA)
    float r = pbr.base_color_factor[0];
    float g = pbr.base_color_factor[1];
    float b = pbr.base_color_factor[2];
    float a = pbr.base_color_factor[3];

    // Metalness / roughness scalar factors
    float metallic  = pbr.metallic_factor;   // 0.0 = dielectric, 1.0 = metal
    float roughness = pbr.roughness_factor;  // 0.0 = smooth, 1.0 = rough

    // Base color texture (index into data->textures[])
    const cgltf_texture_view& tv = pbr.base_color_texture;
    if (tv.texture) {
        const cgltf_image* img = tv.texture->image;
        // img->uri  → relative path for external textures (glTF)
        // img->buffer_view → embedded image data (GLB)
        // Pass img to stb_image for decoding (see below).
    }
}
```

---

## Texture Decoding (stb_image bridge)

### Embedded images (GLB)

```cpp
const cgltf_buffer_view* bv = img->buffer_view;
// Use the helper to get the raw pointer (handles buffer + offset internally):
const uint8_t* src = static_cast<const uint8_t*>(cgltf_buffer_view_data(bv));
int w, h, ch;
uint8_t* pixels = stbi_load_from_memory(src, (int)bv->size, &w, &h, &ch, 4);
// → renderer.upload_texture_2d(pixels, w, h, RGBA8)
stbi_image_free(pixels);
```

### External file textures (glTF)

```c
if (img->uri) {
    // Compose full path: ASSET_ROOT + "/" + img->uri
    // Load via stbi_load(full_path, &w, &h, &ch, 4)
}
```

---

## Alpha Mode and Double-Sided

```c
bool double_sided = mat->double_sided;  // set SG_CULLMODE_NONE when true

if (mat->alpha_mode == cgltf_alpha_mode_blend) {
    // Use alpha blending in Sokol pipeline
} else if (mat->alpha_mode == cgltf_alpha_mode_mask) {
    // Use alpha testing — discard fragment in shader when alpha < cutoff
    float cutoff = mat->alpha_cutoff;  // default 0.5 per spec
}
// cgltf_alpha_mode_opaque is the default; no special handling needed.
```

---

## UV Flip

glTF UV origin is top-left; OpenGL expects bottom-left. Flip V when building UVs:
```cpp
uv.v = 1.0f - uv.v;
```
Apply at extraction time (in `extract_vertices`), not in the shader, to avoid inconsistency.
