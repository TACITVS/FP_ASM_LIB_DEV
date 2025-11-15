@echo off
echo ================================================================
echo   GPU Ray Tracer - Performance Profiling Test
echo ================================================================
echo.

echo Step 1: Compiling with OpenCL support and profiling...
gcc -c src/algorithms/fp_ray_tracer.c -o build/obj/fp_ray_tracer.o -I include -I opencl_sdk/include -DUSE_OPENCL -O3 -march=native 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo *** COMPILATION FAILED ***
    pause
    exit /b 1
)
echo   Compilation successful!
echo.

echo Step 2: Linking with OpenCL...
gcc demo_ray_tracer_gpu.c build/obj/fp_ray_tracer.o -o ray_tracer_gpu_profiled.exe -I include -I opencl_sdk/include -L opencl_sdk/lib -lOpenCL -lm -O3 -march=native 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo *** LINKING FAILED ***
    pause
    exit /b 1
)
echo   Build successful!
echo.

echo Step 3: Running GPU demo with profiling...
echo ================================================================
ray_tracer_gpu_profiled.exe

pause
