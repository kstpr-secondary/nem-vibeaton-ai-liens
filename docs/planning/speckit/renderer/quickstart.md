# Renderer Quickstart

## Prerequisites

- CMake 3.19+
- Ninja
- Clang / Clang++
- `sokol-shdc` available on `PATH` before shader milestones begin

## Configure

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
```

## Build renderer workstream targets

```bash
cmake --build build --target renderer_app renderer_tests
```

## Run

```bash
./build/renderer_app
./build/renderer_tests
```

## Bootstrap and mock notes

- `USE_RENDERER_MOCKS` swaps the renderer static library to `src/renderer/mocks/` when present.
- `VIBEATON_REQUIRE_SOKOL_SHDC=ON` is the default; once shader files exist, missing `sokol-shdc` is treated as a configuration error.
- Runtime-loaded assets must resolve from `ASSET_ROOT`; shaders are compiled into generated headers.
