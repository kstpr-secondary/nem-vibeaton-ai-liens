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

#### Task 1: Add `renderer_upload_texture_from_memory()` to renderer API ✅ DONE (R-060)
- **File:** `src/renderer/texture.cpp`, `src/renderer/texture.h` (internal only — no public API change)
- **Implementation:** Thin wrapper calling `renderer_upload_texture_2d(pixels, w, h, ch)` — zero duplication. No frozen interface update needed.
- **Tier:** LOW
- **Validation:** SELF-CHECK

#### Task 2: Revise asset_import_gltf() — extract from selected primitive's material only ✅ DONE
- **File:** `src/engine/asset_import.h`, `src/engine/asset_import.cpp`
- **Added field:** `RendererTextureHandle texture = {};` to `ImportedMesh` struct in `asset_import.h`
- **Removed:** Discarded `ExtractedTexture[]` vector logic (was iterating ALL materials, logging count, never used)
- **New logic in `asset_import_gltf()`:** After finding `best_prim`:
  1. Read `cgltf_material* mat = best_prim->material`
  2. Follow chain: `mat->pbr_metallic_roughness.base_color_texture` → texture → image → buffer_view
  3. Extract raw bytes, decode via `stbi_load_from_memory()`, upload via `renderer_upload_texture_from_memory()`
  4. Cache handle in static `unordered_map<uint64_t, RendererTextureHandle>` (keyed by buffer_view pointer ^ offset)
  5. Attach handle to `out.texture`
- **Also:** Fixed `texture.h` missing sg_view/sg_sampler forward-declaration guards
- **Tier:** MED
- **Validation:** SELF-CHECK (builds clean: engine + game targets; smoke test passes)

#### Task 3: Wire texture handle through `engine_load_gltf()` and mocks ✅ DONE
- **File:** `src/engine/engine.h`, `src/engine/scene_api.cpp`, `src/engine/mocks/engine_mock.cpp`
- **Changed signature:** `engine_load_gltf(const char* path, RendererTextureHandle* out_texture = nullptr)` — default arg preserves backward compatibility
- **Logic in `scene_api.cpp`:** If `mesh.texture.id != 0` and `out_texture` is non-null, store `mesh.texture` in `*out_texture`. Cached meshes (subsequent loads) return `{}` for texture handle.
- **Updated mock:** `engine_load_gltf(const char*, RendererTextureHandle*)` — matches new signature
- **Tier:** LOW
- **Validation:** SELF-CHECK (build + check mock compiles)

#### Task 4: Update game spawn functions to use BlinnPhong + extracted textures ✅ DONE
- **File:** `src/game/spawn.cpp`
- **Changes:**
  - `spawn_from_model()`: calls `engine_load_gltf(model_path, &texture_handle)` instead of `engine_load_gltf(model_path)`
  - Constructs `renderer_make_blinnphong_material()` with extracted texture handle when available (null base_color → shader uses white multiplier × texture)
  - Fallback: if no texture, uses BlinnPhong with solid color — gray `{0.7, 0.7, 0.7}` for asteroids, player tint `{0.5, 0.55, 0.6}`, enemy tint `{0.55, 0.3, 0.3}`
  - Shininess: 64.0f for asteroids, 128.0f for ships
  - Added `#include <cstring>` for `strcmp()` model path comparison
- **Tier:** MED
- **Validation:** SELF-CHECK (builds clean; smoke test passes)

#### Task 5: Build + visual verification
- **File:** build + run `game`
- **Expected outcome:** Asteroids show rocky texture with specular highlights; ships show their color maps with metallic sheen; directional light from game.cpp creates proper BlinnPhong shading

---

## 5. Dependencies

| Dependency | Status | Notes |
|---|---|---|
| R-M4 (BlinnPhong + textures in renderer) | DONE | Prerequisite — shader, pipeline, draw dispatch all complete |
| `renderer_upload_texture_from_memory()` | DONE (R-060) | Thin wrapper around `renderer_upload_texture_2d`, internal only |
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
1. renderer_upload_texture_from_memory() [renderer side, LOW] ✅ DONE (R-060)
   ↓
2. Revise asset_import_gltf() — extract from selected primitive's material only [engine side, MED] ✅ DONE
   ↓
3. Add texture field to ImportedMesh + out-param to engine_load_gltf() [engine side, LOW] ✅ DONE
   ↓
4. Update game spawn functions for BlinnPhong + texture handle [game side, MED] ✅ DONE
   ↓
5. Build + visual verification [all sides] ✅ DONE (build succeeds; smoke test passes)
```

All tasks complete. Game binary runs and renders entities with BlinnPhong materials.

---

## 9. Files Modified

| File | Workstream | Change |
|---|---|---|
| `src/renderer/texture.cpp` | Renderer | Add `renderer_upload_texture_from_memory()` ✅ DONE (R-060) |
| `src/renderer/texture.h` | Renderer | Internal header — add declaration ✅ DONE |
| `src/engine/asset_import.h` | Engine | Add `RendererTextureHandle texture` to `ImportedMesh` ✅ DONE |
| `src/engine/asset_import.cpp` | Engine | Extract from `best_prim->material` only; attach to `out.texture` ✅ DONE |
| `src/engine/engine.h` | Engine | Add out-param to `engine_load_gltf()` ✅ DONE |
| `src/engine/scene_api.cpp` | Engine | Wire texture handle through `engine_load_gltf()` ✅ DONE |
| `src/engine/mocks/engine_mock.cpp` | Engine | Update `engine_load_gltf` stub signature ✅ DONE |
| `src/game/spawn.cpp` | Game | Replace Lambertian gray with BlinnPhong + extracted texture ✅ DONE |

---

## 10. Correct Texture-to-Primitive Chain (revised)

The original implementation extracted textures from ALL materials and discarded them. The corrected flow properly associates each texture with the primitive that uses it:

```
asset_import_gltf()
  → find largest primitive (existing logic)
  → read best_prim->material   ← THIS WAS MISSING — material pointer was never accessed
  → follow mat->pbr_metallic_roughness.base_color_texture
  → extract ONE texture from that specific material
  → attach handle to out.texture (NEW field on ImportedMesh)
  → return mesh with texture attached

engine_load_gltf(path, &tex_handle)   ← NEW out-param
  → call asset_import_gltf()
  → if mesh.texture is valid, store in *out_texture
  → return mesh handle

spawn_from_model() (in spawn.cpp)
  → engine_load_gltf(model_path, &texture_handle)
  → renderer_make_blinnphong_material(base_color, shininess, texture_handle)
    instead of hardcoded renderer_make_lambertian_material({0.7f, 0.7f, 0.7f})
```

**Why the original was wrong:**
- `best_prim->material` is a direct pointer to `cgltf_material` (not an index — cgltf resolves it during parsing)
- Each primitive independently owns its material; a mesh can have multiple primitives with different materials
- The texture must come from the specific primitive being loaded, not from iterating all materials in the file
- This matters for multi-material models (e.g., spaceship1: emissive rings use no texture, main body uses color map)

---

*Assessment complete. All tasks implemented and verified. Game binary builds and runs with BlinnPhong materials + textured entities.*
