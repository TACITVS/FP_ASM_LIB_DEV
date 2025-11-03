@echo off
echo ================================================================
echo   Building FP-ASM Generic Type System Test Suite
echo ================================================================
echo.

echo Step 1: Compiling generic type system implementation...
gcc -c src/wrappers/fp_generic.c -o build/obj/fp_generic.o -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile fp_generic.c
    pause
    exit /b 1
)
echo   fp_generic.o compiled successfully!

echo.
echo Step 2: Building test executable...
gcc test_generic.c build/obj/fp_generic.o -o test_generic.exe -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build test_generic.exe
    pause
    exit /b 1
)
echo   test_generic.exe built successfully!

echo.
echo Step 3: Running tests...
echo ================================================================
test_generic.exe

echo.
echo ================================================================
echo   Build and test complete!
echo ================================================================
pause
