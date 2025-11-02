@echo off
echo ========================================
echo Building FP Trinity Test
echo ========================================
echo.
echo Compiling demo_fp_trinity.c with FP-ASM library...
gcc demo_fp_trinity.c fp_core_fused_maps.o fp_core_reductions.o fp_core_fused_folds.o fp_core_scans.o -o fp_trinity.exe
if exist fp_trinity.exe (
    echo SUCCESS: fp_trinity.exe created
    echo.
    echo ========================================
    echo Running FP Trinity Test
    echo ========================================
    echo.
    echo This tests Map, ZipWith, Fold, Scan
    echo.
    fp_trinity.exe
) else (
    echo FAILED: Could not create fp_trinity.exe
    echo Please check if Windows Defender or antivirus is blocking gcc
)
pause
