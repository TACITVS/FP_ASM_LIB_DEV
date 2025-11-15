@echo off
echo ================================================================
echo   Building Linear Regression + Gradient Descent Demo
echo   Showcasing FP-ASM ML Optimization
echo ================================================================
echo.

echo Step 1: Compiling linear regression algorithm...
gcc -c src/algorithms/fp_linear_regression.c -o build/obj/fp_linear_regression.o -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile fp_linear_regression.c
    pause
    exit /b 1
)
echo   fp_linear_regression.o ✓
echo.

echo Step 2: Building demo executable...
gcc demo_linear_regression.c build/obj/fp_linear_regression.o -o linear_regression_demo.exe -I include -lm
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build linear_regression_demo.exe
    pause
    exit /b 1
)
echo   linear_regression_demo.exe ✓
echo.

echo Step 3: Running Linear Regression demo...
echo ================================================================
linear_regression_demo.exe

pause
