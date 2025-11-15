@echo off
echo ==============================================================
echo   Building Matrix Operations Module
echo ==============================================================
echo.

echo Step 1: Compiling matrix operations (C implementation)...
cmd /c "gcc -c src/algorithms/fp_matrix_ops.c -o build/obj/fp_matrix_ops.o -I include 2>&1"
if %ERRORLEVEL% NEQ 0 (
    echo *** COMPILATION OF MATRIX OPS FAILED ***
    pause
    exit /b 1
)
echo   Matrix ops compiled successfully!
echo.

echo Step 2: Compiling test file and linking...
cmd /c "gcc test_matrix_ops.c build/obj/fp_matrix_ops.o build/obj/fp_core_matrix.o -o test_matrix_ops.exe -I include -lm 2>&1"
if %ERRORLEVEL% NEQ 0 (
    echo *** TEST BUILD FAILED ***
    pause
    exit /b 1
)
echo   Test built successfully!
echo.

echo Step 3: Running comprehensive test suite...
echo ==============================================================
test_matrix_ops.exe

pause
