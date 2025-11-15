@echo off
echo ==============================================================
echo   Building REAL-WORLD Game Engine Matrix Benchmark v2
echo   NOW WITH BATCHED VERTEX TRANSFORM!
echo ==============================================================
echo.

echo Step 1: Assembling fp_core_matrix.asm with BATCHED function...
nasm -f win64 src/asm/fp_core_matrix.asm -o build/obj/fp_core_matrix.o
if %ERRORLEVEL% NEQ 0 (
    echo *** ASSEMBLY FAILED ***
    pause
    exit /b 1
)
echo   Assembly successful!
echo.

echo Step 2: Compiling real-world benchmark...
gcc demo_bench_matrix_realworld.c build/obj/fp_core_matrix.o -o bench_matrix_realworld.exe -I include -O3 -march=native -fno-tree-vectorize -lm
if %ERRORLEVEL% NEQ 0 (
    echo *** COMPILATION FAILED ***
    pause
    exit /b 1
)
echo   Build successful!
echo.

echo Step 3: Running REAL-WORLD game engine benchmark...
echo   Expected: MVP chains @ 4x speedup
echo   Expected: Vertex transform @ 3-4x speedup (WORLD-CLASS!)
echo ==============================================================
bench_matrix_realworld.exe

pause
