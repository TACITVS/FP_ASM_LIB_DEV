@echo off
echo ================================================================
echo   Ray Tracer GPU OpenCL Test
echo   Testing GPU acceleration with OpenCL
echo ================================================================
echo.

echo IMPORTANT: This requires OpenCL SDK to be installed!
echo.
echo If you haven't installed OpenCL yet, see OPENCL_SETUP.md
echo.
echo Common paths for OpenCL on Windows:
echo - NVIDIA GPU SDK: C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v11.x\
echo - Intel OpenCL SDK: C:\Program Files (x86)\Intel\OpenCL SDK\
echo - AMD APP SDK: C:\Program Files (x86)\AMD APP SDK\
echo.

REM Try to detect OpenCL installation
set OPENCL_INCLUDE=
set OPENCL_LIB=

REM Check NVIDIA CUDA (includes OpenCL)
if exist "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA" (
    for /d %%D in ("C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v*") do (
        if exist "%%D\include\CL\cl.h" (
            set OPENCL_INCLUDE=%%D\include
            set OPENCL_LIB=%%D\lib\x64
            echo Found NVIDIA OpenCL SDK at: %%D
        )
    )
)

REM Check Intel OpenCL SDK
if not defined OPENCL_INCLUDE (
    if exist "C:\Program Files (x86)\Intel\OpenCL SDK" (
        set OPENCL_INCLUDE=C:\Program Files (x86)\Intel\OpenCL SDK\include
        set OPENCL_LIB=C:\Program Files (x86)\Intel\OpenCL SDK\lib\x64
        echo Found Intel OpenCL SDK
    )
)

if not defined OPENCL_INCLUDE (
    echo.
    echo *** ERROR: OpenCL SDK not found! ***
    echo.
    echo Please install one of:
    echo 1. NVIDIA CUDA Toolkit (includes OpenCL)
    echo 2. Intel OpenCL SDK
    echo 3. AMD APP SDK
    echo.
    echo See OPENCL_SETUP.md for instructions.
    echo.
    pause
    exit /b 1
)

echo.
echo OpenCL Include: %OPENCL_INCLUDE%
echo OpenCL Library: %OPENCL_LIB%
echo.

echo Step 1: Cleaning old files...
if exist build\obj\fp_ray_tracer.o del build\obj\fp_ray_tracer.o
if exist ray_tracer_gpu.exe del ray_tracer_gpu.exe
echo   Cleanup done.
echo.

echo Step 2: Compiling ray tracer with OpenCL support...
echo Command: gcc -c src/algorithms/fp_ray_tracer.c -o build/obj/fp_ray_tracer.o -I include -I "%OPENCL_INCLUDE%" -DUSE_OPENCL -O3 -march=native
gcc -c src/algorithms/fp_ray_tracer.c -o build/obj/fp_ray_tracer.o -I include -I "%OPENCL_INCLUDE%" -DUSE_OPENCL -O3 -march=native 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo *** COMPILATION FAILED ***
    echo Check errors above
    pause
    exit /b 1
)
echo   Compilation successful!
echo.

echo Step 3: Building GPU demo executable...
echo Command: gcc demo_ray_tracer_gpu.c build/obj/fp_ray_tracer.o -o ray_tracer_gpu.exe -I include -I "%OPENCL_INCLUDE%" -L "%OPENCL_LIB%" -lOpenCL -lm -O3 -march=native
gcc demo_ray_tracer_gpu.c build/obj/fp_ray_tracer.o -o ray_tracer_gpu.exe -I include -I "%OPENCL_INCLUDE%" -L "%OPENCL_LIB%" -lOpenCL -lm -O3 -march=native 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo *** LINKING FAILED ***
    echo Check errors above
    echo.
    echo Common issues:
    echo - Make sure OpenCL.dll is in your PATH
    echo - Check that lib path contains OpenCL.lib
    pause
    exit /b 1
)
echo   Build successful!
echo.

echo Step 4: Checking if kernel file exists...
if not exist src\kernels\ray_tracer.cl (
    echo.
    echo *** ERROR: Kernel file not found! ***
    echo Expected: src\kernels\ray_tracer.cl
    pause
    exit /b 1
)
echo   Kernel file found: src\kernels\ray_tracer.cl
echo.

echo Step 5: Running GPU demo...
echo ================================================================
echo.

REM Make sure OpenCL.dll is findable (add to PATH if needed)
if exist "%OPENCL_LIB%\..\bin" set PATH=%OPENCL_LIB%\..\bin;%PATH%

ray_tracer_gpu.exe

echo.
echo ================================================================
echo   GPU Test Complete!
echo ================================================================
echo.
echo If you saw "Using GPU: [your GPU name]" above, it worked!
echo.
echo Expected output:
echo - Using GPU: GeForce GT 730M (or similar)
echo - OpenCL kernel compiled successfully!
echo - Launching GPU threads...
echo - GPU rendering complete!
echo - output_gpu_640x480.ppm created
echo - output_gpu_1080p.ppm created
echo.
echo Expected performance on GT 730M (384 cores):
echo - 640x480:    ~180 FPS
echo - 1920x1080:  ~30-60 FPS
echo.
echo If you see these speeds, you have REAL-TIME GPU RAY TRACING!
echo ================================================================
pause
