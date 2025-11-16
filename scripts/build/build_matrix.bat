@echo off
echo ==============================================================
echo   Building Module 7: 3D Matrix Math (Game Engine Foundation)
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

echo Step 2: Compiling and linking demo...
gcc demo_bench_matrix.c build/obj/fp_core_matrix.o -o bench_matrix.exe -I include -O3 -march=native -fno-tree-vectorize
if %ERRORLEVEL% NEQ 0 (
    echo *** COMPILATION FAILED ***
    pause
    exit /b 1
)
echo   Build successful!
echo.

echo Step 3: Running tests and benchmarks...
echo ==============================================================
bench_matrix.exe

pause
