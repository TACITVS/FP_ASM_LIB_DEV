@echo off
echo ========================================
echo Building Recursion vs Loop Benchmark
echo ========================================

gcc examples\benchmarks\bench_nb_recursion_vs_loop.c ^
    src\algorithms\fp_naive_bayes.c ^
    build\obj\fp_core_reductions.o ^
    build\obj\fp_core_fused_folds.o ^
    build\obj\fp_core_fused_maps.o ^
    build\obj\fp_core_simple_maps.o ^
    -o bench_nb_recursion_vs_loop.exe ^
    -I include -O3 -foptimize-sibling-calls -Wall -Wextra -lm

if %errorlevel% neq 0 (
    echo.
    echo ========================================
    echo COMPILATION FAILED
    echo ========================================
    exit /b 1
)

echo.
echo ========================================
echo Build successful! Running benchmark...
echo ========================================
echo.

bench_nb_recursion_vs_loop.exe

echo.
echo ========================================
echo Benchmark complete
echo ========================================
