@echo off
echo ================================================================================
echo Building Pure FP (v3) Tests
echo ================================================================================
echo.

gcc tests\test_fp_stats_v3_pure.c build\obj\fp_core_reductions.o -I.\include -o test_v3_pure.exe -lm -O2 > v3_compile_log.txt 2>&1
if errorlevel 1 (
    echo COMPILATION FAILED! Error output saved to v3_compile_log.txt
    type v3_compile_log.txt
    pause
    exit /b 1
)

echo.
echo ================================================================================
echo Build successful! Running tests...
echo ================================================================================
echo.

test_v3_pure.exe
if errorlevel 1 (
    echo TESTS FAILED!
    pause
    exit /b 1
)

echo.
echo ================================================================================
echo ALL TESTS PASSED!
echo ================================================================================
pause
