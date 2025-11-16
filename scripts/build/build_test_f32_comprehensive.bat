@echo off
echo ================================================================
echo   Building f32 Comprehensive Test Suite
echo   Testing ALL 12 f32 functions (reductions, folds, maps)
echo   Features FMA for dotp and axpy!
echo ================================================================
echo.

echo Step 1: Assembling f32 modules...
nasm -f win64 src/asm/fp_core_reductions_f32.asm -o build/obj/fp_core_reductions_f32.o
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to assemble reductions_f32
    pause
    exit /b 1
)
echo   fp_core_reductions_f32.o ✓

nasm -f win64 src/asm/fp_core_fused_folds_f32.asm -o build/obj/fp_core_fused_folds_f32.o
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to assemble fused_folds_f32
    pause
    exit /b 1
)
echo   fp_core_fused_folds_f32.o ✓

nasm -f win64 src/asm/fp_core_fused_maps_f32.asm -o build/obj/fp_core_fused_maps_f32.o
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to assemble fused_maps_f32
    pause
    exit /b 1
)
echo   fp_core_fused_maps_f32.o ✓

echo.
echo Step 2: Building test executable...
gcc test_f32_comprehensive.c build/obj/fp_core_reductions_f32.o build/obj/fp_core_fused_folds_f32.o build/obj/fp_core_fused_maps_f32.o -o test_f32_comprehensive.exe -I include -lm
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build test_f32_comprehensive.exe
    pause
    exit /b 1
)
echo   test_f32_comprehensive.exe ✓

echo.
echo Step 3: Running comprehensive tests...
echo ================================================================
test_f32_comprehensive.exe

pause
