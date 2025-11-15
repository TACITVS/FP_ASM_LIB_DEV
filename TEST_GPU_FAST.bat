@echo off
echo ================================================================
echo   GPU Ray Tracer - Persistent Context Test
echo   This is the FAST version - real GPU performance!
echo ================================================================
echo.

echo Step 1: Compiling with OpenCL support...
gcc -c src/algorithms/fp_ray_tracer.c -o build/obj/fp_ray_tracer.o -I include -I opencl_sdk/include -DUSE_OPENCL -O3 -march=native 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo *** COMPILATION FAILED ***
    pause
    exit /b 1
)
echo   Compilation successful!
echo.

echo Step 2: Linking with OpenCL...
gcc demo_ray_tracer_gpu_fast.c build/obj/fp_ray_tracer.o -o ray_tracer_gpu_fast.exe -I include -I opencl_sdk/include -L opencl_sdk/lib -lOpenCL -lm -O3 -march=native 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo *** LINKING FAILED ***
    pause
    exit /b 1
)
echo   Build successful!
echo.

echo Step 3: Running GPU demo with persistent context...
echo ================================================================
ray_tracer_gpu_fast.exe

pause
