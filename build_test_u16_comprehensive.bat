@echo off
echo ================================================================
echo   Building u16 Comprehensive Test Suite
echo   Testing ALL 12 u16 functions (reductions, folds, maps)
echo   16-wide SIMD - 4X throughput vs u64!
echo ================================================================
echo.

echo Step 1: Assembling u16 modules...
nasm -f win64 src/asm/fp_core_reductions_u16.asm -o build/obj/fp_core_reductions_u16.o
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to assemble reductions_u16
    pause
    exit /b 1
)
echo   fp_core_reductions_u16.o ✓

nasm -f win64 src/asm/fp_core_fused_folds_u16.asm -o build/obj/fp_core_fused_folds_u16.o
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to assemble fused_folds_u16
    pause
    exit /b 1
)
echo   fp_core_fused_folds_u16.o ✓

nasm -f win64 src/asm/fp_core_fused_maps_u16.asm -o build/obj/fp_core_fused_maps_u16.o
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to assemble fused_maps_u16
    pause
    exit /b 1
)
echo   fp_core_fused_maps_u16.o ✓

echo.
echo Step 2: Building test executable...
gcc test_u16_comprehensive.c build/obj/fp_core_reductions_u16.o build/obj/fp_core_fused_folds_u16.o build/obj/fp_core_fused_maps_u16.o -o test_u16_comprehensive.exe -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build test_u16_comprehensive.exe
    pause
    exit /b 1
)
echo   test_u16_comprehensive.exe ✓

echo.
echo Step 3: Running comprehensive tests...
echo ================================================================
test_u16_comprehensive.exe

pause
