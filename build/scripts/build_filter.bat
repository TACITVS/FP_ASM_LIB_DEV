@echo off
echo ========================================
echo Building Filter FP Fitness Demo
echo ========================================
echo.
echo Compiling demo_filter.c with FP-ASM library...
gcc demo_filter.c fp_core_reductions.o -o filter.exe 2^>^&1
if exist filter.exe (
    echo SUCCESS: filter.exe created
    echo.
    echo ========================================
    echo Running Filter FP Benchmark
    echo ========================================
    echo.
    echo Usage: filter.exe [n_elements] [iterations]
    echo Example: filter.exe 20000000 30
    echo.
    echo Running with defaults (10M elements, 50 iterations)...
    echo.
    filter.exe
) else (
    echo FAILED: Could not create filter.exe
    echo Please check if Windows Defender or antivirus is blocking gcc
)
pause
