cmake_minimum_required(VERSION 3.20)

project(FPAsmLib LANGUAGES C ASM_NASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Build type (Release, Debug, RelWithDebInfo, MinSizeRel)" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

file(GLOB ASM_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/asm/*.asm")
file(GLOB WRAPPER_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/wrappers/*.c")

if(ASM_SOURCES STREQUAL "")
    message(FATAL_ERROR "No NASM sources were found under src/asm")
endif()

add_library(fp_asm_objects OBJECT ${ASM_SOURCES})
target_include_directories(fp_asm_objects PRIVATE ${CMAKE_SOURCE_DIR}/include)

if(WIN32)
    set_property(TARGET fp_asm_objects PROPERTY NASM_OBJECT_FORMAT win64)
else()
    set_property(TARGET fp_asm_objects PROPERTY NASM_OBJECT_FORMAT elf64)
    target_compile_options(fp_asm_objects PRIVATE -p ${CMAKE_SOURCE_DIR}/src/asm/nonexecstack.inc)
endif()

add_library(fp_wrapper_objects OBJECT ${WRAPPER_SOURCES})
target_include_directories(fp_wrapper_objects PUBLIC ${CMAKE_SOURCE_DIR}/include)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(fp_wrapper_objects PRIVATE -O3 -march=native)
elseif(MSVC)
    target_compile_options(fp_wrapper_objects PRIVATE /O2)
endif()

add_library(fp_asm STATIC
    $<TARGET_OBJECTS:fp_asm_objects>
    $<TARGET_OBJECTS:fp_wrapper_objects>)

target_include_directories(fp_asm PUBLIC ${CMAKE_SOURCE_DIR}/include)

set(MATH_LIB "")
if(NOT WIN32)
    set(MATH_LIB m)
endif()

set(TEST_TARGETS)
if(WIN32)
    set(TEST_SOURCES)
    file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/tests/test_*.c")
    file(GLOB ROOT_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/test_*.c")
    list(APPEND TEST_SOURCES ${ROOT_TEST_SOURCES})
    list(REMOVE_DUPLICATES TEST_SOURCES)
    list(SORT TEST_SOURCES)

    enable_testing()
    foreach(test_source IN LISTS TEST_SOURCES)
        get_filename_component(test_name "${test_source}" NAME_WE)
        add_executable(${test_name} ${test_source})
        target_link_libraries(${test_name} PRIVATE fp_asm ${MATH_LIB})
        target_include_directories(${test_name} PRIVATE ${CMAKE_SOURCE_DIR}/include)
        if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
            target_link_options(${test_name} PRIVATE -Wl,--allow-multiple-definition)
        endif()
        add_test(NAME ${test_name} COMMAND ${test_name})
        list(APPEND TEST_TARGETS ${test_name})
    endforeach()

    if(TEST_TARGETS)
        add_custom_target(fp_tests DEPENDS ${TEST_TARGETS})
        add_custom_target(run-tests
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            DEPENDS ${TEST_TARGETS}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    endif()
else()
    message(WARNING "Test executables are only available on Win64 where NASM modules match the calling convention.")
endif()

option(FP_BUILD_BENCHMARKS "Build benchmark executables" ON)
if(FP_BUILD_BENCHMARKS AND WIN32)
    file(GLOB BENCH_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/benchmarks/demo_*.c")
    list(SORT BENCH_SOURCES)
    set(BENCH_TARGETS)
    foreach(bench_source IN LISTS BENCH_SOURCES)
        get_filename_component(bench_name "${bench_source}" NAME_WE)
        add_executable(${bench_name} ${bench_source})
        target_link_libraries(${bench_name} PRIVATE fp_asm ${MATH_LIB})
        target_include_directories(${bench_name} PRIVATE ${CMAKE_SOURCE_DIR}/include)
        if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
            target_link_options(${bench_name} PRIVATE -Wl,--allow-multiple-definition)
        endif()
        list(APPEND BENCH_TARGETS ${bench_name})
    endforeach()
    if(BENCH_TARGETS)
        add_custom_target(fp_benchmarks DEPENDS ${BENCH_TARGETS})
    endif()
elseif(FP_BUILD_BENCHMARKS)
    message(WARNING "Benchmarks are disabled on non-Windows platforms because the NASM implementations target the Win64 ABI.")
endif()

add_custom_target(fp_complete ALL DEPENDS fp_asm ${TEST_TARGETS})
