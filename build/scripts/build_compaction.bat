@echo off
echo ========================================
echo Building Stream Compaction Module
echo ========================================
echo.
echo Assembling fp_core_compaction.asm...
nasm -f win64 fp_core_compaction.asm -o fp_core_compaction.o

if exist fp_core_compaction.o (
    echo SUCCESS: fp_core_compaction.o created
    echo.
    echo ========================================
    echo Building Filter Demo with REAL SIMD
    echo ========================================
    echo.
    gcc demo_filter.c fp_core_reductions.o fp_core_compaction.o -o filter_simd.exe

    if exist filter_simd.exe (
        echo SUCCESS: filter_simd.exe created
        echo.
        echo ========================================
        echo Testing "List FP" Fitness
        echo ========================================
        echo.
        echo This will determine if the library can achieve
        echo "List FP" status with real SIMD compaction!
        echo.
        filter_simd.exe
    ) else (
        echo FAILED: Could not create filter_simd.exe
    )
) else (
    echo FAILED: Could not assemble fp_core_compaction.o
    echo Check NASM installation
)
pause
