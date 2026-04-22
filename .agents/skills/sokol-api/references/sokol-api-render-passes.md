# sokol-api / render-passes

Open this when you need anything beyond the standard swapchain pass — e.g. offscreen render targets, multi-pass pipelines (shadow depth pass + scene pass), MRT (multiple render targets), explicit load/store actions (`SG_LOADACTION_LOAD`, `SG_LOADACTION_DONTCARE`), or RenderDoc debug groups. For the basic swapchain clear + draw loop, SKILL.md §7 is sufficient.

---

## Pass Structs

```c
typedef struct sg_pass {
    bool compute;               // false for render passes
    sg_pass_action action;      // load/store/clear actions per attachment
    sg_attachments attachments; // offscreen targets (leave empty for swapchain)
    sg_swapchain swapchain;     // from sglue_swapchain() for the default pass
    const char* label;
} sg_pass;
```

---

## Default (Swapchain) Pass

```c
sg_pass_action clear_action = {
    .colors[0] = {
        .load_action = SG_LOADACTION_CLEAR,
        .clear_value = { 0.1f, 0.1f, 0.1f, 1.0f },
    },
    // depth clears to 1.0 by default
};

sg_begin_pass(&(sg_pass){
    .action   = clear_action,
    .swapchain = sglue_swapchain(),
    .label    = "main-pass",
});
// ... draw calls ...
sg_end_pass();
sg_commit();
```

---

## Pass Load/Store Actions

| `sg_load_action` | Meaning | Use when |
|------------------|---------|---------|
| `SG_LOADACTION_CLEAR` | Clear to `clear_value` (default) | Normal frame start |
| `SG_LOADACTION_LOAD` | Preserve previous frame content | Accumulation, compositing |
| `SG_LOADACTION_DONTCARE` | Undefined (fastest) | You'll overdraw every pixel |

| `sg_store_action` | Meaning | Default |
|-------------------|---------|---------|
| `SG_STOREACTION_STORE` | Write result | Color attachments |
| `SG_STOREACTION_DONTCARE` | Discard | Depth/stencil (default) |

---

## Offscreen Render Pass

### Step 1: Create render-target images

```c
sg_image color_rt = sg_make_image(&(sg_image_desc){
    .width  = 512, .height = 512,
    .pixel_format = SG_PIXELFORMAT_RGBA8,
    .usage  = { .color_attachment = true, .immutable = true },
    .label  = "offscreen-color",
});

sg_image depth_rt = sg_make_image(&(sg_image_desc){
    .width  = 512, .height = 512,
    .pixel_format = SG_PIXELFORMAT_DEPTH,
    .usage  = { .depth_stencil_attachment = true, .immutable = true },
    .label  = "offscreen-depth",
});
```

### Step 2: Create views for attachments

```c
sg_view color_view = sg_make_view(&(sg_view_desc){
    .color_attachment = { .image = color_rt },
});
sg_view depth_view = sg_make_view(&(sg_view_desc){
    .depth_stencil_attachment = { .image = depth_rt },
});
```

### Step 3: Render to offscreen target

```c
sg_begin_pass(&(sg_pass){
    .action = {
        .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = {0,0,0,1} },
        .depth     = { .load_action = SG_LOADACTION_CLEAR, .clear_value = 1.0f },
    },
    .attachments = {
        .colors[0]    = color_view,
        .depth_stencil = depth_view,
    },
    .label = "offscreen-pass",
});
sg_apply_pipeline(offscreen_pip);
// ... draw to offscreen target ...
sg_end_pass();
```

### Step 4: Use result as texture in main pass

```c
sg_begin_pass(&(sg_pass){
    .action   = main_pass_action,
    .swapchain = sglue_swapchain(),
    .label    = "main-pass",
});
sg_apply_pipeline(fullscreen_pip);
// Bind the offscreen color as a texture:
state.bind.views[SG_SLOT_offscreen_tex]  = color_view;
state.bind.samplers[SG_SLOT_offscreen_smp] = smp;
sg_apply_bindings(&state.bind);
sg_draw(0, 6, 1);  // fullscreen quad
sg_end_pass();
sg_commit();
```

---

## Multi-Pass: Shadow Map Example

```c
// Pass 1: Shadow depth pass (depth only, no color attachment)
sg_begin_pass(&(sg_pass){
    .action = {
        .depth = { .load_action = SG_LOADACTION_CLEAR, .clear_value = 1.0f },
        .colors[0].load_action = SG_LOADACTION_DONTCARE,
    },
    .attachments.depth_stencil = shadow_depth_view,
    .label = "shadow-pass",
});
sg_apply_pipeline(shadow_pip);
// ... draw shadow casters only ...
sg_end_pass();

// Pass 2: Main scene with shadow lookup
sg_begin_pass(&(sg_pass){
    .action   = main_pass_action,
    .swapchain = sglue_swapchain(),
    .label    = "scene-pass",
});
sg_apply_pipeline(scene_pip);
state.bind.views[SG_SLOT_shadow_map]    = shadow_depth_view;
state.bind.samplers[SG_SLOT_shadow_smp] = shadow_smp;
sg_apply_bindings(&state.bind);
// ... draw full scene ...
sg_end_pass();
sg_commit();
```

Shadow sampler (comparison):
```c
sg_sampler shadow_smp = sg_make_sampler(&(sg_sampler_desc){
    .min_filter  = SG_FILTER_LINEAR,
    .mag_filter  = SG_FILTER_LINEAR,
    .compare     = SG_COMPAREFUNC_LESS_EQUAL,
    .wrap_u      = SG_WRAP_CLAMP_TO_EDGE,
    .wrap_v      = SG_WRAP_CLAMP_TO_EDGE,
    .label       = "shadow-smp",
});
```

---

## MRT (Multiple Render Targets)

```c
sg_begin_pass(&(sg_pass){
    .action = {
        .colors[0] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = {0,0,0,1} },
        .colors[1] = { .load_action = SG_LOADACTION_CLEAR, .clear_value = {0,0,0,1} },
    },
    .attachments = {
        .colors[0] = albedo_view,
        .colors[1] = normal_view,
        .depth_stencil = depth_view,
    },
});
// Pipeline must set .color_count = 2 and pixel formats to match
```

---

## Resource State Check (before use)

```c
if (sg_query_image_state(color_rt) != SG_RESOURCESTATE_VALID) {
    // render target creation failed — log and use fallback
}
if (sg_query_pipeline_state(offscreen_pip) != SG_RESOURCESTATE_VALID) {
    // pipeline creation failed — use magenta error pipeline
}
```

---

## Debug Groups (RenderDoc)

```c
sg_push_debug_group("shadow-pass");
sg_begin_pass(&shadow_pass);
// ...
sg_end_pass();
sg_pop_debug_group();
```
