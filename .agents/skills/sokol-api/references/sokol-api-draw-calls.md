# sokol-api / draw-calls

Open this when you need per-pass state beyond a simple single-pipeline draw — e.g. viewport or scissor overrides, instanced rendering (`sg_draw` with `num_instances > 1`), multi-object loops with per-object uniforms, transparent vs opaque pipeline switching within a pass, or the `sg_draw_ex` extended form. For the basic `apply_pipeline → apply_bindings → apply_uniforms → draw` sequence, SKILL.md §7 is sufficient.

---

## Draw Call Sequence (per pass)

```
sg_begin_pass(...)
  [sg_apply_viewport(...)]          // optional
  [sg_apply_scissor_rect(...)]      // optional
  sg_apply_pipeline(pip)
  sg_apply_bindings(&bind)
  sg_apply_uniforms(slot, &SG_RANGE(params))
  sg_draw(base, count, instances)
  // repeat apply_pipeline / apply_bindings / apply_uniforms / draw for more objects
sg_end_pass()
sg_commit()                         // once per frame, after ALL passes
```

---

## Core Draw Functions

### sg_draw

```c
void sg_draw(int base_element, int num_elements, int num_instances);
```

| Parameter | Meaning |
|-----------|---------|
| `base_element` | First vertex index (non-indexed) or first index (indexed) |
| `num_elements` | Number of vertices or indices to draw |
| `num_instances` | Instance count — use `1` for non-instanced |

### sg_draw_ex (extended)

```c
void sg_draw_ex(int base_element, int num_elements, int num_instances,
                int base_vertex, int base_instance);
```

Only supported if `sg_query_features().draw_base_vertex` and/or `.draw_base_instance` are true. **Not guaranteed on all GL 3.3 drivers** — check before use.

---

## sg_apply_pipeline

```c
void sg_apply_pipeline(sg_pipeline pip);
```

Binds the pipeline (shader + render state). Must be called before `sg_apply_bindings` and `sg_apply_uniforms`. Pipeline must be `SG_RESOURCESTATE_VALID`.

---

## sg_apply_bindings

```c
void sg_apply_bindings(const sg_bindings* bindings);
```

Binds vertex buffers, index buffer, texture views, and samplers. All bound resources must be VALID. Must be called after `sg_apply_pipeline`.

```c
sg_bindings bind = {
    .vertex_buffers[0] = vbuf,
    .index_buffer      = ibuf,
    .views[SG_SLOT_tex]    = tex_view,
    .samplers[SG_SLOT_smp] = smp,
};
sg_apply_bindings(&bind);
```

---

## sg_apply_uniforms

```c
void sg_apply_uniforms(int ub_slot, const sg_range* data);
```

Uploads a uniform block. `ub_slot` is the bind slot index — matches `SG_SLOT_*` constants from the sokol-shdc generated header. Call once per uniform block per draw or state change.

```c
vs_params_t vs_p = { .mvp = proj * view * model };
sg_apply_uniforms(SG_SLOT_vs_params, &SG_RANGE(vs_p));

fs_params_t fs_p = { .light_dir = {0.577f,0.577f,0.577f}, .ambient = 0.2f };
sg_apply_uniforms(SG_SLOT_fs_params, &SG_RANGE(fs_p));
```

**Alignment rule:** uniform structs must be 16-byte aligned. Pad manually if needed.

---

## Viewport

```c
void sg_apply_viewport(int x, int y, int w, int h, bool origin_top_left);
void sg_apply_viewportf(float x, float y, float w, float h, bool origin_top_left);
```

For OpenGL: `origin_top_left = true` (GL origin is bottom-left but sokol corrects it).

```c
// Full framebuffer viewport:
sg_apply_viewport(0, 0, sapp_width(), sapp_height(), true);

// Split-screen left half:
sg_apply_viewport(0, 0, sapp_width() / 2, sapp_height(), true);
```

---

## Scissor Rectangle

```c
void sg_apply_scissor_rect(int x, int y, int w, int h, bool origin_top_left);
void sg_apply_scissor_rectf(float x, float y, float w, float h, bool origin_top_left);
```

Clips rendering to a sub-rectangle. Must be within the current viewport.

```c
sg_apply_scissor_rect(100, 100, 400, 300, true);
```

---

## Instanced Rendering

**Pipeline setup (two vertex buffers):**
```c
sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
    .shader = shd,
    .layout = {
        .buffers = {
            [0] = { .stride = sizeof(vertex_t) },
            [1] = { .stride = sizeof(instance_t),
                    .step_func = SG_VERTEXSTEP_PER_INSTANCE },
        },
        .attrs = {
            [SG_ATTR_vs_position]   = { .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT3 },
            [SG_ATTR_vs_texcoord0]  = { .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT2 },
            [SG_ATTR_vs_inst_pos]   = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3 },
            [SG_ATTR_vs_inst_scale] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT },
        },
    },
    .index_type = SG_INDEXTYPE_UINT16,
    .depth = { .compare = SG_COMPAREFUNC_LESS_EQUAL, .write_enabled = true },
    .cull_mode = SG_CULLMODE_BACK,
});
```

**Per-frame update and draw:**
```c
// Update instance data
sg_update_buffer(inst_buf, &SG_RANGE(instances));

// Bind and draw
sg_bindings bind = {
    .vertex_buffers[0] = mesh_vbuf,
    .vertex_buffers[1] = inst_buf,
    .index_buffer = mesh_ibuf,
};
sg_apply_pipeline(pip);
sg_apply_bindings(&bind);
sg_apply_uniforms(SG_SLOT_vs_params, &SG_RANGE(vs_p));
sg_draw(0, num_indices, num_instances);  // num_instances > 1
```

---

## Full Frame Pattern

```c
void frame(void) {
    // 1. Delta time
    uint64_t dt_ticks = stm_laptime(&state.last_time);
    float dt = (float)stm_sec(dt_ticks);

    // 2. Update
    update_game(dt);

    // 3. (Optional offscreen pass first)

    // 4. Main pass
    sg_begin_pass(&(sg_pass){
        .action   = state.pass_action,
        .swapchain = sglue_swapchain(),
        .label    = "main",
    });

    // Opaque objects
    sg_apply_pipeline(state.opaque_pip);
    for (int i = 0; i < state.draw_count; i++) {
        sg_apply_bindings(&state.draws[i].bind);
        sg_apply_uniforms(SG_SLOT_vs_params, &SG_RANGE(state.draws[i].vs_p));
        sg_draw(0, state.draws[i].num_elements, 1);
    }

    // Transparent objects (separate pipeline with blending, depth write off)
    sg_apply_pipeline(state.alpha_pip);
    // ... similar pattern ...

    sg_end_pass();
    sg_commit();   // once per frame, after all passes
}
```

---

## Resource Validity Before Draw

```c
// Always check after creation; skip draw if invalid
if (sg_query_pipeline_state(pip) != SG_RESOURCESTATE_VALID) {
    sg_apply_pipeline(state.error_pip);  // magenta fallback
}
```
