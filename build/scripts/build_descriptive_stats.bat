@echo off
echo Building Algorithm #1: Descriptive Statistics Suite
echo.

echo Step 1: Assembling fp_core_descriptive_stats.asm...
nasm -f win64 fp_core_descriptive_stats.asm -o fp_core_descriptive_stats.o
if errorlevel 1 (
    echo ERROR: Assembly failed!
    exit /b 1
)
echo OK

echo.
echo Step 2: Compiling demo_descriptive_stats.c...
gcc demo_descriptive_stats.c fp_core_descriptive_stats.o -o descriptive_stats.exe -lm
if errorlevel 1 (
    echo ERROR: Compilation failed!
    exit /b 1
)
echo OK

echo.
echo Build successful! Run with: descriptive_stats.exe
