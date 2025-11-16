@echo off
echo ================================================================
echo   Building Naive Bayes Classifier Demo
echo   Showcasing FP-ASM Probabilistic Classification
echo ================================================================
echo.

echo Step 1: Compiling naive bayes algorithm...
gcc -c src/algorithms/fp_naive_bayes.c -o build/obj/fp_naive_bayes.o -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile fp_naive_bayes.c
    pause
    exit /b 1
)
echo   fp_naive_bayes.o ✓
echo.

echo Step 2: Building demo executable...
gcc demo_naive_bayes.c build/obj/fp_naive_bayes.o -o naive_bayes_demo.exe -I include -lm
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build naive_bayes_demo.exe
    pause
    exit /b 1
)
echo   naive_bayes_demo.exe ✓
echo.

echo Step 3: Running naive bayes demo...
echo ================================================================
naive_bayes_demo.exe

pause
