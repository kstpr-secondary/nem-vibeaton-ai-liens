# sokol-api / work-with-buffers

Open this when the SKILL.md buffer snippets are insufficient — e.g. when you need dynamic or stream-update buffers, `sg_append_buffer` with per-batch offsets, two-step async allocation, overflow detection, or instanced-rendering buffer layout. For a simple immutable vertex or index buffer, the SKILL.md cheatsheet is enough.

---

## Buffer Creation Functions

```c
sg_buffer sg_make_buffer(const sg_buffer_desc* desc);
void      sg_destroy_buffer(sg_buffer buf);
void      sg_update_buffer(sg_buffer buf, const sg_range* data);
int       sg_append_buffer(sg_buffer buf, const sg_range* data);
bool      sg_query_buffer_overflow(sg_buffer buf);
bool      sg_query_buffer_will_overflow(sg_buffer buf, size_t size);
```

---

## Usage Modes

| Mode | Flag | Update function | How often |
|------|------|----------------|-----------|
| Immutable | `.immutable = true` (default) | — (set at creation) | Once |
| Dynamic | `.dynamic_update = true` | `sg_update_buffer()` | Once per frame per buffer |
| Stream | `.stream_update = true` | `sg_append_buffer()` | Many times per frame |

---

## Immutable Vertex Buffer

```c
float vertices[] = {
    // pos (xyz)          uv (uv)
    -0.5f, -0.5f, 0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, 0.0f,  1.0f, 1.0f,
     0.0f,  0.5f, 0.0f,  0.5f, 0.0f,
};
sg_buffer vbuf = sg_make_buffer(&(sg_buffer_desc){
    .data = SG_RANGE(vertices),
    .label = "triangle-vbuf",
});
```

## Immutable Index Buffer

```c
uint16_t indices[] = { 0, 1, 2, 0, 2, 3 };
sg_buffer ibuf = sg_make_buffer(&(sg_buffer_desc){
    .type = SG_BUFFERTYPE_INDEXBUFFER,
    .data = SG_RANGE(indices),
    .label = "quad-ibuf",
});
```

Note: The `SG_BUFFERTYPE_INDEXBUFFER` sets `usage.index_buffer = true`. The new-style API uses `sg_buffer_usage` flags directly; the legacy `.type` field still works.

---

## Dynamic Buffer (once per frame)

```c
// Create (must specify size; no initial data)
sg_buffer dyn_vbuf = sg_make_buffer(&(sg_buffer_desc){
    .size = MAX_VERTICES * sizeof(vertex_t),
    .usage = { .vertex_buffer = true, .dynamic_update = true },
    .label = "dynamic-vbuf",
});

// Update each frame (only once per frame per buffer):
sg_update_buffer(dyn_vbuf, &(sg_range){
    .ptr  = vertex_data,
    .size = vertex_count * sizeof(vertex_t),
});
```

---

## Stream Buffer (multiple appends per frame)

Use `sg_append_buffer` when you build draw batches incrementally within a frame.

```c
sg_buffer stream_buf = sg_make_buffer(&(sg_buffer_desc){
    .size = MAX_VERTICES * sizeof(vertex_t),
    .usage = { .vertex_buffer = true, .stream_update = true },
    .label = "stream-vbuf",
});

// Multiple appends per frame — each returns the byte offset of the appended data:
int offset_a = sg_append_buffer(stream_buf, &SG_RANGE(batch_a));
int offset_b = sg_append_buffer(stream_buf, &SG_RANGE(batch_b));

// Use offset when binding:
sg_bindings bind_a = {
    .vertex_buffers[0] = stream_buf,
    .vertex_buffer_offsets[0] = offset_a,
};
sg_apply_bindings(&bind_a);
sg_draw(0, count_a, 1);

sg_bindings bind_b = {
    .vertex_buffers[0] = stream_buf,
    .vertex_buffer_offsets[0] = offset_b,
};
sg_apply_bindings(&bind_b);
sg_draw(0, count_b, 1);
```

**Overflow check:**
```c
if (sg_query_buffer_will_overflow(stream_buf, batch_size)) {
    // skip this batch or resize
}
// After append:
if (sg_query_buffer_overflow(stream_buf)) {
    // data was silently dropped — log error
}
```

---

## Two-Step (Async) Buffer Creation

For async loading workflows (e.g. loading mesh data in a background thread then uploading):

```c
// Step 1: Allocate slot immediately (returns a valid but uninitialized ID)
sg_buffer buf = sg_alloc_buffer();

// ... load data asynchronously ...

// Step 2: Initialize when data is ready
sg_init_buffer(buf, &(sg_buffer_desc){
    .data = SG_RANGE(loaded_mesh_data),
    .label = "async-mesh",
});

// On error path:
sg_fail_buffer(buf);  // marks state as FAILED

// Cleanup (uninit + dealloc):
sg_uninit_buffer(buf);
sg_dealloc_buffer(buf);
```

---

## Buffer Queries

```c
sg_resource_state sg_query_buffer_state(sg_buffer buf);
// Returns INITIAL, ALLOC, VALID, FAILED, or INVALID

size_t           sg_query_buffer_size(sg_buffer buf);
sg_buffer_usage  sg_query_buffer_usage(sg_buffer buf);
sg_buffer_desc   sg_query_buffer_desc(sg_buffer buf);   // descriptor used at creation
sg_buffer_desc   sg_query_buffer_defaults(const sg_buffer_desc*);  // with defaults filled
```

---

## Instance Buffer Pattern

For instanced rendering, use a second vertex buffer with `SG_VERTEXSTEP_PER_INSTANCE`:

```c
typedef struct { float x, y, z, scale; } instance_t;

sg_buffer inst_buf = sg_make_buffer(&(sg_buffer_desc){
    .size = MAX_INSTANCES * sizeof(instance_t),
    .usage = { .vertex_buffer = true, .dynamic_update = true },
    .label = "instance-buf",
});

// Pipeline layout:
.layout = {
    .buffers = {
        [0] = { .stride = sizeof(vertex_t) },
        [1] = { .stride = sizeof(instance_t), .step_func = SG_VERTEXSTEP_PER_INSTANCE },
    },
    .attrs = {
        [SG_ATTR_vs_position]   = { .buffer_index = 0, .format = SG_VERTEXFORMAT_FLOAT3 },
        [SG_ATTR_vs_inst_pos]   = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT3 },
        [SG_ATTR_vs_inst_scale] = { .buffer_index = 1, .format = SG_VERTEXFORMAT_FLOAT },
    },
},

// Bindings:
sg_bindings bind = {
    .vertex_buffers[0] = mesh_vbuf,
    .vertex_buffers[1] = inst_buf,
    .index_buffer = mesh_ibuf,
};

// Draw 1000 instances:
sg_draw(0, num_indices, 1000);
```
