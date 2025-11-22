@echo off
echo ================================================================================
echo Building and Testing Neural Networks (Pattern 1 Refactored)
echo ================================================================================
echo.

echo Step 1: Compiling fp_neural_network.c...
gcc -c src\algorithms\fp_neural_network.c -o build\obj\fp_neural_network.o -I include > nn_build_log.txt 2>&1
if errorlevel 1 (
    echo COMPILATION FAILED! Error output:
    type nn_build_log.txt
    pause
    exit /b 1
)
echo   fp_neural_network.o compiled successfully!
echo.

echo Step 2: Building Neural Network demo...
gcc examples\algorithms\demo_neural_network.c build\obj\fp_neural_network.o build\obj\fp_core_fused_folds.o build\obj\fp_core_fused_maps.o -o neural_network_demo.exe -I include -lm > nn_link_log.txt 2>&1
if errorlevel 1 (
    echo LINKING FAILED! Error output:
    type nn_link_log.txt
    pause
    exit /b 1
)
echo   neural_network_demo.exe built successfully!
echo.

echo Step 3: Running Neural Network demo...
echo ================================================================================
neural_network_demo.exe
if errorlevel 1 (
    echo TESTS FAILED!
    pause
    exit /b 1
)

echo.
echo ================================================================================
echo NEURAL NETWORKS PATTERN 1 REFACTORING: ALL TESTS PASSED!
echo ================================================================================
pause
