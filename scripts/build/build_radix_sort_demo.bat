@echo off
echo ================================================================
echo   Building Radix Sort Demo
echo   Showcasing FP-ASM Integer Type Support
echo ================================================================
echo.

echo Step 1: Compiling radix sort algorithm...
gcc -c src/algorithms/fp_radix_sort.c -o build/obj/fp_radix_sort.o -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile fp_radix_sort.c
    pause
    exit /b 1
)
echo   fp_radix_sort.o ✓
echo.

echo Step 2: Building demo executable...
gcc demo_radix_sort.c build/obj/fp_radix_sort.o -o radix_sort_demo.exe -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build radix_sort_demo.exe
    pause
    exit /b 1
)
echo   radix_sort_demo.exe ✓
echo.

echo Step 3: Running Radix Sort demo...
echo ================================================================
radix_sort_demo.exe

pause
