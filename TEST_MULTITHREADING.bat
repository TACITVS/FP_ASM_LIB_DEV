@echo off
echo ================================================================
echo   Ray Tracer Multithreading Test
echo   Testing Windows native threading (_beginthreadex)
echo ================================================================
echo.

echo Step 1: Cleaning old files...
if exist build\obj\fp_ray_tracer.o del build\obj\fp_ray_tracer.o
if exist ray_tracer_benchmark.exe del ray_tracer_benchmark.exe
echo   Cleanup done.
echo.

echo Step 2: Compiling ray tracer with multithreading...
echo Command: gcc -c src/algorithms/fp_ray_tracer.c -o build/obj/fp_ray_tracer.o -I include -O3 -march=native
gcc -c src/algorithms/fp_ray_tracer.c -o build/obj/fp_ray_tracer.o -I include -O3 -march=native 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo *** COMPILATION FAILED ***
    echo Check errors above
    pause
    exit /b 1
)
echo   Compilation successful!
echo.

echo Step 3: Building benchmark executable...
echo Command: gcc demo_ray_tracer_benchmark.c build/obj/fp_ray_tracer.o -o ray_tracer_benchmark.exe -I include -O3 -march=native -lm
gcc demo_ray_tracer_benchmark.c build/obj/fp_ray_tracer.o -o ray_tracer_benchmark.exe -I include -O3 -march=native -lm 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo *** LINKING FAILED ***
    echo Check errors above
    pause
    exit /b 1
)
echo   Build successful!
echo.

echo Step 4: Running benchmark...
echo ================================================================
echo.
ray_tracer_benchmark.exe

echo.
echo ================================================================
echo   Test Complete!
echo ================================================================
pause
