@echo off
echo ================================================================
echo   Building Ray Tracer Demo
echo   FP-ASM Real-Time + Offline Rendering
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

echo Step 2: Building demo executable...
gcc demo_ray_tracer_simple.c build/obj/fp_ray_tracer.o -o ray_tracer_demo.exe -I include -O3 -march=native -lm
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build ray_tracer_demo.exe
    pause
    exit /b 1
)
echo   ray_tracer_demo.exe ✓
echo.

echo Step 3: Running ray tracer demo...
echo ================================================================
ray_tracer_demo.exe

pause
