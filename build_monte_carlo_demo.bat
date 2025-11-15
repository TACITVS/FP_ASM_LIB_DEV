@echo off
echo ================================================================
echo   Building Monte Carlo Simulation Demo
echo   Showcasing FP-ASM Probabilistic Computation
echo ================================================================
echo.

echo Step 1: Compiling Monte Carlo algorithm...
gcc -c src/algorithms/fp_monte_carlo.c -o build/obj/fp_monte_carlo.o -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile fp_monte_carlo.c
    pause
    exit /b 1
)
echo   fp_monte_carlo.o ✓
echo.

echo Step 2: Building demo executable...
gcc demo_monte_carlo.c build/obj/fp_monte_carlo.o -o monte_carlo_demo.exe -I include -lm
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build monte_carlo_demo.exe
    pause
    exit /b 1
)
echo   monte_carlo_demo.exe ✓
echo.

echo Step 3: Running Monte Carlo demo...
echo ================================================================
monte_carlo_demo.exe

pause
