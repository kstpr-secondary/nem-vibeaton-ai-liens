# sokol-api / work-with-descriptors

Full descriptor structs for all sokol_gfx resource types. Open this only when the SKILL.md cheatsheet is insufficient — e.g. when you need non-default fields for complex resources.

---

## sg_desc (sg_setup)

```c
typedef struct sg_desc {
    int buffer_pool_size;       // default: 128
    int image_pool_size;        // default: 128
    int sampler_pool_size;      // default: 64
    int shader_pool_size;       // default: 32
    int pipeline_pool_size;     // default: 64
    int view_pool_size;         // default: 256
    int uniform_buffer_size;    // default: 4 MB
    int max_commit_listeners;   // default: 1024
    bool disable_validation;    // skip validation in release
    sg_allocator allocator;     // custom malloc/free
    sg_logger logger;           // .func = slog_func
    sg_environment environment; // from sglue_environment()
} sg_desc;
```

---

## sg_buffer_desc

```c
typedef struct sg_buffer_usage {
    bool vertex_buffer;         // bind as vertex buffer
    bool index_buffer;          // bind as index buffer
    bool storage_buffer;        // bind as SSBO
    bool immutable;             // data set at creation (default)
    bool dynamic_update;        // sg_update_buffer once/frame
    bool stream_update;         // sg_append_buffer many/frame
} sg_buffer_usage;

typedef struct sg_buffer_desc {
    size_t size;                // buffer size in bytes
    sg_buffer_usage usage;      // usage flags
    sg_range data;              // initial data (required for immutable)
    const char* label;
} sg_buffer_desc;
```

**SG_RANGE macro:**
```c
// C:   SG_RANGE(x) → (sg_range){ &x, sizeof(x) }
// C++: SG_RANGE(x) → sg_range{ &x, sizeof(x) }
// Arrays: SG_RANGE(arr) uses sizeof(arr) — only works for stack arrays, not pointers
```

---

## sg_image_desc

```c
typedef struct sg_image_usage {
    bool storage_image;
    bool color_attachment;
    bool resolve_attachment;
    bool depth_stencil_attachment;
    bool immutable;             // default: true
    bool dynamic_update;
    bool stream_update;
} sg_image_usage;

typedef struct sg_image_data {
    sg_range mip_levels[SG_MAX_MIPMAPS];  // data per mip level (index 0 = base)
} sg_image_data;

typedef struct sg_image_desc {
    sg_image_type   type;       // 2D (default), CUBE, 3D, ARRAY
    sg_image_usage  usage;
    int width;                  // pixels (required)
    int height;                 // pixels (required)
    int num_slices;             // array layers / 3D depth (default: 1)
    int num_mipmaps;            // mip levels (default: 1)
    sg_pixel_format pixel_format; // default: RGBA8
    int sample_count;           // MSAA samples (default: 1)
    sg_image_data data;         // initial pixel data (required for immutable)
    const char* label;
} sg_image_desc;
```

**Render target image:**
```c
sg_image color_rt = sg_make_image(&(sg_image_desc){
    .width = 512, .height = 512,
    .pixel_format = SG_PIXELFORMAT_RGBA8,
    .usage = { .color_attachment = true, .immutable = true },
    .label = "offscreen-color",
});
sg_image depth_rt = sg_make_image(&(sg_image_desc){
    .width = 512, .height = 512,
    .pixel_format = SG_PIXELFORMAT_DEPTH,
    .usage = { .depth_stencil_attachment = true, .immutable = true },
    .label = "offscreen-depth",
});
```

---

## sg_sampler_desc

```c
typedef struct sg_sampler_desc {
    sg_filter min_filter;       // default: LINEAR
    sg_filter mag_filter;       // default: LINEAR
    sg_filter mipmap_filter;    // default: LINEAR
    sg_wrap wrap_u;             // default: REPEAT
    sg_wrap wrap_v;             // default: REPEAT
    sg_wrap wrap_w;             // default: REPEAT (3D textures)
    float min_lod;              // default: 0.0
    float max_lod;              // default: FLT_MAX
    sg_border_color border_color;  // for CLAMP_TO_BORDER
    sg_compare_func compare;    // for shadow map comparison samplers
    uint32_t max_anisotropy;    // 1..16 (1 = disabled)
    const char* label;
} sg_sampler_desc;
```

---

## sg_pipeline_desc

```c
typedef struct sg_vertex_buffer_layout_state {
    int stride;                 // bytes per vertex (0 = auto)
    sg_vertex_step step_func;   // PER_VERTEX (default) or PER_INSTANCE
    int step_rate;              // advance every N instances (default: 1)
} sg_vertex_buffer_layout_state;

typedef struct sg_vertex_attr_state {
    int buffer_index;           // which vertex buffer slot (default: 0)
    int offset;                 // byte offset in vertex struct (0 = auto)
    sg_vertex_format format;    // FLOAT3, FLOAT2, UBYTE4N, etc.
} sg_vertex_attr_state;

typedef struct sg_vertex_layout_state {
    sg_vertex_buffer_layout_state buffers[SG_MAX_VERTEXBUFFER_BINDSLOTS];
    sg_vertex_attr_state attrs[SG_MAX_VERTEX_ATTRIBUTES];
} sg_vertex_layout_state;

typedef struct sg_depth_state {
    sg_pixel_format pixel_format;  // must match render target depth format
    sg_compare_func compare;    // LESS_EQUAL typical
    bool write_enabled;         // write to depth buffer
    float bias;                 // polygon offset (shadow maps)
    float bias_slope_scale;
    float bias_clamp;
} sg_depth_state;

typedef struct sg_blend_state {
    bool enabled;
    sg_blend_factor src_factor_rgb;
    sg_blend_factor dst_factor_rgb;
    sg_blend_op op_rgb;
    sg_blend_factor src_factor_alpha;
    sg_blend_factor dst_factor_alpha;
    sg_blend_op op_alpha;
} sg_blend_state;

typedef struct sg_color_target_state {
    sg_pixel_format pixel_format;  // must match render target color format
    sg_color_mask write_mask;   // RGBA by default
    sg_blend_state blend;
} sg_color_target_state;

typedef struct sg_pipeline_desc {
    bool compute;               // false for render pipelines
    sg_shader shader;           // required
    sg_vertex_layout_state layout;
    sg_depth_state depth;
    sg_stencil_state stencil;
    int color_count;            // number of color targets (default: 1)
    sg_color_target_state colors[SG_MAX_COLOR_ATTACHMENTS];
    sg_primitive_type primitive_type;  // TRIANGLES default
    sg_index_type index_type;          // NONE, UINT16, UINT32
    sg_cull_mode cull_mode;            // NONE (default), FRONT, BACK
    sg_face_winding face_winding;      // CCW default
    int sample_count;                  // MSAA (match render target)
    sg_color blend_color;              // constant blend color
    bool alpha_to_coverage_enabled;
    const char* label;
} sg_pipeline_desc;
```

**Alpha-blend pipeline snippet:**
```c
.colors[0] = {
    .blend = {
        .enabled = true,
        .src_factor_rgb   = SG_BLENDFACTOR_SRC_ALPHA,
        .dst_factor_rgb   = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
        .op_rgb           = SG_BLENDOP_ADD,
        .src_factor_alpha = SG_BLENDFACTOR_ONE,
        .dst_factor_alpha = SG_BLENDFACTOR_ZERO,
        .op_alpha         = SG_BLENDOP_ADD,
    },
},
.depth = {
    .compare = SG_COMPAREFUNC_LESS_EQUAL,
    .write_enabled = false,   // don't write depth for transparent
},
```

---

## sg_pass_action

```c
typedef struct sg_color_attachment_action {
    sg_load_action load_action;   // default: CLEAR
    sg_store_action store_action; // default: STORE
    sg_color clear_value;         // default: { 0.5, 0.5, 0.5, 1.0 }
} sg_color_attachment_action;

typedef struct sg_depth_attachment_action {
    sg_load_action load_action;   // default: CLEAR
    sg_store_action store_action; // default: DONTCARE
    float clear_value;            // default: 1.0
} sg_depth_attachment_action;

typedef struct sg_stencil_attachment_action {
    sg_load_action load_action;   // default: CLEAR
    sg_store_action store_action; // default: DONTCARE
    uint8_t clear_value;          // default: 0
} sg_stencil_attachment_action;

typedef struct sg_pass_action {
    sg_color_attachment_action colors[SG_MAX_COLOR_ATTACHMENTS];
    sg_depth_attachment_action depth;
    sg_stencil_attachment_action stencil;
} sg_pass_action;
```

---

## sg_bindings

```c
typedef struct sg_bindings {
    sg_buffer vertex_buffers[SG_MAX_VERTEXBUFFER_BINDSLOTS];   // up to 8
    int vertex_buffer_offsets[SG_MAX_VERTEXBUFFER_BINDSLOTS];  // byte offsets
    sg_buffer index_buffer;
    int index_buffer_offset;
    sg_view views[SG_MAX_VIEW_BINDSLOTS];       // textures / SSBOs (up to 32)
    sg_sampler samplers[SG_MAX_SAMPLER_BINDSLOTS]; // up to 12
} sg_bindings;
```

---

## Compile-Time Constants

| Constant | Value |
|----------|-------|
| `SG_MAX_COLOR_ATTACHMENTS` | 8 |
| `SG_MAX_VERTEX_ATTRIBUTES` | 16 |
| `SG_MAX_VERTEXBUFFER_BINDSLOTS` | 8 |
| `SG_MAX_UNIFORMBLOCK_BINDSLOTS` | 8 |
| `SG_MAX_VIEW_BINDSLOTS` | 32 |
| `SG_MAX_SAMPLER_BINDSLOTS` | 12 |
| `SG_MAX_MIPMAPS` | 16 |
| `SG_NUM_INFLIGHT_FRAMES` | 2 |
