@echo off
echo Building Algorithm #6: Moving Averages
echo.

echo Step 1: Assembling fp_core_moving_averages.asm...
nasm -f win64 fp_core_moving_averages.asm -o fp_core_moving_averages.o
if errorlevel 1 (
    echo ERROR: Assembly failed!
    exit /b 1
)
echo OK

echo.
echo Step 2: Compiling demo_moving_averages.c...
gcc demo_moving_averages.c fp_core_moving_averages.o -o moving_averages.exe
if errorlevel 1 (
    echo ERROR: Compilation failed!
    exit /b 1
)
echo OK

echo.
echo Build successful! Run with: moving_averages.exe
