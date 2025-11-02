@echo off
echo Building Algorithm #2: Percentile Calculations
echo.

echo Step 1: Assembling fp_core_percentiles.asm...
nasm -f win64 fp_core_percentiles.asm -o fp_core_percentiles.o
if errorlevel 1 (
    echo ERROR: Assembly failed!
    exit /b 1
)
echo OK

echo.
echo Step 2: Compiling demo_percentiles.c...
echo (Linking with fp_core_percentiles.o and fp_core_tier2.o for sorting)
gcc demo_percentiles.c fp_core_percentiles.o fp_core_tier2.o -o percentiles.exe
if errorlevel 1 (
    echo ERROR: Compilation failed!
    exit /b 1
)
echo OK

echo.
echo Build successful! Run with: percentiles.exe
