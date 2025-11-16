@echo off
echo Building critical fixes test suite...
echo.

REM Assemble all reduction ASM files
echo Assembling reduction functions...
nasm -f win64 -I ../../src/asm/ ../../src/asm/fp_core_reductions_i32.asm -o ../../build/obj/fp_core_reductions_i32.obj
nasm -f win64 -I ../../src/asm/ ../../src/asm/fp_core_reductions_u32.asm -o ../../build/obj/fp_core_reductions_u32.obj
nasm -f win64 -I ../../src/asm/ ../../src/asm/fp_core_reductions_i16.asm -o ../../build/obj/fp_core_reductions_i16.obj
nasm -f win64 -I ../../src/asm/ ../../src/asm/fp_core_reductions_u16.asm -o ../../build/obj/fp_core_reductions_u16.obj
nasm -f win64 -I ../../src/asm/ ../../src/asm/fp_core_reductions_i8.asm -o ../../build/obj/fp_core_reductions_i8.obj
nasm -f win64 -I ../../src/asm/ ../../src/asm/fp_core_reductions_u8.asm -o ../../build/obj/fp_core_reductions_u8.obj
nasm -f win64 -I ../../src/asm/ ../../src/asm/fp_core_reductions_u64.asm -o ../../build/obj/fp_core_reductions_u64.obj
nasm -f win64 -I ../../src/asm/ ../../src/asm/fp_core_reductions_f32.asm -o ../../build/obj/fp_core_reductions_f32.obj

if %ERRORLEVEL% NEQ 0 (
    echo Assembly failed!
    exit /b 1
)

echo.
echo Compiling and linking test...
gcc -o ../../build/bin/test_reductions_critical.exe ../../tests/critical/test_reductions_critical.c ^
    ../../build/obj/fp_core_reductions_i32.obj ^
    ../../build/obj/fp_core_reductions_u32.obj ^
    ../../build/obj/fp_core_reductions_i16.obj ^
    ../../build/obj/fp_core_reductions_u16.obj ^
    ../../build/obj/fp_core_reductions_i8.obj ^
    ../../build/obj/fp_core_reductions_u8.obj ^
    ../../build/obj/fp_core_reductions_u64.obj ^
    ../../build/obj/fp_core_reductions_f32.obj ^
    -I../../include -O2 -mavx2

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build successful!
    echo.
    echo ========================================
    echo Running Critical Fixes Test Suite...
    echo ========================================
    echo.
    ..\..\build\bin\test_reductions_critical.exe
    set TEST_RESULT=%ERRORLEVEL%
    echo.
    if %TEST_RESULT% EQU 0 (
        echo ========================================
        echo ALL TESTS PASSED!
        echo ========================================
    ) else (
        echo ========================================
        echo TESTS FAILED!
        echo ========================================
        exit /b 1
    )
) else (
    echo.
    echo Build failed!
    exit /b 1
)
