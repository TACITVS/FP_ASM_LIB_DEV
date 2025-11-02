@echo off
echo ========================================================================
echo Rebuilding Algorithm #7 with Fixed Tolerances
echo ========================================================================
echo.

echo [1/2] Recompiling benchmark...
gcc benchmarks/demo_bench_rolling_window.c ^
    build/obj/fp_rolling_window.o ^
    build/obj/fp_core_reductions.o ^
    build/obj/fp_core_descriptive_stats.o ^
    -o build/bin/bench_rolling_window.exe ^
    -I include -O3 -march=native
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to build benchmark
    exit /b 1
)
echo SUCCESS: bench_rolling_window.exe rebuilt
echo.

echo [2/2] Running full benchmark suite...
echo ========================================================================
echo.

build\bin\bench_rolling_window.exe 100000 10
echo.

echo ========================================================================
echo Rebuild and Test COMPLETE!
echo ========================================================================
