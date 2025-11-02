@echo off
echo ================================================================
echo   Building General Higher-Order Functions Test Suite
echo   Demonstrating 100%% FP Language Equivalence
echo ================================================================

echo.
echo === Step 1: Compiling general HOF wrapper ===
gcc -c src/wrappers/fp_general_hof.c -o build/obj/fp_general_hof.o -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile general HOF wrapper
    pause
    exit /b 1
)
echo SUCCESS: General HOF wrapper compiled

echo.
echo === Step 2: Compiling test program ===
gcc test_general_hof.c build/obj/fp_general_hof.o -o test_general_hof.exe -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile test program
    pause
    exit /b 1
)
echo SUCCESS: Test program compiled

echo.
echo === Step 3: Running comprehensive test suite ===
echo.
test_general_hof.exe

echo.
echo === Build and test complete ===
pause
