@echo off
echo ================================================================
echo   Building FP Lighting Test
echo ================================================================
echo.

echo Step 1: Assembling f32 modules...
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
echo Step 2: Compiling C source files...
gcc -c src/engine/fp_lighting.c -o build/obj/fp_lighting.o -I include -O3 -march=native -Wall
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile fp_lighting.c
    pause
    exit /b 1
)
echo   fp_lighting.o ✓

echo.
echo Step 3: Building demo executable...
gcc demo_lighting_test.c build/obj/fp_lighting.o build/obj/fp_core_fused_folds_f32.o build/obj/fp_core_fused_maps_f32.o -o lighting_test.exe -I include -lm
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build demo executable
    pause
    exit /b 1
)
echo   lighting_test.exe ✓

echo.
echo Step 4: Running demo...
echo ================================================================
lighting_test.exe
echo ================================================================

echo.
echo Build and run complete.
pause