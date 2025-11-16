@echo off
echo ================================================================
echo   FP-ASM Library - Build All Tests
echo   Professional Assembly Library for High-Performance Computing
echo ================================================================
echo.

REM Navigate to project root
cd ..\..

echo Step 1: Assembling all reduction modules...
echo.

for %%F in (i32 u32 i16 u16 i8 u8 u64 f32) do (
    echo   - Assembling fp_core_reductions_%%F.asm...
    nasm -f win64 -I src/asm/ src/asm/fp_core_reductions_%%F.asm -o build/obj/fp_core_reductions_%%F.obj
    if %ERRORLEVEL% NEQ 0 (
        echo [ERROR] Failed to assemble fp_core_reductions_%%F.asm
        exit /b 1
    )
)

for %%F in (i32 u32 i16 u16 i8 u8 u64 f32) do (
    echo   - Assembling fp_core_fused_folds_%%F.asm...
    nasm -f win64 -I src/asm/ src/asm/fp_core_fused_folds_%%F.asm -o build/obj/fp_core_fused_folds_%%F.obj
    if %ERRORLEVEL% NEQ 0 (
        echo [ERROR] Failed to assemble fp_core_fused_folds_%%F.asm
        exit /b 1
    )
)

for %%F in (i32 u32 i16 u16 i8 u8 u64 f32) do (
    echo   - Assembling fp_core_fused_maps_%%F.asm...
    nasm -f win64 -I src/asm/ src/asm/fp_core_fused_maps_%%F.asm -o build/obj/fp_core_fused_maps_%%F.obj
    if %ERRORLEVEL% NEQ 0 (
        echo [ERROR] Failed to assemble fp_core_fused_maps_%%F.asm
        exit /b 1
    )
)

echo.
echo [SUCCESS] All assembly modules compiled!
echo.

echo Step 2: Building comprehensive test suites...
echo.

REM Build all comprehensive tests
for %%T in (u64 u32 i32 i16 u16 i8 u8 f32) do (
    echo   - Building test_%%T_comprehensive.exe...
    gcc tests/unit/test_%%T_comprehensive.c ^
        build/obj/fp_core_reductions_%%T.obj ^
        build/obj/fp_core_fused_folds_%%T.obj ^
        build/obj/fp_core_fused_maps_%%T.obj ^
        -o build/bin/test_%%T_comprehensive.exe ^
        -Iinclude
    if %ERRORLEVEL% NEQ 0 (
        echo [ERROR] Failed to build test_%%T_comprehensive.exe
        exit /b 1
    )
)

echo.
echo [SUCCESS] All tests compiled!
echo.

echo ================================================================
echo   Running Test Suites
echo ================================================================
echo.

set TOTAL_TESTS=0
set PASSED_TESTS=0

for %%T in (u64 u32 i32 i16 u16 i8 u8) do (
    set /A TOTAL_TESTS+=1
    echo Running test_%%T_comprehensive.exe...
    build\bin\test_%%T_comprehensive.exe
    if %ERRORLEVEL% EQU 0 (
        set /A PASSED_TESTS+=1
        echo [PASS] %%T tests passed
    ) else (
        echo [FAIL] %%T tests failed
    )
    echo.
)

echo ================================================================
echo   Test Results: %PASSED_TESTS%/%TOTAL_TESTS% test suites passed
echo ================================================================

if %PASSED_TESTS% EQU %TOTAL_TESTS% (
    echo.
    echo [SUCCESS] All tests passed!
    exit /b 0
) else (
    echo.
    echo [ERROR] Some tests failed!
    exit /b 1
)
