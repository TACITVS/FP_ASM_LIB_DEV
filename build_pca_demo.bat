@echo off
echo ================================================================
echo   Building Principal Component Analysis (PCA) Demo
echo   Showcasing FP-ASM Dimensionality Reduction
echo ================================================================
echo.

echo Step 1: Compiling PCA algorithm...
gcc -c src/algorithms/fp_pca.c -o build/obj/fp_pca.o -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile fp_pca.c
    pause
    exit /b 1
)
echo   fp_pca.o ✓
echo.

echo Step 2: Building demo executable...
gcc demo_pca.c build/obj/fp_pca.o -o pca_demo.exe -I include -lm
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build pca_demo.exe
    pause
    exit /b 1
)
echo   pca_demo.exe ✓
echo.

echo Step 3: Running PCA demo...
echo ================================================================
pca_demo.exe

pause
