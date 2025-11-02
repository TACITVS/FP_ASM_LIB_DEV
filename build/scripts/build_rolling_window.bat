@echo off
echo ========================================================================
echo Algorithm #7: Rolling Window Statistics - Build and Test
echo Functional Composition Pattern Demonstration
echo ========================================================================
echo.

echo [Step 1/4] Compiling rolling window wrapper...
gcc -c src/wrappers/fp_rolling_window.c -o build/obj/fp_rolling_window.o -I include -O3 -march=native
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile fp_rolling_window.c
    exit /b 1
)
echo SUCCESS: fp_rolling_window.o created
echo.

echo [Step 2/4] Checking object file size...
dir build\obj\fp_rolling_window.o | find "fp_rolling_window.o"
echo.

echo [Step 3/4] Building benchmark executable...
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
echo SUCCESS: bench_rolling_window.exe created
echo.

echo [Step 4/4] Running benchmarks...
echo ========================================================================
echo.

echo --- Test 1: Small array (1000 elements, 5 iterations) ---
build\bin\bench_rolling_window.exe 1000 5
echo.
echo.

echo --- Test 2: Medium array (100,000 elements, 10 iterations) ---
build\bin\bench_rolling_window.exe 100000 10
echo.
echo.

echo --- Test 3: Large array (1,000,000 elements, 5 iterations) ---
build\bin\bench_rolling_window.exe 1000000 5
echo.

echo ========================================================================
echo Algorithm #7 Build and Test COMPLETE!
echo ========================================================================
