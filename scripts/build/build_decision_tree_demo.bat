@echo off
echo ================================================================
echo   Building Decision Tree Demo
echo   Showcasing FP-ASM Interpretable Machine Learning
echo ================================================================
echo.

echo Step 1: Compiling decision tree algorithm...
gcc -c src/algorithms/fp_decision_tree.c -o build/obj/fp_decision_tree.o -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile fp_decision_tree.c
    pause
    exit /b 1
)
echo   fp_decision_tree.o ✓
echo.

echo Step 2: Building demo executable...
gcc demo_decision_tree.c build/obj/fp_decision_tree.o -o decision_tree_demo.exe -I include -lm
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build decision_tree_demo.exe
    pause
    exit /b 1
)
echo   decision_tree_demo.exe ✓
echo.

echo Step 3: Running decision tree demo...
echo ================================================================
decision_tree_demo.exe

pause
