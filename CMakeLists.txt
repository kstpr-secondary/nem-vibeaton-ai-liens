cmake_minimum_required(VERSION 3.19)
project(vibeaton LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# --- Compiler warnings (not -Werror for hackathon speed) ---
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")

# --- Asset path resolution ---
configure_file(${CMAKE_SOURCE_DIR}/cmake/paths.h.in
               ${CMAKE_BINARY_DIR}/generated/paths.h)

# ====================================================================
# FetchContent dependencies
# ====================================================================

include(FetchContent)

# Sokol (sokol_gfx + sokol_app + sokol_time — all in one repo)
FetchContent_Declare(
    sokol
    GIT_REPOSITORY https://github.com/floo7/sokol.git
    GIT_TAG v1.176
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(sokol)

# GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(glm)

# EnTT (ECS)
FetchContent_Declare(
    entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG v3.13.2
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(entt)

# cgtlf (glTF loader)
FetchContent_Declare(
    cgltf
    GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
    GIT_TAG v1.15
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(cgltf)

# tinyobjloader (OBJ fallback)
FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG release_v2.0.0
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(tinyobjloader)

# Catch2 v3 (testing)
FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/contrib)
include(Catch)

# Dear ImGui (HUD/debug overlay)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.8-docking
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(imgui)

# ====================================================================
# sokol-shdc — shader precompilation (custom command, per-workstream)
# ====================================================================
function(add_sokol_shdc target_name glsl_file output_header)
    find_program(SOKOL_SHDC_EXECUTABLE sokol-shdc)
    if(NOT SOKOL_SHDC_EXECUTABLE)
        message(WARNING "sokol-shdc not found — shaders will not compile. Install from: https://github.com/floo7/sokol-shdc")
        return()
    endif()

    set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/generated/shaders")
    set(FULL_OUTPUT "${OUTPUT_DIR}/${output_header}")

    add_custom_command(
        OUTPUT "${FULL_OUTPUT}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}"
        COMMAND ${SOKOL_SHDC_EXECUTABLE} -i "${glsl_file}" -o "${FULL_OUTPUT}" -s glsl330
        DEPENDS "${glsl_file}"
        COMMENT "sokol-shdc: compiling ${glsl_file} → ${output_header}"
        VERBATIM
    )

    add_custom_target(shd_${target_name}_compile ALL
        DEPENDS "${FULL_OUTPUT}"
    )
endfunction()

# ====================================================================
# Renderer static lib
# ====================================================================
add_library(renderer STATIC
    src/renderer/renderer_core.cpp
    src/renderer/pipeline_unlit.cpp
    src/renderer/pipeline_phong.cpp
    src/renderer/mesh_builders.cpp
    src/renderer/camera.cpp
    src/renderer/material.cpp
    src/renderer/skybox.cpp
    src/renderer/line_quad.cpp
    src/renderer/input.cpp
)

target_include_directories(renderer PUBLIC
    ${CMAKE_SOURCE_DIR}/src/renderer
    ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(renderer PUBLIC
    Sokol::sokol
    glm::glm
)

# Sokol headers are mega-headers — suppress warnings via compile option
target_compile_options(renderer PRIVATE -w)

# ====================================================================
# Renderer driver executable (standalone demo harness)
# ====================================================================
add_executable(renderer_app
    src/renderer/app/main.cpp
)

target_link_libraries(renderer_app PRIVATE renderer)

# ====================================================================
# Renderer tests (Catch2)
# ====================================================================
add_executable(renderer_tests
    src/renderer/tests/camera_tests.cpp
    src/renderer/tests/mesh_builder_tests.cpp
)

target_link_libraries(renderer_tests PRIVATE renderer Catch2::Catch2WithMain)
list(APPEND renderer_test_sources src/renderer/tests/*.cpp)

# ====================================================================
# Engine static lib
# ====================================================================
add_library(engine STATIC
    src/engine/ecs.cpp
    src/engine/game_loop.cpp
    src/engine/input_handler.cpp
    src/engine/physics.cpp
    src/engine/collision.cpp
    src/engine/raycast.cpp
    src/engine/asset_bridge.cpp
    src/engine/entity_factory.cpp
)

target_include_directories(engine PUBLIC
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(engine PUBLIC
    renderer
    EnTT::EnTT
    cgtlf::cgtlf
)

# ====================================================================
# Engine driver executable (standalone demo harness)
# ====================================================================
add_executable(engine_app
    src/engine/app/main.cpp
)

target_link_libraries(engine_app PRIVATE engine)

# ====================================================================
# Engine tests (Catch2)
# ====================================================================
add_executable(engine_tests
    src/engine/tests/ecs_tests.cpp
    src/engine/tests/physics_tests.cpp
)

target_link_libraries(engine_tests PRIVATE engine Catch2::Catch2WithMain)

# ====================================================================
# Game executable (full game — consumes engine public API)
# ====================================================================
add_executable(game
    src/game/game.cpp
    src/game/player.cpp
    src/game/asteroid_field.cpp
    src/game/enemy.cpp
    src/game/weapons.cpp
    src/game/hud.cpp
    src/game/vfx.cpp
    src/game/main.cpp
)

target_link_libraries(game PRIVATE engine)

# ====================================================================
# Dear ImGui integration (renderer for OpenGL 3.3 backend)
# ====================================================================
set(IMGUI_DIR ${imgui_SOURCE_DIR})
add_library(imgui_lib STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sokol.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
)

target_include_directories(imgui_lib PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(imgui_lib PUBLIC Sokol::sokol)
target_compile_options(imgui_lib PRIVATE -w)
