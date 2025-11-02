@echo off
echo ========================================
echo Building Moving Average Trading Demo
echo ========================================
echo.
echo Compiling demo_moving_avg.c with FP-ASM library...
gcc demo_moving_avg.c fp_core_reductions.o fp_core_scans.o -o moving_avg.exe
if exist moving_avg.exe (
    echo SUCCESS: moving_avg.exe created
    echo.
    echo ========================================
    echo Running Moving Average Benchmark
    echo ========================================
    echo.
    echo Usage: moving_avg.exe [n_prices] [iterations]
    echo Example: moving_avg.exe 5000000 50
    echo.
    echo Running with defaults (1M prices, 100 iterations)...
    echo.
    moving_avg.exe
) else (
    echo FAILED: Could not create moving_avg.exe
    echo Please check if Windows Defender or antivirus is blocking gcc
)
pause
