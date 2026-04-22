# sokol-api / shadow-mapping

**Stretch goal.** Use this when implementing directional-light shadow mapping. Shadows are not in the MVP scope — see Blueprint §13.1.

---

## The Two-Pass Strategy

Shadow mapping requires two distinct render passes per frame:

1. **Shadow pass** — render the scene from the light's perspective into a depth texture.
2. **Main pass** — render the scene from the camera, sampling the shadow depth texture to determine visibility.

---

## Resource Setup

### Shadow map images

```c
// Depth texture that stores the shadow map
sg_image shadow_depth = sg_make_image(&(sg_image_desc){
    .width  = 2048, .height = 2048,
    .pixel_format = SG_PIXELFORMAT_DEPTH,
    .usage  = { .depth_stencil_attachment = true, .immutable = true },
    .label  = "shadow-depth",
});

// Color dummy (required if the pass has a depth-only attachment on some backends)
// On GL 3.3 Core a depth-only attachment works without a color attachment.
```

### Views for attachment and sampling

```c
// View for attaching as depth target in the shadow pass
sg_view shadow_depth_att = sg_make_view(&(sg_view_desc){
    .depth_stencil_attachment = { .image = shadow_depth },
});

// View for sampling in the main pass (bind to shader texture slot)
sg_view shadow_depth_tex = sg_make_view(&(sg_view_desc){
    .texture = { .image = shadow_depth },
});
```

### Shadow comparison sampler

```c
sg_sampler shadow_smp = sg_make_sampler(&(sg_sampler_desc){
    .min_filter  = SG_FILTER_LINEAR,
    .mag_filter  = SG_FILTER_LINEAR,
    .compare     = SG_COMPAREFUNC_LESS_EQUAL,  // PCF-style comparison
    .wrap_u      = SG_WRAP_CLAMP_TO_EDGE,
    .wrap_v      = SG_WRAP_CLAMP_TO_EDGE,
    .label       = "shadow-smp",
});
```

---

## Shadow Pass Pipeline

```c
sg_pipeline shadow_pip = sg_make_pipeline(&(sg_pipeline_desc){
    .shader     = shadow_shader,        // depth-only shader (writes gl_FragDepth or nothing)
    .layout.attrs[SG_ATTR_vs_position].format = SG_VERTEXFORMAT_FLOAT3,
    .index_type = SG_INDEXTYPE_UINT16,
    .cull_mode  = SG_CULLMODE_FRONT,    // front-face culling reduces shadow acne
    .depth = {
        .pixel_format  = SG_PIXELFORMAT_DEPTH,
        .compare       = SG_COMPAREFUNC_LESS_EQUAL,
        .write_enabled = true,
        .bias           = 0.001f,        // polygon offset to reduce self-shadowing
        .bias_slope_scale = 1.0f,
    },
    .color_count = 0,                   // depth-only pass — no color output
    .label = "shadow-pip",
});
```

---

## Frame Loop

### 1. Compute light matrices

```c
// Orthographic projection for a directional light
glm::mat4 light_view = glm::lookAt(light_dir * 50.0f, glm::vec3(0), glm::vec3(0,1,0));
glm::mat4 light_proj = glm::ortho(-20.f, 20.f, -20.f, 20.f, 1.f, 100.f);
glm::mat4 light_vp   = light_proj * light_view;
```

### 2. Shadow pass

```c
sg_begin_pass(&(sg_pass){
    .action = {
        .depth = { .load_action = SG_LOADACTION_CLEAR, .clear_value = 1.0f },
        .colors[0].load_action = SG_LOADACTION_DONTCARE,
    },
    .attachments.depth_stencil = shadow_depth_att,
    .label = "shadow-pass",
});
sg_apply_pipeline(shadow_pip);
// Set viewport to match shadow map dimensions:
sg_apply_viewport(0, 0, 2048, 2048, true);

for each shadow caster:
    shadow_vs_params_t p = { .light_mvp = light_vp * model };
    sg_apply_bindings(&bind);
    sg_apply_uniforms(SG_SLOT_shadow_vs_params, &SG_RANGE(p));
    sg_draw(0, num_indices, 1);

sg_end_pass();
```

### 3. Main pass with shadow lookup

```c
sg_begin_pass(&(sg_pass){
    .action   = main_pass_action,
    .swapchain = sglue_swapchain(),
    .label    = "main-pass",
});
sg_apply_pipeline(scene_pip);

// Bind shadow map for sampling:
state.bind.views[SG_SLOT_shadow_map]    = shadow_depth_tex;
state.bind.samplers[SG_SLOT_shadow_smp] = shadow_smp;

scene_vs_params_t vp = {
    .mvp      = proj * view * model,
    .light_mvp = light_vp * model,
};
sg_apply_uniforms(SG_SLOT_scene_vs_params, &SG_RANGE(vp));
sg_apply_bindings(&state.bind);
sg_draw(0, num_indices, 1);

sg_end_pass();
sg_commit();
```

---

## Fragment Shader Pattern (sokol-shdc)

```glsl
@fs fs
uniform texture2D shadow_map;
uniform sampler shadow_smp;           // comparison sampler

in vec4 light_space_pos;
out vec4 frag_color;

void main() {
    // Project to shadow map UV space
    vec3 proj = light_space_pos.xyz / light_space_pos.w;
    proj = proj * 0.5 + 0.5;         // NDC [-1,1] → UV [0,1]

    // PCF shadow lookup (comparison sampler returns 0.0 or 1.0)
    float shadow = texture(sampler2DShadow(shadow_map, shadow_smp), proj);

    // shadow == 1.0 means lit, 0.0 means in shadow
    frag_color = vec4(base_color * (ambient + shadow * diffuse), 1.0);
}
@end
```

---

## Common Pitfalls

| Problem | Cause | Fix |
|---------|-------|-----|
| Shadow acne (self-shadowing) | Depth precision / no bias | Add `.bias` and `.bias_slope_scale` to pipeline depth state |
| Peter-panning (shadow detaches) | Bias too large | Reduce bias; use front-face culling (`SG_CULLMODE_FRONT`) in shadow pass |
| Hard shadow edge | Point sampler | Use comparison sampler + PCF in shader |
| Shadow outside map shows as shadowed | UV outside [0,1] range | Clamp-to-edge wrap + check `proj.z > 1.0` in shader |
