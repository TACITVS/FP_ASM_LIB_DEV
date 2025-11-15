@echo off
echo ================================================================
echo   Building 3D Math Correctness Test
echo ================================================================
echo.

:: Create build directories if they don't exist
if not exist build\obj mkdir build\obj
if not exist build\bin mkdir build\bin

echo Step 1: Assembling fp_core_3dmath_f32.asm...
nasm -f win64 src/asm/3d_math_kernels.asm -o build/obj/3d_math_kernels.o
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to assemble fp_core_3dmath_f32.asm
    pause
    exit /b 1
)
echo   fp_core_3dmath_f32.o assembled successfully!

echo.
echo Step 2: Compiling 3d_math_wrapper.c...
gcc -c src/algorithms/3d_math_wrapper.c -o build/obj/3d_math_wrapper.o -I include -O3 -march=native
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile 3d_math_wrapper.c
    pause
    exit /b 1
)
echo   3d_math_wrapper.o compiled successfully!

echo.
echo Step 3: Compiling test_3d_math_correctness.c...
gcc -c tests/test_3d_math_correctness.c -o build/obj/test_3d_math_correctness.o -I include -O3 -march=native
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile test_3d_math_correctness.c
    pause
    exit /b 1
)
echo   test_3d_math_correctness.o compiled successfully!

echo.
echo Step 4: Linking test executable...
gcc build/obj/test_3d_math_correctness.o build/obj/3d_math_kernels.o build/obj/3d_math_wrapper.o -o build/bin/test_3d_math_correctness.exe -I include -lm -lOpenCL
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to link test_3d_math_correctness.exe
    pause
    exit /b 1
)
echo   test_3d_math_correctness.exe built successfully!

echo.
echo Step 5: Running tests...
echo ================================================================
echo on
call .\build\bin\test_3d_math_correctness.exe
echo off

pause
