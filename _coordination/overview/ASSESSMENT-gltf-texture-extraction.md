# Assessment: glTF-Embedded Texture Extraction (Post-MVP, Engine Workstream)

> **Date:** 2026-04-28
> **Trigger:** R-M4 merged (BlinnPhong + diffuse textures in renderer). Game still renders all entities as flat gray Lambertian.
> **Status:** Analysis complete — Approach B selected (engine-side glTF texture extraction).

---

## 1. Current State

### Renderer (R-M4 complete)
- `blinnphong.glsl` shader supports texture sampling gated by `flags.x` uniform
- `renderer_make_blinnphong_material(rgb, shininess, texture_handle)` accepts optional `RendererTextureHandle`
- Draw dispatch (`renderer.cpp:507-577`): when `material.texture.id != 0`, binds the texture from `texture_table[]`; otherwise uses a dummy white 1x1 texture
- Texture upload pipeline: `renderer_upload_texture_from_file(path)` → `stbi_load` → `sg_make_image` → `texture_store_insert()`

### Engine (textures NOT extracted)
- `asset_import_gltf()` extracts only vertex data: positions, normals, UVs, indices
- Zero code touches `data->images[]`, `data->textures[]`, or material properties from glTF
- All spawned entities get hardcoded Lambertian gray material: `{0.7, 0.7, 0.7}`

### Game (solid gray everywhere)
- `spawn.cpp:59`: `renderer_make_lambertian_material({0.7f, 0.7f, 0.7f})` — applied to player, enemy, and all 600 asteroids
- `asteroid.png` exists at `assets/textures/asteroid.png` but is never loaded

---

## 2. glTF File Analysis

All `.glb` files contain **embedded** textures (PNG/JPEG inside bufferViews, not external URIs).

### Model inventory

| Model | Textures | Texture Formats | Materials | Meshes with UVs |
|---|---|---|---|---|
| `Asteroid_1a.glb` | 1 (`Asteroid1a_AO_1K`) | PNG | 1 ("Model.001") | Yes (TEXCOORD_0) |
| `Asteroid_1e.glb` | 1 (`Asteroid1e_AO_1K`) | PNG | 1 ("Asteroid_1e") | Yes (TEXCOORD_0) |
| `spaceship1.glb` | 3 (emi, color, metal/rough) | JPEG/JPEG/PNG | 3 (2 emissive + 1 textured) | Yes on main meshes |
| `spaceship2.glb` | 1 (`Andorian`) | PNG | 2 (1 textured + 1 solid gray) | Yes |
| `spaceship3.glb` | 1 (`1`) | PNG | 3 (1 textured + 2 solid gray) | Yes |

### Material-to-texture chain (cgltf API)

```
data->materials[0].pbrMetallicRoughness.baseColorTexture
    → cgltf_texture_view.texture
        → cgltf_texture.image
            → cgltf_image.buffer_view
                → cgltf_buffer_view.buffer->data  [raw PNG/JPEG bytes]
```

Spaceship1 is special: Material[2] has both `baseColorTexture` (color map) and `emissiveTexture` (glow). The emissive meshes are the engine rings (torus geometry without UVs). The main body uses the color texture.

### Key finding: UV coordinates already present in all textured meshes

`asset_import_gltf()` already unpacks `TEXCOORD_0` attributes (line 100-103 of `asset_import.cpp`). The meshes are ready for texturing — only the material's texture handle is missing.

---

## 3. cgltf API for Texture Extraction

```c
// After cgltf_parse_file() + cgltf_load_buffers():

// Access images (raw pixel data in bufferViews)
cgltf_image* images = data->images;       // array of N images
cgltf_size images_count = data->images_count;

// Access textures (image + sampler links)
cgltf_texture* textures = data->textures;
cgltf_size textures_count = data->textures_count;

// Access materials
cgltf_material* materials = data->materials;
cgltf_size materials_count = data->materials_count;

// Follow the chain from material to image bytes:
const cgltf_texture_view& tex_view = materials[0].pbrMetallicRoughness.baseColorTexture;
if (tex_view.texture && tex_view.texture->image) {
    const cgltf_image* img = tex_view.texture->image;
    // img->buffer_view points to the binary chunk containing PNG/JPEG data
    const cgltf_buffer_view* bv = img->buffer_view;
    const uint8_t* raw_bytes = bv->buffer->data + bv->offset;
    cgltf_size byte_count = bv->size;
    // Use stbi_load_from_memory(raw_bytes, byte_count, &w, &h, &ch, 4)
}
```

---

## 4. Approach B — Implementation Plan

### Scope
Extract baseColor textures from glTF files at load time, create BlinnPhong materials with those textures in the game's spawn functions. No engine public API changes (textures stay internal to the engine).

### Design decisions

| Decision | Choice | Rationale |
|---|---|---|
| Where to extract textures | `asset_import.cpp` (internal) | Keeps texture extraction near the cgltf data; no new public API needed |
| Texture ownership | Renderer (via `renderer_upload_texture_from_memory`) | Reuses existing texture upload pipeline; textures are GPU resources |
| Texture caching | `static std::unordered_map<string, RendererTextureHandle>` in asset_import.cpp | Avoids duplicate upload for same glTF file loaded multiple times |
| Material selection | First material with `baseColorTexture` | Simplest MVP mapping. Spaceship1's main body uses textured material[2] — need to scan for it. |
| Emissive textures | Skip for MVP | Requires separate pipeline or alpha-blended pass; adds complexity |
| Metalness/roughness maps | Skip for MVP | Renderer BlinnPhong shader doesn't support PBR; only baseColor + shininess uniform |
| Solid-color fallback | Use `baseColorFactor` when no texture | Handles materials[1] in spaceship2/3 which have no texture but have a gray factor |
| Multi-material meshes | Extract first textured material | Renderer only supports one Material per draw call. Meshes with mixed materials render with the first texture found. |

### Tasks

#### Task 1: Add `renderer_upload_texture_from_memory()` to renderer API
- **File:** `src/renderer/texture.cpp`, `src/renderer/texture.h` (internal), `src/renderer/renderer.h` (public)
- **Signature:** `RendererTextureHandle renderer_upload_texture_from_memory(const void* pixels, int width, int height, int channels);`
- **Implementation:** Same as `renderer_upload_texture_2d()` — just the first step, no `stbi_load`. Caller already has pixels in memory.
- **Tier:** LOW
- **Validation:** SELF-CHECK

#### Task 2: Add texture extraction to `asset_import_gltf()`
- **File:** `src/engine/asset_import.cpp`
- **New struct:** `ImportedTexture { RendererTextureHandle handle; glm::vec3 base_color; }` — or extend `ImportedMesh` with optional texture field
- **Function:** `std::vector<ImportedTexture> asset_extract_textures(cgltf_data* data)` — called after `cgltf_load_buffers`, before `cgltf_free`
- **Logic:**
  1. Iterate `data->materials[]`
  2. For each material, check `pbrMetallicRoughness.baseColorTexture.texture`
  3. If present: follow chain to `image->buffer_view`
  4. Extract raw bytes from `buffer_view->buffer->data + offset`
  5. Decode via `stbi_load_from_memory()` → get w/h/ch/pixels
  6. Upload via `renderer_upload_texture_from_memory(pixels, w, h, ch)`
  7. Store handle + base color in result vector
  8. Free pixel data with `stbi_image_free()`
- **Note:** Need `#define STB_IMAGE_IMPLEMENTATION` in this TU (or reuse existing); need renderer header include
- **Tier:** MED
- **Validation:** SELF-CHECK

#### Task 3: Expose texture extraction via scene_api / asset_bridge
- **File:** `src/engine/scene_api.h`, `scene_api.cpp`
- **New API:** `std::vector<ImportedTexture> engine_extract_gltf_textures(const char* relative_path);`
- **Or alternatively:** Return textures from a modified `engine_load_gltf()` that also returns texture handles
- **Design choice:** Add `struct ImportedTexture { RendererTextureHandle handle; glm::vec3 base_color; };` to `asset_import.h`
- **Tier:** LOW
- **Validation:** SELF-CHECK

#### Task 4: Update game spawn functions to use textured materials
- **File:** `src/game/spawn.cpp`
- **Changes:**
  - `spawn_asteroid()`: query textures from the asteroid model path, create `renderer_make_blinnphong_material(base_color, shininess=64.0f, texture_handle)` for first textured material found
  - `spawn_player()`: query textures from spaceship1.glb; use textured BlinnPhong if color map found, else fall back to solid metallic color `{0.5, 0.55, 0.6}`, shininess=128
  - `spawn_enemy()`: same as player but with red tint `{0.55, 0.3, 0.3}` if no texture
- **Tier:** MED
- **Validation:** SPEC-VALIDATE

#### Task 5: Build + visual verification
- **File:** build + run `game`
- **Expected outcome:** Asteroids show rocky texture with specular highlights; ships show their color maps with metallic sheen; directional light from game.cpp creates proper BlinnPhong shading

---

## 5. Dependencies

| Dependency | Status | Notes |
|---|---|---|
| R-M4 (BlinnPhong + textures in renderer) | DONE | Prerequisite — shader, pipeline, draw dispatch all complete |
| `renderer_upload_texture_from_memory()` | NOT STARTED | New renderer API addition — requires frozen interface update |
| stb_image available in engine TU | YES | Already a FetchContent dep of renderer; needs to be accessible from engine |
| `#include <renderer.h>` in asset_import.cpp | CHECK | Need to verify engine includes renderer header (via engine.h) |

---

## 6. Risks

1. **Frozen interface:** Adding `renderer_upload_texture_from_memory()` requires updating `docs/interfaces/renderer-interface-spec.md`. This is a post-freeze change — needs human approval per AGENTS.md rule 3.
   - **Mitigation:** The function is trivially derivable from existing `renderer_upload_texture_2d()`. Low risk of drift.

2. **Circular dependency:** `asset_import.cpp` would need to call renderer functions, but the engine currently depends on renderer (via `engine.h → renderer.h`). Need to verify no circular include.
   - **Mitigation:** Engine already includes `renderer.h` through `engine.h`. Adding a renderer call from engine internals should be fine as long as we don't create a header-level cycle.

3. **Spaceship1 multi-material complexity:** The main mesh (RetopoGroup2, prim[10]) has COLOR_0 and COLOR_1 vertex attributes alongside TEXCOORD_0. This mesh is the largest by index count and will be selected by `asset_import_gltf()`. If its material index doesn't match the first textured material in `data->materials[]`, we need to find the correct one.
   - **Mitigation:** Scan all materials for any with `baseColorTexture`, pick the first one found.

4. **JPEG decoding:** Spaceship1 textures are JPEG (`.jpg`). `stbi_load_from_memory` handles both PNG and JPEG transparently. No special handling needed.

5. **Memory overhead:** Each glTF texture is decoded once and stored in the renderer's texture table (up to 255 textures). With ~7 models × 3 textures max = 21 textures — well within limits.

---

## 7. Open Questions

1. **Should we also extract `baseColorFactor` as a fallback?** Some materials (spaceship1 material[0], material[1]) have no texture but have solid color factors. For MVP, skipping these is acceptable — the first textured material will be used.
2. **Emissive materials (spaceship1):** The engine rings glow. Should emissive textures get a separate pass? → Defer to Desirable (custom shader hook at R-M5).
3. **Normal maps:** Not present in current glTF files. No action needed.

---

## 8. Recommended Task Ordering

```
1. Add renderer_upload_texture_from_memory() [renderer side, LOW]
   ↓
2. Extend asset_import_gltf() to extract textures [engine side, MED]
   ↓
3. Update game spawn functions to use BlinnPhong + extracted textures [game side, MED]
   ↓
4. Build + visual verification [all sides]
```

Steps 1 and 2 can be done in parallel if the renderer API addition is pre-approved (low risk). Step 3 depends on both 1 and 2.

---

## 9. Files to Touch

| File | Workstream | Change |
|---|---|---|
| `src/renderer/texture.cpp` | Renderer | Add `renderer_upload_texture_from_memory()` |
| `src/renderer/texture.h` | Renderer | Internal header — add declaration |
| `src/renderer/renderer.h` | Renderer | **Frozen interface update** — add new public function |
| `docs/interfaces/renderer-interface-spec.md` | Renderer | **Frozen interface update** — add function to spec |
| `src/engine/asset_import.h` | Engine | Add `ImportedTexture` struct |
| `src/engine/asset_import.cpp` | Engine | Extend `asset_import_gltf()` or add `asset_extract_textures()` |
| `src/game/spawn.cpp` | Game | Replace Lambertian gray with BlinnPhong + texture handle |

---

*Assessment complete. Awaiting human approval on frozen interface update before implementation begins.*
