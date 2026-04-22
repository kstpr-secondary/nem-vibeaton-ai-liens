# Project Summary: Sokol

Sokol is a collection of simple, STB-style cross-platform libraries for C and C++, written in C. It is designed to be lightweight, easy to integrate, and highly portable, with a strong emphasis on WebAssembly as a first-class citizen.

## Core Libraries

The core headers are standalone and provide foundational functionality for cross-platform development:

- **sokol_gfx.h**: A modern 3D-API wrapper supporting GL/GLES3/WebGL2, Metal, D3D11, and WebGPU.
- **sokol_app.h**: An application framework wrapper handling window creation, 3D context initialization, and input (keyboard, mouse, touch).
- **sokol_time.h**: Simple cross-platform time measurement.
- **sokol_audio.h**: A minimal buffer-streaming audio playback library.
- **sokol_fetch.h**: Asynchronous data streaming from HTTP and the local filesystem.
- **sokol_args.h**: A unified command-line and URL argument parser.
- **sokol_log.h**: A standard logging callback used across the Sokol headers.
- **sokol_glue.h**: Glue code to simplify using `sokol_app.h` together with `sokol_gfx.h`.

## Utility Libraries (in `util/`)

Utility libraries provide higher-level functionality built on top of the core headers:

- **sokol_gl.h**: OpenGL 1.x style immediate-mode rendering API.
- **sokol_imgui.h** / **sokol_nuklear.h**: Rendering backends for Dear ImGui and Nuklear.
- **sokol_fontstash.h**: Rendering backend for the fontstash library.
- **sokol_debugtext.h**: Simple text renderer using vintage home computer fonts.
- **sokol_shape.h**: Utilities for generating simple geometric shapes.
- **sokol_color.h**: X11-style color constants and utilities.
- **sokol_memtrack.h**: Tools for tracking memory allocations within Sokol.
- **sokol_spine.h**: A Sokol-style wrapper around the Spine C runtime.

## Architecture and Design Goals

- **Header-Only Integration**: Following the STB style, libraries are implemented in header files. A single C/C++ file must define an implementation macro (e.g., `#define SOKOL_GFX_IMPL`) before including the header to create the implementation.
- **C99 Compatibility**: Written in clean C99 for maximum compatibility with compilers and other languages.
- **Minimal Dependencies**: Designed to have zero or minimal external dependencies.
- **Cross-Platform**: Supports Windows, macOS, Linux, iOS, Android, and WebAssembly.
- **Modern 3D API Wrapper**: `sokol_gfx.h` provides a common, modern API (inspired by Metal and WebGPU) across different backend APIs.

## Project Structure

- **Root Directory**: Contains the core library headers (`sokol_*.h`).
- **`util/`**: Contains the utility library headers.
- **`bindgen/`**: Contains Python scripts and metadata for generating automatic language bindings (Zig, Rust, Odin, Nim, etc.).
- **`tests/`**: Contains a comprehensive suite of functional and compilation tests for various platforms and backends.
- **`assets/`**: Project-related assets like logos.
- **`.github/workflows/`**: CI/CD pipelines for building, testing, and generating bindings.

## Language Bindings

Sokol provides "official" language bindings that are automatically updated:
- Zig, Rust, Odin, Nim, D, JAI, and C3.
