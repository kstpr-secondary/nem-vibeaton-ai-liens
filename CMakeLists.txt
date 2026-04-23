cmake_minimum_required(VERSION 3.19)
project(vibeaton LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# --- Compiler warnings (not -Werror for hackathon speed) ---
add_compile_options(
    $<$<COMPILE_LANGUAGE:C>:-Wall>
    $<$<COMPILE_LANGUAGE:C>:-Wextra>
    $<$<COMPILE_LANGUAGE:C>:-Wpedantic>
    $<$<COMPILE_LANGUAGE:CXX>:-Wall>
    $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
    $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>
)

# --- Hackathon bootstrap toggles ---
option(USE_RENDERER_MOCKS "Build the renderer target from src/renderer/mocks when present" OFF)
option(USE_ENGINE_MOCKS "Build the engine target from src/engine/mocks when present" OFF)
option(VIBEATON_REQUIRE_SOKOL_SHDC "Fail when shader sources exist but sokol-shdc is unavailable" ON)

# --- Asset path resolution ---
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/paths.h.in
    ${CMAKE_BINARY_DIR}/generated/paths.h
    @ONLY
)

include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# ====================================================================
# FetchContent dependencies
# ====================================================================

FetchContent_Declare(
    sokol
    GIT_REPOSITORY https://github.com/floooh/sokol.git
    GIT_TAG d48d3c09b7d5bc8da3e2d303e109f659a45108eb
    GIT_SHALLOW ON
)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW ON
)

FetchContent_Declare(
    entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG v3.13.2
    GIT_SHALLOW ON
)

FetchContent_Declare(
    cgltf
    GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
    GIT_TAG v1.15
    GIT_SHALLOW ON
)

FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG v2.0.0rc13
    GIT_SHALLOW ON
)

FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
    GIT_SHALLOW ON
)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.8-docking
    GIT_SHALLOW ON
)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 31c1ad37456438565541f4919958214b6e762fb4
    GIT_SHALLOW ON
)

FetchContent_MakeAvailable(glm entt catch2)

FetchContent_GetProperties(sokol)
if(NOT sokol_POPULATED)
    FetchContent_Populate(sokol)
endif()

FetchContent_GetProperties(cgltf)
if(NOT cgltf_POPULATED)
    FetchContent_Populate(cgltf)
endif()

FetchContent_GetProperties(tinyobjloader)
if(NOT tinyobjloader_POPULATED)
    FetchContent_Populate(tinyobjloader)
endif()

FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
    FetchContent_Populate(stb)
endif()

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)

if(NOT TARGET Sokol::sokol)
    add_library(sokol INTERFACE)
    add_library(Sokol::sokol ALIAS sokol)
    target_include_directories(sokol INTERFACE ${sokol_SOURCE_DIR})
endif()

if(NOT TARGET cgltf::cgltf)
    add_library(cgltf INTERFACE)
    add_library(cgltf::cgltf ALIAS cgltf)
    target_include_directories(cgltf INTERFACE ${cgltf_SOURCE_DIR})
endif()

if(NOT TARGET tinyobjloader::tinyobjloader)
    add_library(tinyobjloader_headers INTERFACE)
    add_library(tinyobjloader::tinyobjloader ALIAS tinyobjloader_headers)
    target_include_directories(tinyobjloader_headers INTERFACE ${tinyobjloader_SOURCE_DIR})
endif()

# ====================================================================
# Helper functions
# ====================================================================

function(vibeaton_make_stub out_var target_name kind)
    set(stub_dir "${CMAKE_BINARY_DIR}/generated/bootstrap")
    file(MAKE_DIRECTORY "${stub_dir}")

    if(kind STREQUAL "library")
        set(stub_path "${stub_dir}/${target_name}_stub.cpp")
        file(WRITE "${stub_path}" "namespace vibeaton_bootstrap { void ${target_name}_stub() {} }\n")
    elseif(kind STREQUAL "executable")
        set(stub_path "${stub_dir}/${target_name}_stub.cpp")
        file(WRITE "${stub_path}" "int main() { return 0; }\n")
    else()
        message(FATAL_ERROR "Unknown stub kind: ${kind}")
    endif()

    set(${out_var} "${stub_path}" PARENT_SCOPE)
endfunction()

function(vibeaton_collect_sources out_var)
    set(options)
    set(one_value_args ROOT MOCK_ROOT USE_MOCKS)
    set(multi_value_args INCLUDE_GLOBS EXCLUDE_REGEXES)
    cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    set(using_mocks FALSE)
    if(ARG_USE_MOCKS AND ARG_MOCK_ROOT AND EXISTS "${ARG_MOCK_ROOT}")
        set(search_root "${ARG_MOCK_ROOT}")
        set(using_mocks TRUE)
    else()
        set(search_root "${ARG_ROOT}")
    endif()

    set(collected)
    foreach(include_glob IN LISTS ARG_INCLUDE_GLOBS)
        file(GLOB_RECURSE matches CONFIGURE_DEPENDS "${search_root}/${include_glob}")
        list(APPEND collected ${matches})
    endforeach()

    if(collected)
        list(REMOVE_DUPLICATES collected)
    endif()

    # Skip exclude regexes when mock root is the search root — the regexes are
    # designed to strip mocks/app/tests from the real source tree, not from the
    # mock directory itself.
    if(NOT using_mocks)
        foreach(exclude_regex IN LISTS ARG_EXCLUDE_REGEXES)
            if(collected)
                list(FILTER collected EXCLUDE REGEX "${exclude_regex}")
            endif()
        endforeach()
    endif()

    set(${out_var} "${collected}" PARENT_SCOPE)
endfunction()

function(vibeaton_attach_shader_headers owner_target shader_root aggregate_target)
    file(GLOB shader_files CONFIGURE_DEPENDS "${shader_root}/*.glsl")

    if(NOT TARGET "${aggregate_target}")
        add_custom_target("${aggregate_target}")
    endif()

    if(NOT shader_files)
        return()
    endif()

    find_program(SOKOL_SHDC_EXECUTABLE sokol-shdc)
    if(NOT SOKOL_SHDC_EXECUTABLE)
        if(VIBEATON_REQUIRE_SOKOL_SHDC)
            message(FATAL_ERROR
                "sokol-shdc is required to compile shader sources under ${shader_root}. "
                "Install sokol-shdc or configure with -DVIBEATON_REQUIRE_SOKOL_SHDC=OFF to keep bootstrap mode."
            )
        endif()
        message(WARNING "sokol-shdc not found; shader headers under ${shader_root} will not be generated yet.")
        return()
    endif()

    set(output_dir "${CMAKE_BINARY_DIR}/generated/shaders")
    file(MAKE_DIRECTORY "${output_dir}")

    foreach(shader_file IN LISTS shader_files)
        get_filename_component(shader_name "${shader_file}" NAME_WE)
        set(output_header "${output_dir}/${shader_name}.glsl.h")
        set(shader_target "${aggregate_target}_${shader_name}")

        add_custom_command(
            OUTPUT "${output_header}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${output_dir}"
            COMMAND ${SOKOL_SHDC_EXECUTABLE} -i "${shader_file}" -o "${output_header}" -l glsl430
            DEPENDS "${shader_file}"
            COMMENT "sokol-shdc: compiling ${shader_file}"
            VERBATIM
        )

        add_custom_target("${shader_target}" DEPENDS "${output_header}")
        add_dependencies("${aggregate_target}" "${shader_target}")
    endforeach()

    add_dependencies("${owner_target}" "${aggregate_target}")
endfunction()

# ====================================================================
# Source collection
# ====================================================================

vibeaton_collect_sources(
    renderer_sources
    ROOT "${CMAKE_SOURCE_DIR}/src/renderer"
    MOCK_ROOT "${CMAKE_SOURCE_DIR}/src/renderer/mocks"
    USE_MOCKS "${USE_RENDERER_MOCKS}"
    INCLUDE_GLOBS "*.cpp" "*.c"
    EXCLUDE_REGEXES "/app/" "/tests/" "/mocks/"
)

vibeaton_collect_sources(
    engine_sources
    ROOT "${CMAKE_SOURCE_DIR}/src/engine"
    MOCK_ROOT "${CMAKE_SOURCE_DIR}/src/engine/mocks"
    USE_MOCKS "${USE_ENGINE_MOCKS}"
    INCLUDE_GLOBS "*.cpp" "*.c"
    EXCLUDE_REGEXES "/app/" "/tests/" "/mocks/"
)

vibeaton_collect_sources(
    game_sources
    ROOT "${CMAKE_SOURCE_DIR}/src/game"
    INCLUDE_GLOBS "*.cpp" "*.c"
    EXCLUDE_REGEXES "/mocks/" "/tests/"
)

vibeaton_collect_sources(
    renderer_app_sources
    ROOT "${CMAKE_SOURCE_DIR}/src/renderer/app"
    INCLUDE_GLOBS "*.cpp" "*.c"
)

vibeaton_collect_sources(
    engine_app_sources
    ROOT "${CMAKE_SOURCE_DIR}/src/engine/app"
    INCLUDE_GLOBS "*.cpp" "*.c"
)

vibeaton_collect_sources(
    renderer_test_sources
    ROOT "${CMAKE_SOURCE_DIR}/src/renderer/tests"
    INCLUDE_GLOBS "*.cpp" "*.c"
)

vibeaton_collect_sources(
    engine_test_sources
    ROOT "${CMAKE_SOURCE_DIR}/src/engine/tests"
    INCLUDE_GLOBS "*.cpp" "*.c"
)

if(NOT renderer_sources)
    vibeaton_make_stub(renderer_sources renderer library)
endif()

if(NOT engine_sources)
    vibeaton_make_stub(engine_sources engine library)
endif()

if(NOT game_sources)
    vibeaton_make_stub(game_sources game executable)
endif()

if(NOT renderer_app_sources)
    vibeaton_make_stub(renderer_app_sources renderer_app executable)
endif()

if(NOT engine_app_sources)
    vibeaton_make_stub(engine_app_sources engine_app executable)
endif()

set(renderer_tests_have_real_sources TRUE)
if(NOT renderer_test_sources)
    set(renderer_tests_have_real_sources FALSE)
    vibeaton_make_stub(renderer_test_sources renderer_tests executable)
endif()

set(engine_tests_have_real_sources TRUE)
if(NOT engine_test_sources)
    set(engine_tests_have_real_sources FALSE)
    vibeaton_make_stub(engine_test_sources engine_tests executable)
endif()

# ====================================================================
# Renderer static lib
# ====================================================================

add_library(renderer STATIC ${renderer_sources})

target_include_directories(renderer
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src/renderer
        ${CMAKE_BINARY_DIR}/generated
        ${stb_SOURCE_DIR}
)

target_link_libraries(renderer
    PUBLIC
        Sokol::sokol
        glm::glm
)

target_compile_options(renderer PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-Wno-unused-parameter>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-unused-parameter>
)

# SOKOL_NO_ENTRY: renderer owns sapp_run(); no sokol-provided main().
# SOKOL_GLCORE: desktop OpenGL backend (replaces legacy SOKOL_GLCORE33).
target_compile_definitions(renderer
    PUBLIC
        SOKOL_NO_ENTRY
        SOKOL_GLCORE
)

add_custom_target(renderer_shaders)
vibeaton_attach_shader_headers(renderer "${CMAKE_SOURCE_DIR}/shaders/renderer" renderer_shaders)

# ====================================================================
# Renderer driver executable (standalone demo harness)
# ====================================================================

add_executable(renderer_app ${renderer_app_sources})
target_link_libraries(renderer_app PRIVATE renderer)

# ====================================================================
# Renderer tests (Catch2 when present)
# ====================================================================

add_executable(renderer_tests ${renderer_test_sources})
target_link_libraries(renderer_tests PRIVATE renderer)
if(renderer_tests_have_real_sources)
    target_link_libraries(renderer_tests PRIVATE Catch2::Catch2WithMain)
    catch_discover_tests(renderer_tests)
endif()

# ====================================================================
# Engine static lib
# ====================================================================

add_library(engine STATIC ${engine_sources})

target_include_directories(engine
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src/engine
        ${CMAKE_BINARY_DIR}/generated
)

target_link_libraries(engine
    PUBLIC
        renderer
        EnTT::EnTT
        cgltf::cgltf
        tinyobjloader::tinyobjloader
)

# ====================================================================
# Engine driver executable (standalone demo harness)
# ====================================================================

add_executable(engine_app ${engine_app_sources})
target_link_libraries(engine_app PRIVATE engine)

# ====================================================================
# Engine tests (Catch2 when present)
# ====================================================================

add_executable(engine_tests ${engine_test_sources})
target_link_libraries(engine_tests PRIVATE engine)
if(engine_tests_have_real_sources)
    target_link_libraries(engine_tests PRIVATE Catch2::Catch2WithMain)
    catch_discover_tests(engine_tests)
endif()

# ====================================================================
# Game executable (full game — consumes engine public API)
# ====================================================================

add_executable(game ${game_sources})
target_link_libraries(game PRIVATE engine)

add_custom_target(game_shaders)
vibeaton_attach_shader_headers(game "${CMAKE_SOURCE_DIR}/shaders/game" game_shaders)

# ====================================================================
# Dear ImGui core (backend wiring stays inside renderer implementation)
# ====================================================================

set(imgui_sources
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)

if(EXISTS "${imgui_SOURCE_DIR}/imgui_demo.cpp")
    list(APPEND imgui_sources ${imgui_SOURCE_DIR}/imgui_demo.cpp)
endif()

add_library(imgui_lib STATIC ${imgui_sources})
target_include_directories(imgui_lib PUBLIC ${imgui_SOURCE_DIR})
add_library(imgui::core ALIAS imgui_lib)

# Freeze the hackathon UI path now: Dear ImGui is rendered through
# sokol_gfx/sokol_app via util/sokol_imgui.h, owned by the renderer.
# The implementation translation unit will live in src/renderer/ when added.
add_library(imgui_sokol INTERFACE)
target_include_directories(imgui_sokol INTERFACE
    ${imgui_SOURCE_DIR}
    ${sokol_SOURCE_DIR}
    ${sokol_SOURCE_DIR}/util
)
target_link_libraries(imgui_sokol INTERFACE imgui_lib Sokol::sokol)
add_library(vibeaton::imgui_backend ALIAS imgui_sokol)

target_link_libraries(renderer PRIVATE vibeaton::imgui_backend)
