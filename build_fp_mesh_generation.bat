@echo off
echo ================================================================
echo   Building FP Mesh Generation Demo
echo ================================================================
echo.

echo Step 1: Compiling fp_mesh_generation.c...
gcc -c src/engine/fp_mesh_generation.c -o build/obj/fp_mesh_generation.o -I include -O3 -march=native -Wall
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile fp_mesh_generation.c
    pause
    exit /b 1
)
echo   fp_mesh_generation.o ✓

echo.
echo Step 2: Building demo executable...
gcc demo_fp_mesh_generation.c build/obj/fp_mesh_generation.o -o fp_mesh_generation.exe -I include -lm
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build demo executable
    pause
    exit /b 1
)
echo   fp_mesh_generation.exe ✓

echo.
echo Step 3: Running demo...
echo ================================================================
fp_mesh_generation.exe
echo ================================================================

echo.
echo Build and run complete.
pause