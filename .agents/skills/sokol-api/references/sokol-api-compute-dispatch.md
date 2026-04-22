# sokol-api / compute-dispatch

**Stretch goal.** Use this when implementing GPU compute workloads. Compute is not in the MVP scope. Check `sg_query_features().compute` at runtime — it is `false` on many GL 3.3 drivers and will be `false` if sokol_gfx was built without compute support.

---

## Feature Check (always do this first)

```c
sg_features f = sg_query_features();
if (!f.compute) {
    // Compute not available on this backend/driver — fall back to CPU path
}
```

On the project's target backend (`SG_BACKEND_GLCORE` / OpenGL 3.3 Core), compute shaders require **OpenGL 4.3+** under the hood. GL 3.3 Core does **not** support compute. This reference applies if the backend is upgraded or for forward-compatibility documentation.

---

## Compute Pipeline

A compute pipeline sets `.compute = true` and requires a compute shader (no vertex/fragment stages).

```c
sg_pipeline compute_pip = sg_make_pipeline(&(sg_pipeline_desc){
    .compute = true,
    .shader  = compute_shader,   // shader compiled with @cs stage via sokol-shdc
    .label   = "compute-pip",
});
```

---

## sokol-shdc — Compute Shader Annotation

```glsl
@cs cs_name
layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

layout(std430, binding = 0) buffer input_buf  { float data_in[]; };
layout(std430, binding = 1) buffer output_buf { float data_out[]; };

uniform cs_params {
    int count;
};

void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx < uint(count)) {
        data_out[idx] = data_in[idx] * 2.0;
    }
}
@end

@program my_compute cs_name
```

---

## Storage Buffers (SSBO)

Compute shaders read/write storage buffers. Create them with `.storage_buffer = true`:

```c
sg_buffer input_buf = sg_make_buffer(&(sg_buffer_desc){
    .size  = NUM_ELEMENTS * sizeof(float),
    .usage = { .storage_buffer = true, .dynamic_update = true },
    .data  = SG_RANGE(initial_data),
    .label = "compute-input",
});

sg_buffer output_buf = sg_make_buffer(&(sg_buffer_desc){
    .size  = NUM_ELEMENTS * sizeof(float),
    .usage = { .storage_buffer = true, .stream_update = true },
    .label = "compute-output",
});
```

Create views to bind them:

```c
sg_view input_view = sg_make_view(&(sg_view_desc){
    .storage_buffer = { .buffer = input_buf },
});
sg_view output_view = sg_make_view(&(sg_view_desc){
    .storage_buffer = { .buffer = output_buf },
});
```

---

## Compute Pass and Dispatch

```c
// Compute pass: sg_pass.compute = true, no swapchain or attachments
sg_begin_pass(&(sg_pass){
    .compute = true,
    .label   = "compute-pass",
});

sg_apply_pipeline(compute_pip);

// Bind SSBOs via views:
sg_bindings bind = {
    .views[SG_SLOT_input_buf]  = input_view,
    .views[SG_SLOT_output_buf] = output_view,
};
sg_apply_bindings(&bind);

// Upload uniforms:
cs_params_t p = { .count = NUM_ELEMENTS };
sg_apply_uniforms(SG_SLOT_cs_params, &SG_RANGE(p));

// Dispatch — ceil(NUM_ELEMENTS / local_size_x) groups:
int groups_x = (NUM_ELEMENTS + 63) / 64;
sg_dispatch(groups_x, 1, 1);

sg_end_pass();
sg_commit();
```

---

## Reading Results Back to CPU

sokol_gfx has no built-in GPU→CPU readback. Options for this project if needed:

1. **Use the output buffer as a vertex buffer in the next render pass** — GPU-only pipeline, no readback needed.
2. **Persistent mapped buffer (GL-specific)** — outside sokol_gfx; requires raw GL calls via `sg_gl_get_native_buffer` (if available) or a separate GL buffer.

For the hackathon, keep compute results on the GPU and consume them as vertex/storage buffers in subsequent render passes.

---

## Full Compute + Render Pattern

```c
// Frame:
// 1. Compute pass — update particle positions
sg_begin_pass(&(sg_pass){ .compute = true, .label = "particle-update" });
sg_apply_pipeline(particle_compute_pip);
sg_apply_bindings(&compute_bind);
sg_apply_uniforms(SG_SLOT_cs_params, &SG_RANGE(cs_p));
sg_dispatch((NUM_PARTICLES + 63) / 64, 1, 1);
sg_end_pass();

// 2. Render pass — draw particles using compute output as vertex data
sg_begin_pass(&(sg_pass){
    .action   = clear_action,
    .swapchain = sglue_swapchain(),
    .label    = "particle-draw",
});
sg_apply_pipeline(particle_render_pip);
// Bind compute output buffer as vertex buffer:
sg_bindings render_bind = {
    .vertex_buffers[0] = output_buf,  // positions written by compute
};
sg_apply_bindings(&render_bind);
sg_apply_uniforms(SG_SLOT_vs_params, &SG_RANGE(vs_p));
sg_draw(0, NUM_PARTICLES, 1);
sg_end_pass();
sg_commit();
```

---

## Pitfalls

| Problem | Cause | Fix |
|---------|-------|-----|
| `sg_query_features().compute == false` | GL 3.3 or unsupported backend | Check feature; skip or fall back to CPU |
| Pipeline creation fails | Compute shader syntax error or unsupported extension | Check sokol-shdc output; verify GL version |
| Race condition between compute and render pass | No explicit barrier needed — sokol_gfx handles pass ordering | Just don't re-order passes manually |
| Wrong dispatch count | Off-by-one in group size | Use `ceil(N / local_size)` = `(N + local_size - 1) / local_size` |
