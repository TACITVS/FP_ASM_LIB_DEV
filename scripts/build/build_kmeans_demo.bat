@echo off
echo ================================================================
echo   Building K-Means Clustering Demo
echo   Showcasing FP-ASM Library Power
echo ================================================================
echo.

echo Step 1: Compiling k-means algorithm...
gcc -c src/algorithms/fp_kmeans.c -o build/obj/fp_kmeans.o -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile fp_kmeans.c
    pause
    exit /b 1
)
echo   fp_kmeans.o ✓
echo.

echo Step 2: Building demo executable...
gcc demo_kmeans.c build/obj/fp_kmeans.o -o kmeans_demo.exe -I include -lm
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build kmeans_demo.exe
    pause
    exit /b 1
)
echo   kmeans_demo.exe ✓
echo.

echo Step 3: Running K-Means demo...
echo ================================================================
kmeans_demo.exe

pause
