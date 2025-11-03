@echo off
echo ================================================================
echo   Building u64 Comprehensive Test Suite
echo   Testing ALL 12 u64 functions (reductions, folds, maps)
echo ================================================================
echo.

echo Step 1: Assembling u64 modules...
nasm -f win64 src/asm/fp_core_reductions_u64.asm -o build/obj/fp_core_reductions_u64.o
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to assemble reductions_u64
    pause
    exit /b 1
)
echo   fp_core_reductions_u64.o ✓

nasm -f win64 src/asm/fp_core_fused_folds_u64.asm -o build/obj/fp_core_fused_folds_u64.o
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to assemble fused_folds_u64
    pause
    exit /b 1
)
echo   fp_core_fused_folds_u64.o ✓

nasm -f win64 src/asm/fp_core_fused_maps_u64.asm -o build/obj/fp_core_fused_maps_u64.o
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to assemble fused_maps_u64
    pause
    exit /b 1
)
echo   fp_core_fused_maps_u64.o ✓

echo.
echo Step 2: Building test executable...
gcc test_u64_comprehensive.c build/obj/fp_core_reductions_u64.o build/obj/fp_core_fused_folds_u64.o build/obj/fp_core_fused_maps_u64.o -o test_u64_comprehensive.exe -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build test_u64_comprehensive.exe
    pause
    exit /b 1
)
echo   test_u64_comprehensive.exe ✓

echo.
echo Step 3: Running comprehensive tests...
echo ================================================================
test_u64_comprehensive.exe

pause
