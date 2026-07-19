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
file(GLOB C_SOURCES   CONFIGURE_DEPENDS src/wrappers/*.c src/algorithms/*.c)

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

# The AVX2 kernels need AVX2+FMA; hint the C compiler too (override with
# -DFPASM_ARCH=... for distributable builds).
set(FPASM_ARCH "native" CACHE STRING "C -march for the wrappers")
if(NOT MSVC)
    # Apply only to C — NASM does not understand -march.
    target_compile_options(fpasm PRIVATE $<$<COMPILE_LANGUAGE:C>:-march=${FPASM_ARCH}>)
endif()

# Tests
enable_testing()
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS tests/test_*.c)
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    target_link_libraries(${test_name} PRIVATE fpasm::fpasm)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

install(TARGETS fpasm EXPORT fpasmTargets
        ARCHIVE DESTINATION lib LIBRARY DESTINATION lib RUNTIME DESTINATION bin)
install(DIRECTORY include/ DESTINATION include/fpasm FILES_MATCHING PATTERN "*.h")
