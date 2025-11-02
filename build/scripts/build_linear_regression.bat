@echo off
echo Building Algorithm #4: Linear Regression
echo.

echo Step 1: Assembling fp_core_linear_regression.asm...
nasm -f win64 fp_core_linear_regression.asm -o fp_core_linear_regression.o
if errorlevel 1 (
    echo ERROR: Assembly failed!
    exit /b 1
)
echo OK

echo.
echo Step 2: Compiling demo_linear_regression.c...
gcc demo_linear_regression.c fp_core_linear_regression.o -o linear_regression.exe
if errorlevel 1 (
    echo ERROR: Compilation failed!
    exit /b 1
)
echo OK

echo.
echo Build successful! Run with: linear_regression.exe
