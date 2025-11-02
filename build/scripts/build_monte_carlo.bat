@echo off
echo ========================================
echo Building Monte Carlo Options Pricing Demo
echo ========================================
echo.
echo Compiling demo_monte_carlo.c with FP-ASM library...
gcc demo_monte_carlo.c fp_core_reductions.o fp_core_fused_maps.o -o monte_carlo.exe -lm
if exist monte_carlo.exe (
    echo SUCCESS: monte_carlo.exe created
    echo.
    echo ========================================
    echo Running Monte Carlo Benchmark
    echo ========================================
    echo.
    echo Usage: monte_carlo.exe [n_paths] [n_steps] [iterations]
    echo Example: monte_carlo.exe 5000000 252 5
    echo.
    echo Running with defaults (1M paths, 252 steps, 10 iterations)...
    echo.
    monte_carlo.exe
) else (
    echo FAILED: Could not create monte_carlo.exe
    echo Please check if Windows Defender or antivirus is blocking gcc
)
pause
