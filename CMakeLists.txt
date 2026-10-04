# CMake build for FP-ASM. Produces static + shared libraries and an exported
# `fpasm::fpasm` target that game/graphics projects can link with
# `target_link_libraries(mygame PRIVATE fpasm::fpasm)`.
cmake_minimum_required(VERSION 3.16)
project(fpasm LANGUAGES C ASM_NASM VERSION 0.1.0)

if(NOT CMAKE_ASM_NASM_COMPILER)
    message(FATAL_ERROR "NASM not found; install nasm to build the assembly kernels.")
endif()

# NASM object format per platform.
if(WIN32)
    set(CMAKE_ASM_NASM_OBJECT_FORMAT win64)
elseif(APPLE)
    set(CMAKE_ASM_NASM_OBJECT_FORMAT macho64)
else()
    set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
endif()
# Let %include "abi.inc" / "macros.inc" resolve.
set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -I${CMAKE_CURRENT_SOURCE_DIR}/src/asm/")

file(GLOB ASM_SOURCES CONFIGURE_DEPENDS src/asm/*.asm)
file(GLOB C_SOURCES   CONFIGURE_DEPENDS src/wrappers/*.c src/algorithms/*.c src/runtime/*.c)

# --- ISA target ---------------------------------------------------------------
# FPASM_ISA picks the C -march (same presets as the Makefile's ISA=):
#   native (default, not portable) | x86-64-v3 (portable AVX2) | haswell |
#   alderlake | raptorlake (12th-14th gen Core) | x86-64-v4 / avx512 | any -march
# FPASM_ARCH (the old knob), if set, overrides the resulting -march directly.
set(FPASM_ISA "native" CACHE STRING "ISA preset for the C code (see README: Build targets)")
set_property(CACHE FPASM_ISA PROPERTY STRINGS native x86-64-v3 haswell alderlake raptorlake x86-64-v4)
option(FPASM_DISPATCH "Pick AVX-VNNI / AVX-512 kernel variants at runtime" ON)
# Default renderer conventions returned by fp_gfx_default() (include/fp_gfx.h).
# Never links or includes a graphics API; every preset stays usable at runtime.
set(FPASM_GFX "opengl" CACHE STRING "Default renderer conventions")
set_property(CACHE FPASM_GFX PROPERTY STRINGS opengl d3d11 d3d11-lh d3d12 vulkan metal)
set(_fpasm_gfx_enum_opengl   FP_GFX_OPENGL)
set(_fpasm_gfx_enum_generic  FP_GFX_OPENGL)
set(_fpasm_gfx_enum_d3d11    FP_GFX_D3D11)
set(_fpasm_gfx_enum_dx11     FP_GFX_D3D11)
set(_fpasm_gfx_enum_d3d11-lh FP_GFX_D3D11_LH)
set(_fpasm_gfx_enum_d3d12    FP_GFX_D3D12)
set(_fpasm_gfx_enum_vulkan   FP_GFX_VULKAN)
set(_fpasm_gfx_enum_metal    FP_GFX_METAL)
set(_fpasm_gfx_enum ${_fpasm_gfx_enum_${FPASM_GFX}})
if(NOT _fpasm_gfx_enum)
    message(FATAL_ERROR "FP-ASM: unknown FPASM_GFX=${FPASM_GFX} (opengl, d3d11, d3d11-lh, d3d12, vulkan, metal)")
endif()
option(FPASM_BUILD_TOOLS "Build the fpasm-info diagnostic tool" ON)

include(CheckCCompilerFlag)
if(FPASM_ISA STREQUAL "avx512")
    set(_fpasm_march x86-64-v4)
elseif(FPASM_ISA STREQUAL "raptorlake")
    check_c_compiler_flag(-march=raptorlake FPASM_CC_HAS_RAPTORLAKE)
    if(FPASM_CC_HAS_RAPTORLAKE)
        set(_fpasm_march raptorlake)
    else()
        message(STATUS "FP-ASM: compiler lacks -march=raptorlake; using alderlake (same ISA)")
        set(_fpasm_march alderlake)
    endif()
else()
    set(_fpasm_march ${FPASM_ISA})
endif()
if(FPASM_ARCH)
    set(_fpasm_march ${FPASM_ARCH})
endif()
if(NOT MSVC)
    check_c_compiler_flag(-march=${_fpasm_march} FPASM_CC_HAS_MARCH_${_fpasm_march})
    if(NOT FPASM_CC_HAS_MARCH_${_fpasm_march})
        message(FATAL_ERROR "FP-ASM: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION} does not support "
                            "-march=${_fpasm_march} (FPASM_ISA=${FPASM_ISA}).")
    endif()
endif()
if(FPASM_DISPATCH)
    set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -DFP_DISPATCH")
endif()
message(STATUS "FP-ASM: ISA=${FPASM_ISA} -> -march=${_fpasm_march}; dispatch=${FPASM_DISPATCH}; gfx=${FPASM_GFX}")

add_library(fpasm ${ASM_SOURCES} ${C_SOURCES})
add_library(fpasm::fpasm ALIAS fpasm)

target_include_directories(fpasm PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/fpasm>)
target_compile_features(fpasm PRIVATE c_std_11)
set_target_properties(fpasm PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(NOT WIN32)
    target_link_libraries(fpasm PUBLIC m)
endif()

if(NOT MSVC)
    # Apply only to C — NASM does not understand -march.
    target_compile_options(fpasm PRIVATE $<$<COMPILE_LANGUAGE:C>:-march=${_fpasm_march}>)
    # CPU detection/dispatch must run on any x86-64 to explain a mismatch
    # instead of dying on SIGILL: compile them for the baseline (these
    # source-level options come after the target's, so they win).
    set_source_files_properties(src/runtime/fp_cpu.c src/runtime/fp_dispatch.c
        PROPERTIES COMPILE_OPTIONS "-march=x86-64;-mtune=generic")
endif()
target_compile_definitions(fpasm PRIVATE
    $<$<COMPILE_LANGUAGE:C>:FPASM_ISA_NAME=${FPASM_ISA} FPASM_MARCH=${_fpasm_march}>
    $<$<COMPILE_LANGUAGE:C>:FPASM_GFX_DEFAULT=${_fpasm_gfx_enum} FPASM_GFX_NAME=${FPASM_GFX}>
    $<$<AND:$<COMPILE_LANGUAGE:C>,$<BOOL:${FPASM_DISPATCH}>>:FP_DISPATCH>)

if(FPASM_BUILD_TOOLS)
    add_executable(fpasm-info tools/fpasm_info.c)
    target_link_libraries(fpasm-info PRIVATE fpasm::fpasm)
    if(NOT MSVC)
        target_compile_options(fpasm-info PRIVATE -march=x86-64 -mtune=generic)
    endif()
endif()

# Tests
enable_testing()
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS tests/test_*.c)
if(FPASM_BUILD_TOOLS)
    # Runs first; a build/CPU mismatch is then a readable failure, not SIGILL.
    add_test(NAME check_cpu COMMAND fpasm-info --check)
endif()
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE fpasm::fpasm)
    if(NOT MSVC)
        target_compile_options(${test_name} PRIVATE -march=${_fpasm_march})
    endif()
    add_test(NAME ${test_name} COMMAND ${test_name})
    if(FPASM_BUILD_TOOLS)
        set_tests_properties(${test_name} PROPERTIES DEPENDS check_cpu)
    endif()
endforeach()

if(FPASM_BUILD_TOOLS)
    install(TARGETS fpasm-info RUNTIME DESTINATION bin)
endif()
install(TARGETS fpasm EXPORT fpasmTargets
        ARCHIVE DESTINATION lib LIBRARY DESTINATION lib RUNTIME DESTINATION bin)
install(DIRECTORY include/ DESTINATION include/fpasm FILES_MATCHING PATTERN "*.h")
install(DIRECTORY shaders/ DESTINATION share/fpasm/shaders)
