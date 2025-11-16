@echo off
echo ================================================================
echo   Building FP Graphics Transforms Demo
echo ================================================================
echo.

echo Step 1: Assembling fp_core_matrix.asm...
nasm -f win64 src/asm/fp_core_matrix.asm -o build/obj/fp_core_matrix.o
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to assemble fp_core_matrix.asm
    pause
    exit /b 1
)
echo   fp_core_matrix.o ✓

echo.
echo Step 2: Compiling C source files...
gcc -c src/algorithms/fp_matrix_ops.c -o build/obj/fp_matrix_ops.o -I include -O3 -march=native -Wall
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile fp_matrix_ops.c
    pause
    exit /b 1
)
echo   fp_matrix_ops.o ✓

gcc -c src/engine/fp_transforms.c -o build/obj/fp_transforms.o -I include -O3 -march=native -Wall
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile fp_transforms.c
    pause
    exit /b 1
)
echo   fp_transforms.o ✓

echo.
echo Step 3: Building demo executable...
gcc demo_fp_transforms.c build/obj/fp_transforms.o build/obj/fp_core_matrix.o build/obj/fp_matrix_ops.o -o fp_transforms.exe -I include -lm
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build demo executable
    pause
    exit /b 1
)
echo   fp_transforms.exe ✓

echo.
echo Step 4: Running demo...
echo ================================================================
fp_transforms.exe
echo ================================================================

echo.
echo Build and run complete.
pause