@echo off
echo ================================================================
echo   Building i16 Comprehensive Test Suite
echo   Testing ALL 12 i16 functions (reductions, folds, maps)
echo   16-wide SIMD - 4X throughput vs i64!
echo ================================================================
echo.

echo Step 1: Assembling i16 modules...
nasm -f win64 src/asm/fp_core_reductions_i16.asm -o build/obj/fp_core_reductions_i16.o
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to assemble reductions_i16
    pause
    exit /b 1
)
echo   fp_core_reductions_i16.o ✓

nasm -f win64 src/asm/fp_core_fused_folds_i16.asm -o build/obj/fp_core_fused_folds_i16.o
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to assemble fused_folds_i16
    pause
    exit /b 1
)
echo   fp_core_fused_folds_i16.o ✓

nasm -f win64 src/asm/fp_core_fused_maps_i16.asm -o build/obj/fp_core_fused_maps_i16.o
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to assemble fused_maps_i16
    pause
    exit /b 1
)
echo   fp_core_fused_maps_i16.o ✓

echo.
echo Step 2: Building test executable...
gcc test_i16_comprehensive.c build/obj/fp_core_reductions_i16.o build/obj/fp_core_fused_folds_i16.o build/obj/fp_core_fused_maps_i16.o -o test_i16_comprehensive.exe -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build test_i16_comprehensive.exe
    pause
    exit /b 1
)
echo   test_i16_comprehensive.exe ✓

echo.
echo Step 3: Running comprehensive tests...
echo ================================================================
test_i16_comprehensive.exe

pause
