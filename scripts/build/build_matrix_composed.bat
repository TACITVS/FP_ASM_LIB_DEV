@echo off
echo ==============================================================
echo   Testing Library Composition Approach
echo   (Primitives + GCC vs Custom Assembly)
echo ==============================================================
echo.

nasm -f win64 src/asm/fp_core_matrix.asm -o build/obj/fp_core_matrix.o
if %ERRORLEVEL% NEQ 0 (
    echo ASSEMBLY FAILED
    pause
    exit /b 1
)

gcc demo_bench_matrix_composed.c build/obj/fp_core_matrix.o -o bench_composed.exe -I include -O3 -march=native -lm
if %ERRORLEVEL% NEQ 0 (
    echo COMPILATION FAILED
    pause
    exit /b 1
)

echo Build successful! Running test...
echo.
bench_composed.exe

pause
