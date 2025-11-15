@echo off
echo ================================================================
echo   Building FP-First 3D Demo: Spinning Cube (Debug Version)
echo ================================================================
echo.

REM Build all necessary f32 object files
if not exist "build\obj\fp_core_fused_folds_f32.o" (
    echo Building fp_core_fused_folds_f32.asm...
    nasm -f win64 src\asm\fp_core_fused_folds_f32.asm -o build\obj\fp_core_fused_folds_f32.o
    if errorlevel 1 (
        echo ERROR: Failed to assemble fused_folds_f32
        pause
        exit /b 1
    )
)

echo Compiling demo_fp_cube_simple.c...
gcc demo_fp_cube_simple.c ^
    build\obj\fp_core_fused_folds_f32.o ^
    -o fp_cube_simple.exe ^
    -I include ^
    -lopengl32 -lgdi32 -luser32 ^
    -O2 ^
    -Wall -Wextra

if errorlevel 1 (
    echo.
    echo ================================================================
    echo   BUILD FAILED
    echo ================================================================
    pause
    exit /b 1
)

echo.
echo ================================================================
echo   BUILD SUCCESSFUL
echo ================================================================
echo.
echo Running demo with debug output...
echo.

fp_cube_simple.exe

if errorlevel 1 (
    echo.
    echo ================================================================
    echo   DEMO CRASHED - Check output above for error
    echo ================================================================
)

echo.
pause
