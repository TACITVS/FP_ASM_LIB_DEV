@echo off
echo ================================================================
echo   Building Ray Tracer Benchmark
echo   Comparing CPU Scalar vs Multithreaded
echo ================================================================
echo.

echo Step 1: Compiling ray tracer core...
gcc -c src/algorithms/fp_ray_tracer.c -o build/obj/fp_ray_tracer.o -I include -O3 -march=native
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile fp_ray_tracer.c
    pause
    exit /b 1
)
echo   fp_ray_tracer.o ✓
echo.

echo Step 2: Building benchmark executable...
gcc demo_ray_tracer_benchmark.c build/obj/fp_ray_tracer.o -o ray_tracer_benchmark.exe -I include -O3 -march=native -pthread -lm
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build ray_tracer_benchmark.exe
    pause
    exit /b 1
)
echo   ray_tracer_benchmark.exe ✓
echo.

echo Step 3: Running benchmark...
echo ================================================================
ray_tracer_benchmark.exe

pause
