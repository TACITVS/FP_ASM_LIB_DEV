@echo off
echo ================================================================
echo   Building i32 Comprehensive Test Suite
echo   Testing ALL 12 i32 functions (reductions, folds, maps)
echo ================================================================
echo.

echo Step 1: Assembling i32 modules...
nasm -f win64 src/asm/fp_core_reductions_i32.asm -o build/obj/fp_core_reductions_i32.o
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to assemble reductions_i32
    pause
    exit /b 1
)
echo   fp_core_reductions_i32.o ✓

nasm -f win64 src/asm/fp_core_fused_folds_i32.asm -o build/obj/fp_core_fused_folds_i32.o
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to assemble fused_folds_i32
    pause
    exit /b 1
)
echo   fp_core_fused_folds_i32.o ✓

nasm -f win64 src/asm/fp_core_fused_maps_i32.asm -o build/obj/fp_core_fused_maps_i32.o
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to assemble fused_maps_i32
    pause
    exit /b 1
)
echo   fp_core_fused_maps_i32.o ✓

echo.
echo Step 2: Building test executable...
gcc test_i32_comprehensive.c build/obj/fp_core_reductions_i32.o build/obj/fp_core_fused_folds_i32.o build/obj/fp_core_fused_maps_i32.o -o test_i32_comprehensive.exe -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build test_i32_comprehensive.exe
    pause
    exit /b 1
)
echo   test_i32_comprehensive.exe ✓

echo.
echo Step 3: Running comprehensive tests...
echo ================================================================
test_i32_comprehensive.exe

pause
