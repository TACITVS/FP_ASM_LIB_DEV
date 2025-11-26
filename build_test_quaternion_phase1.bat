@echo off
echo ========================================
echo Building test_quaternion_phase1.exe
echo ========================================
echo.

echo Step 1: Recompiling fp_quaternion_ops.c...
gcc -c src\algorithms\fp_quaternion_ops.c -o build\obj\fp_quaternion_ops.o -I include -O3 -march=native 2>&1
if errorlevel 1 (
    echo.
    echo ========================================
    echo FAILED to compile fp_quaternion_ops.c
    echo ========================================
    exit /b 1
)
echo   fp_quaternion_ops.o compiled successfully!

echo.
echo Step 2: Linking test executable...
gcc tests\unit\test_quaternion_phase1.c ^
    build\obj\fp_quaternion_ops.o ^
    build\obj\fp_vector_ops.o ^
    build\obj\fp_matrix_ops.o ^
    build\obj\fp_core_matrix.o ^
    build\obj\3d_math_kernels.o ^
    -o tests\unit\test_quaternion_phase1.exe ^
    -I include -O3 -march=native -Wall -Wextra -lm 2>&1

if errorlevel 1 (
    echo.
    echo ========================================
    echo COMPILATION FAILED
    echo ========================================
    exit /b 1
)
echo   test_quaternion_phase1.exe built successfully!

echo.
echo ========================================
echo Build successful! Running tests...
echo ========================================
echo.

tests\unit\test_quaternion_phase1.exe

echo.
echo ========================================
echo Test run complete
echo ========================================
