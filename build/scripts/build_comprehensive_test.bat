@echo off
echo ========================================
echo Building Comprehensive Test Suite
echo ========================================
echo.
echo Compiling test_comprehensive.c with all modules...
gcc test_comprehensive.c fp_core_reductions.o fp_core_fused_folds.o fp_core_fused_maps.o fp_core_simple_maps.o fp_core_scans.o fp_core_predicates.o -o test_comprehensive.exe
if exist test_comprehensive.exe (
    echo SUCCESS: test_comprehensive.exe created
    echo.
    echo ========================================
    echo Running Comprehensive Tests
    echo ========================================
    echo.
    test_comprehensive.exe
    echo.
    echo ========================================
    echo Test run completed
    echo ========================================
) else (
    echo FAILED: Could not create test_comprehensive.exe
    echo Please check if Windows Defender or antivirus is blocking gcc
)
pause
