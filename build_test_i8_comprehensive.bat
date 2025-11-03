@echo off
echo ================================================================
echo   Building i8 Comprehensive Test Suite
echo   Testing ALL 12 i8 functions (reductions, folds, maps)
echo   32-wide SIMD - 8X throughput! (BUT no vpmullb)
echo ================================================================
echo.

echo Step 1: Assembling i8 modules...
nasm -f win64 src/asm/fp_core_reductions_i8.asm -o build/obj/fp_core_reductions_i8.o
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to assemble reductions_i8
    pause
    exit /b 1
)
echo   fp_core_reductions_i8.o ✓

nasm -f win64 src/asm/fp_core_fused_folds_i8.asm -o build/obj/fp_core_fused_folds_i8.o
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to assemble fused_folds_i8
    pause
    exit /b 1
)
echo   fp_core_fused_folds_i8.o ✓

nasm -f win64 src/asm/fp_core_fused_maps_i8.asm -o build/obj/fp_core_fused_maps_i8.o
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to assemble fused_maps_i8
    pause
    exit /b 1
)
echo   fp_core_fused_maps_i8.o ✓

echo.
echo Step 2: Building test executable...
gcc test_i8_comprehensive.c build/obj/fp_core_reductions_i8.o build/obj/fp_core_fused_folds_i8.o build/obj/fp_core_fused_maps_i8.o -o test_i8_comprehensive.exe -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build test_i8_comprehensive.exe
    pause
    exit /b 1
)
echo   test_i8_comprehensive.exe ✓

echo.
echo Step 3: Running comprehensive tests...
echo ================================================================
test_i8_comprehensive.exe

pause
