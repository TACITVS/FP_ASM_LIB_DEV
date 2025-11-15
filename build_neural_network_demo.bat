@echo off
echo ================================================================
echo   Building Neural Network Demo
echo   Showcasing FP-ASM Backpropagation and Deep Learning
echo ================================================================
echo.

echo Step 1: Compiling neural network algorithm...
gcc -c src/algorithms/fp_neural_network.c -o build/obj/fp_neural_network.o -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile fp_neural_network.c
    pause
    exit /b 1
)
echo   fp_neural_network.o ✓
echo.

echo Step 2: Building demo executable...
gcc demo_neural_network.c build/obj/fp_neural_network.o -o neural_network_demo.exe -I include -lm
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build neural_network_demo.exe
    pause
    exit /b 1
)
echo   neural_network_demo.exe ✓
echo.

echo Step 3: Running Neural Network demo...
echo ================================================================
neural_network_demo.exe

pause
