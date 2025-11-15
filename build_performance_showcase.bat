@echo off
echo ==============================================================
echo   Building Performance Showcase Demo
echo   (10,000 Rotating Cubes - Massive Scale Test)
echo ==============================================================
echo.

echo Compiling demo...
gcc demo_performance_showcase.c build/obj/fp_core_matrix.o build/obj/fp_matrix_ops.o -o performance_showcase.exe -I include -O3 -march=native -lm
if %ERRORLEVEL% NEQ 0 (
    echo *** BUILD FAILED ***
    pause
    exit /b 1
)
echo   Build successful!
echo.

echo ==============================================================
echo   Running Performance Showcase...
echo ==============================================================
echo.
performance_showcase.exe

pause
