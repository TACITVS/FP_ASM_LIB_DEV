@echo off
echo ================================================================
echo   Building u32 Comprehensive Test Suite
echo   Testing ALL 12 u32 functions (reductions, folds, maps)
echo ================================================================
echo.

echo Step 1: Assembling u32 modules...
nasm -f win64 src/asm/fp_core_reductions_u32.asm -o build/obj/fp_core_reductions_u32.o
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to assemble reductions_u32
    pause
    exit /b 1
)
echo   fp_core_reductions_u32.o ✓

nasm -f win64 src/asm/fp_core_fused_folds_u32.asm -o build/obj/fp_core_fused_folds_u32.o
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to assemble fused_folds_u32
    pause
    exit /b 1
)
echo   fp_core_fused_folds_u32.o ✓

nasm -f win64 src/asm/fp_core_fused_maps_u32.asm -o build/obj/fp_core_fused_maps_u32.o
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to assemble fused_maps_u32
    pause
    exit /b 1
)
echo   fp_core_fused_maps_u32.o ✓

echo.
echo Step 2: Building test executable...
gcc test_u32_comprehensive.c build/obj/fp_core_reductions_u32.o build/obj/fp_core_fused_folds_u32.o build/obj/fp_core_fused_maps_u32.o -o test_u32_comprehensive.exe -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build test_u32_comprehensive.exe
    pause
    exit /b 1
)
echo   test_u32_comprehensive.exe ✓

echo.
echo Step 3: Running comprehensive tests...
echo ================================================================
test_u32_comprehensive.exe

pause
