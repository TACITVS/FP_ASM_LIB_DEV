@echo off
echo Building Algorithm #5: Outlier Detection
echo.

echo Step 1: Assembling fp_core_outliers.asm...
nasm -f win64 fp_core_outliers.asm -o fp_core_outliers.o
if errorlevel 1 (
    echo ERROR: Assembly failed!
    exit /b 1
)
echo OK

echo.
echo Step 2: Compiling demo_outliers.c...
echo (Linking with fp_core_outliers.o and fp_core_percentiles.o for IQR method)
gcc demo_outliers.c fp_core_outliers.o fp_core_percentiles.o -o outliers.exe
if errorlevel 1 (
    echo ERROR: Compilation failed!
    exit /b 1
)
echo OK

echo.
echo Build successful! Run with: outliers.exe
