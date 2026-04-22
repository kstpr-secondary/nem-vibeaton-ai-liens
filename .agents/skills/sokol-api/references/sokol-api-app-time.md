# sokol-api / app-time

Open this when you need input handling details (key codes, mouse lock, scroll, touch), window/framebuffer queries (`sapp_width`, `sapp_dpi_scale`), mouse cursor control, app lifecycle functions (`sapp_request_quit`, `sapp_cancel_quit`), the full `sokol_time.h` API beyond `stm_laptime`, or `sg_query_features` / `sg_query_pixelformat` capability checks. For the basic frame delta pattern and the `sapp_desc` struct, SKILL.md §§2 and 9 are sufficient.

---

## sokol_app.h — Input Events

### Event struct

```c
typedef struct sapp_event {
    uint64_t frame_count;           // current frame counter
    sapp_event_type type;           // event type
    sapp_keycode key_code;          // KEY_DOWN, KEY_UP
    uint32_t char_code;             // UTF-32 for CHAR events
    bool key_repeat;                // true if key-repeat event
    uint32_t modifiers;             // SAPP_MODIFIER_* bitmask
    sapp_mousebutton mouse_button;  // MOUSE_DOWN, MOUSE_UP
    float mouse_x, mouse_y;         // absolute position (pixels)
    float mouse_dx, mouse_dy;       // relative movement (useful with lock)
    float scroll_x, scroll_y;       // scroll wheel delta
    int num_touches;
    sapp_touchpoint touches[SAPP_MAX_TOUCHPOINTS];
    int window_width, window_height;
    int framebuffer_width, framebuffer_height;
} sapp_event;
```

### Input handling pattern

```c
static void event(const sapp_event* ev) {
    switch (ev->type) {
    case SAPP_EVENTTYPE_KEY_DOWN:
        if (ev->key_code == SAPP_KEYCODE_ESCAPE) sapp_request_quit();
        if (ev->key_code == SAPP_KEYCODE_F11)    sapp_toggle_fullscreen();
        if (ev->key_code == SAPP_KEYCODE_W)       state.input.forward = true;
        break;
    case SAPP_EVENTTYPE_KEY_UP:
        if (ev->key_code == SAPP_KEYCODE_W)       state.input.forward = false;
        break;
    case SAPP_EVENTTYPE_MOUSE_DOWN:
        if (ev->mouse_button == SAPP_MOUSEBUTTON_LEFT)
            sapp_lock_mouse(true);   // FPS mouse lock
        break;
    case SAPP_EVENTTYPE_MOUSE_MOVE:
        if (sapp_mouse_locked()) {
            state.cam.yaw   += ev->mouse_dx * SENSITIVITY;
            state.cam.pitch += ev->mouse_dy * SENSITIVITY;
        }
        break;
    case SAPP_EVENTTYPE_MOUSE_SCROLL:
        state.cam.fov -= ev->scroll_y * 2.0f;
        break;
    case SAPP_EVENTTYPE_RESIZED:
        // sapp_width() / sapp_height() automatically updated
        break;
    case SAPP_EVENTTYPE_QUIT_REQUESTED:
        // sapp_cancel_quit() to abort, or let proceed
        break;
    default: break;
    }
}
```

---

## Key Codes (common subset)

```c
SAPP_KEYCODE_SPACE         = 32
SAPP_KEYCODE_0..9          = 48..57
SAPP_KEYCODE_A..Z          = 65..90
SAPP_KEYCODE_ESCAPE        = 256
SAPP_KEYCODE_ENTER         = 257
SAPP_KEYCODE_TAB           = 258
SAPP_KEYCODE_BACKSPACE     = 259
SAPP_KEYCODE_RIGHT         = 262
SAPP_KEYCODE_LEFT          = 263
SAPP_KEYCODE_DOWN          = 264
SAPP_KEYCODE_UP            = 265
SAPP_KEYCODE_F1            = 290  // ..F12 = 301
SAPP_KEYCODE_LEFT_SHIFT    = 340
SAPP_KEYCODE_LEFT_CONTROL  = 341
SAPP_KEYCODE_LEFT_ALT      = 342
```

Modifier bitmask (check with `ev->modifiers`):
```c
SAPP_MODIFIER_SHIFT = 0x1
SAPP_MODIFIER_CTRL  = 0x2
SAPP_MODIFIER_ALT   = 0x4
SAPP_MODIFIER_LMB   = 0x100   // left button held
SAPP_MODIFIER_RMB   = 0x200
SAPP_MODIFIER_MMB   = 0x400
```

---

## Window & Framebuffer Functions

```c
int   sapp_width(void);        // framebuffer width in pixels
int   sapp_height(void);       // framebuffer height in pixels
float sapp_widthf(void);
float sapp_heightf(void);
float sapp_dpi_scale(void);    // 1.0 or 2.0 for HiDPI
bool  sapp_high_dpi(void);
int   sapp_sample_count(void); // current MSAA sample count
```

---

## Mouse Functions

```c
void sapp_lock_mouse(bool lock);     // FPS-style relative movement
bool sapp_mouse_locked(void);
void sapp_show_mouse(bool show);
bool sapp_mouse_shown(void);
void sapp_set_mouse_cursor(sapp_mouse_cursor cursor);
// Cursors: DEFAULT, ARROW, IBEAM, CROSSHAIR, POINTING_HAND, RESIZE_EW/NS, etc.
```

---

## App Lifecycle Functions

```c
void sapp_request_quit(void);   // sends QUIT_REQUESTED (cancelable in event_cb)
void sapp_cancel_quit(void);    // call from QUIT_REQUESTED handler to abort
void sapp_quit(void);           // immediate hard quit
void sapp_consume_event(void);  // prevent event from propagating further
```

---

## Frame Info

```c
uint64_t sapp_frame_count(void);               // total frames since start
double   sapp_frame_duration(void);            // smoothed frame time (seconds)
double   sapp_frame_duration_unfiltered(void); // raw frame time (seconds)
```

---

## Fullscreen

```c
bool sapp_is_fullscreen(void);
void sapp_toggle_fullscreen(void);
```

---

## sokol_time.h — All Functions

```c
void     stm_setup(void);                              // init once in init_cb
uint64_t stm_now(void);                                // current ticks
uint64_t stm_diff(uint64_t new_t, uint64_t old_t);    // new - old (positive)
uint64_t stm_since(uint64_t start);                    // stm_diff(stm_now(), start)
uint64_t stm_laptime(uint64_t* last_time);             // frame delta helper
uint64_t stm_round_to_common_refresh_rate(uint64_t t); // snap to 60/120/144Hz

double stm_sec(uint64_t ticks);   // ticks → seconds
double stm_ms(uint64_t ticks);    // ticks → milliseconds
double stm_us(uint64_t ticks);    // ticks → microseconds
double stm_ns(uint64_t ticks);    // ticks → nanoseconds
```

### Usage patterns

```c
// Frame delta time:
static uint64_t last_time = 0;
uint64_t lap = stm_laptime(&last_time);  // returns 0 on first call
float dt = (float)stm_sec(lap);

// Elapsed since start:
uint64_t start = stm_now();
// ... work ...
double elapsed_ms = stm_ms(stm_since(start));

// Smooth FPS display:
uint64_t snapped = stm_round_to_common_refresh_rate(lap);
double fps = 1.0 / stm_sec(snapped);
```

| Platform | Clock Source |
|----------|-------------|
| Windows | `QueryPerformanceCounter` |
| macOS | `mach_absolute_time` |
| Linux | `clock_gettime(CLOCK_MONOTONIC)` |
| Emscripten | `emscripten_get_now` |

---

## sokol_gfx.h — Query Functions

### Backend and features

```c
sg_backend  sg_query_backend(void);    // SG_BACKEND_GLCORE on this project
sg_features sg_query_features(void);   // feature flags struct
sg_limits   sg_query_limits(void);     // max texture sizes etc.

// Feature flags (check before using advanced features):
sg_features f = sg_query_features();
f.compute          // compute shaders available
f.draw_base_vertex // base vertex in draw calls
f.draw_base_instance
f.dual_source_blending
```

### Pixel format info

```c
sg_pixelformat_info info = sg_query_pixelformat(SG_PIXELFORMAT_RGBA16F);
info.sample   // can be sampled in shaders
info.filter   // supports linear filtering
info.render   // can be render target
info.blend    // supports blending as target
info.msaa     // supports MSAA
info.depth    // is a depth format
info.bytes_per_pixel
```

### Per-resource state

```c
sg_resource_state sg_query_buffer_state(sg_buffer buf);
sg_resource_state sg_query_image_state(sg_image img);
sg_resource_state sg_query_sampler_state(sg_sampler smp);
sg_resource_state sg_query_shader_state(sg_shader shd);
sg_resource_state sg_query_pipeline_state(sg_pipeline pip);
// Returns: INITIAL | ALLOC | VALID | FAILED | INVALID
```

### Per-image property queries

```c
sg_image_type   sg_query_image_type(sg_image img);
int             sg_query_image_width(sg_image img);
int             sg_query_image_height(sg_image img);
int             sg_query_image_num_slices(sg_image img);
int             sg_query_image_num_mipmaps(sg_image img);
sg_pixel_format sg_query_image_pixelformat(sg_image img);
int             sg_query_image_sample_count(sg_image img);
```

---

## OpenGL 3.3 Core App Descriptor

```c
sapp_desc sokol_main(int argc, char* argv[]) {
    return (sapp_desc){
        .init_cb    = init,
        .frame_cb   = frame,
        .cleanup_cb = cleanup,
        .event_cb   = event,
        .width      = 1280,
        .height     = 720,
        .sample_count = 4,             // 4x MSAA
        .high_dpi   = true,
        .window_title = "My Engine",
        .logger.func  = slog_func,
        .gl = {
            .major_version = 3,
            .minor_version = 3,
        },
    };
}
```
