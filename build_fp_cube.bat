@echo off
echo ================================================================
echo   Building FP-First 3D Demo: Spinning Cube
echo ================================================================
echo.

REM Check if we need to build FP library object files
if not exist "build\obj\fp_core_fused_folds_f32.o" (
    echo Building FP library...
    nasm -f win64 src\asm\fp_core_fused_folds_f32.asm -o build\obj\fp_core_fused_folds_f32.o
    if errorlevel 1 (
        echo ERROR: Failed to assemble FP library
        pause
        exit /b 1
    )
    echo FP library built successfully
    echo.
)

echo Compiling demo_fp_cube.c...
gcc demo_fp_cube.c ^
    build\obj\fp_core_fused_folds_f32.o ^
    -o fp_cube.exe ^
    -I include ^
    -lopengl32 -lgdi32 -luser32 ^
    -O3 -march=native ^
    -Wall

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
echo Executable: fp_cube.exe
echo.
echo Running demo...
echo.

fp_cube.exe

echo.
echo Demo finished.
pause
