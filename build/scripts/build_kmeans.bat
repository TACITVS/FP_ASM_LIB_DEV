@echo off
echo ========================================
echo Building K-Means Clustering Demo
echo ========================================
echo.
echo Compiling demo_kmeans.c with FP-ASM library...
gcc demo_kmeans.c fp_core_reductions.o fp_core_fused_folds.o fp_core_fused_maps.o -o kmeans.exe -lm
if exist kmeans.exe (
    echo SUCCESS: kmeans.exe created
    echo.
    echo ========================================
    echo Running K-Means Benchmark
    echo ========================================
    echo.
    echo Usage: kmeans.exe [n_points] [k_clusters] [dimensions] [iterations]
    echo Example: kmeans.exe 50000 5 16 20
    echo.
    echo Running with defaults (10000 points, 5 clusters, 16D, 50 iterations)...
    echo.
    kmeans.exe
) else (
    echo FAILED: Could not create kmeans.exe
    echo Please check if Windows Defender or antivirus is blocking gcc
)
pause
