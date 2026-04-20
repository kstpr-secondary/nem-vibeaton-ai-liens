# Blueprint Open Issues — Analysis & Proposals

Companion document to `Hackathon Master Blueprint.md`. Each item is judged on whether it is a real problem for a 5–6 hour hackathon with AI agents, and whether a solution is worth the added friction. If friction dominates, the item is forfeited.

---

## 1. Ubuntu system packages (Vulkan + X/Wayland for Sokol)

**Real problem.** `sokol_app` + `sokol_gfx` Vulkan backend will not build or run on a clean Ubuntu 24.04 without the dev packages. Discovering this at T+0 wastes hours. Currently `Ubuntu install list.md` only hints at "clang, cmake, ninja-build" with no graphics packages.

**Proposal.** Add a single pinned apt-install block. One command per machine, no workflow impact:

```bash
sudo apt install -y \
  clang cmake ninja-build pkg-config git \
  xorg-dev libx11-dev libxi-dev libxcursor-dev libxrandr-dev libxinerama-dev \
  libgl1-mesa-dev mesa-common-dev \
  libvulkan-dev vulkan-tools vulkan-validationlayers spirv-tools glslang-tools
```

Also add a `vulkaninfo | head` smoke check to the pre-hackathon infrastructure checklist so the Vulkan ICD is confirmed before T+0. Place the block in `Ubuntu install list.md` and cross-reference from the blueprint's Pre-Hackathon Checklist.

> **Human comment** Already added to Ubuntu install list.md, no need to mention in the blueprint anymore

---

## 2. Dear ImGui

**Not actually a problem — forfeit.** Dear ImGui is already listed in §3.1 Core Libraries (FetchContent, pinned) and referenced as the HUD/debug overlay in §6 Game Scope ("Dear ImGui HUD"). No change needed.

If anything is missing, it is a one-line SKILL reference (`.agents/skills/imgui-api/SKILL.md`) — but only if an agent is actually tasked with non-trivial ImGui work. Defer until SpecKit planning flags it.

---

## 3. Build targets per workstream

**Real problem.** The blueprint mandates frozen interfaces + mocks (§11.2) but never names the CMake targets or the per-workstream build command, so every agent will invent its own. More costly: a renderer-only iteration shouldn't rebuild the game; a game iteration shouldn't recompile the renderer's TUs. Without explicit targets this happens by default and iteration loops balloon.

**Proposal.** Add a short "Build topology" subsection to the blueprint (or the runbook) naming three static libraries and one executable, plus canonical build commands. This is documentation + target naming only — no extra tooling friction.

```
renderer        → static lib   (src/renderer)
engine          → static lib   (src/engine; links renderer)
game            → executable   (src/game; links engine + renderer)
renderer_tests  → executable   (Catch2)
engine_tests    → executable   (Catch2)
```

Canonical per-workstream commands (from repo root, `build/` configured once with Ninja):

```bash
# Renderer iteration (fastest — no engine, no game)
cmake --build build --target renderer renderer_tests

# Engine iteration (reuses cached renderer artifact)
cmake --build build --target engine engine_tests

# Game iteration (reuses cached renderer + engine)
cmake --build build --target game

# Full milestone-sync build
cmake --build build
```

Rule: after milestone merge, the downstream workstream runs the full build **once** to refresh upstream static libs; iteration builds stay target-scoped. Ninja + static libs give us correct incremental behavior for free — no extra caching infra required.

Also add a one-time configure command to `quickstart.md`:
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
```

> **Human comment** Proposal accepted.
---

## 4. Parallel-group file-set enforcement

**Real problem, but heavy enforcement = too much friction.** §10.3 already defines the policy (disjoint `files:` in `Notes`, pause-and-flag on violation). The residual risk is an agent silently editing a file outside its declared set.

A pre-commit hook that parses `TASKS.md`, matches the agent's claimed task by `Owner`, diffs `git diff --cached --name-only` against declared `files:`, and aborts on mismatch is technically straightforward — but adds a new failure mode (hook false-positives mid-crunch) and depends on the hook being installed on every worktree.

**Proposal (lightweight).** Provide an opt-in helper script, not a hook:

```bash
# scripts/check_files.sh <task-id>
# Prints: declared files (from TASKS.md Notes), staged files (git diff --cached),
# and the set difference. Exit 0 always — informational.
```

Agents run it as the last step before commit; humans run it before merging a feature branch. Keep it advisory. If an agent is burning time on false positives, they ignore it; if it catches a real overlap, it saved a conflict. No git hook, no CI gate — the cost/benefit of mandatory enforcement is wrong for a 6-hour timebox.

If overlap incidents actually happen in the first hour, promote it to a `pre-commit` hook mid-hackathon. Start advisory.

> **Human comment** Won't plan for it now, don't add to the document. Will do if a collision is really detected during the implementation.

---

## 5. Shader runtime loading — validate at build time

**Real problem.** Runtime GLSL load + "minimal shader-error reporting" (§4 MVP) means shader syntax errors surface at demo time, not build time. A single bad shader bricks a milestone merge.

**Proposal.** Add a CMake custom target `validate_shaders` that runs `glslangValidator -V` (installed via the §1 apt list) on every `shaders/**/*.glsl` file, producing `.spv` as a byproduct but **not** consumed at runtime (still runtime-loaded GLSL per blueprint). Purely a syntactic / semantic gate.

```cmake
file(GLOB_RECURSE SHADER_SOURCES CONFIGURE_DEPENDS
     ${CMAKE_SOURCE_DIR}/shaders/*.vert.glsl
     ${CMAKE_SOURCE_DIR}/shaders/*.frag.glsl)
add_custom_target(validate_shaders ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Validating shaders..."
    COMMAND glslangValidator -V ${SHADER_SOURCES} -o /dev/null
    SOURCES ${SHADER_SOURCES})
add_dependencies(renderer validate_shaders)
```

Full migration to `sokol-shdc` pre-compilation is a larger change and conflicts with the "runtime file loading is the default" decision (§3.3) — skip it. Validation-only is the right middle ground: catches typos, keeps runtime reload, zero change to the render path.

Additionally, the runtime shader-compile callback in the renderer must log file path + compiler log on failure and **not** crash — one `LOG_ERROR` and a fallback to a magenta/error shader. One-time small implementation task, already implied by "minimal shader-error reporting" but worth calling out as a concrete acceptance criterion on the relevant milestone.

> **Human comment** Agreed with the proposal, but I more and more tend to think about precompiled shaders. 

---

## 6. Asset / shader path resolution

**Real problem, high impact.** Running `./game` from `build/`, from repo root, from an IDE, or from `cmake --build --target run` all produce different working directories. Relative paths like `shaders/renderer/lambertian.frag.glsl` will silently fail in half of them. This kills demo reproducibility.

**Proposal.** Bake the source tree root into a generated header at configure time. Zero runtime lookup, zero environment dependency. One file to maintain.

`cmake/paths.cmake`:
```cmake
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/paths.h.in
    ${CMAKE_BINARY_DIR}/generated/paths.h
    @ONLY)
target_include_directories(renderer PUBLIC ${CMAKE_BINARY_DIR}/generated)
```

`cmake/paths.h.in`:
```c
#pragma once
#define PROJECT_SOURCE_ROOT "@CMAKE_SOURCE_DIR@"
#define SHADER_ROOT         PROJECT_SOURCE_ROOT "/shaders"
#define ASSET_ROOT          PROJECT_SOURCE_ROOT "/assets"
```

Usage:
```cpp
load_shader(SHADER_ROOT "/renderer/lambertian.frag.glsl");
```

Trade-off: the executable is not relocatable across machines without rebuilding. For a hackathon demo run from the build machine, this is fine — and it is far simpler than a runtime asset-root search (env var → exe dir → cwd → fallback). If we later need relocatability for the demo laptop, add a `--asset-root` CLI override; skip until needed.

Rule addendum for `AGENTS.md`: "Never hard-code relative asset/shader paths. Always use `SHADER_ROOT` / `ASSET_ROOT` macros from `paths.h`."

> **Human comment** Agreed with the proposal
---

## Summary of proposed blueprint/runbook additions

| # | Item | Cost | Verdict |
|---|---|---|---|
| 1 | Apt install block + `vulkaninfo` check | 1 command | **Do** |
| 2 | Dear ImGui | — | **Forfeit** (already covered) |
| 3 | Build targets + per-workstream commands | Docs only | **Do** |
| 4 | `check_files.sh` advisory helper | Small script | **Do** (advisory, not a hook) |
| 5 | `validate_shaders` CMake target | ~10 lines CMake | **Do** |
| 6 | Generated `paths.h` with `SHADER_ROOT` / `ASSET_ROOT` | ~15 lines CMake + header | **Do** |
