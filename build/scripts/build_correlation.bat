@echo off
echo Building Algorithm #3: Correlation and Covariance
echo.

echo Step 1: Assembling fp_core_correlation.asm...
nasm -f win64 fp_core_correlation.asm -o fp_core_correlation.o
if errorlevel 1 (
    echo ERROR: Assembly failed!
    exit /b 1
)
echo OK

echo.
echo Step 2: Compiling demo_correlation.c...
gcc demo_correlation.c fp_core_correlation.o -o correlation.exe
if errorlevel 1 (
    echo ERROR: Compilation failed!
    exit /b 1
)
echo OK

echo.
echo Build successful! Run with: correlation.exe
