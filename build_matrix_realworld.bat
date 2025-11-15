@echo off
echo ==============================================================
echo   Building REAL-WORLD Game Engine Matrix Benchmark
echo ==============================================================
echo.

echo Step 1: Assembling fp_core_matrix.asm...
nasm -f win64 src/asm/fp_core_matrix.asm -o build/obj/fp_core_matrix.o
if %ERRORLEVEL% NEQ 0 (
    echo *** ASSEMBLY FAILED ***
    pause
    exit /b 1
)
echo   Assembly successful!
echo.

echo Step 2: Compiling real-world benchmark (FULL GCC OPTIMIZATIONS)...
gcc demo_bench_matrix_realworld.c build/obj/fp_core_matrix.o -o bench_matrix_realworld.exe -I include -O3 -march=native -lm
if %ERRORLEVEL% NEQ 0 (
    echo *** COMPILATION FAILED ***
    pause
    exit /b 1
)
echo   Build successful!
echo   (Using -O3 -march=native with FULL auto-vectorization)
echo.

echo Step 3: Running REAL-WORLD game engine benchmark...
echo ==============================================================
bench_matrix_realworld.exe

pause
