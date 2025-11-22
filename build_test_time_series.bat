@echo off
echo ================================================================================
echo Building and Testing Time Series (Pattern 1 Refactored)
echo ================================================================================
echo.

echo Step 1: Compiling fp_time_series.c...
gcc -c src\algorithms\fp_time_series.c -o build\obj\fp_time_series.o -I include > ts_build_log.txt 2>&1
if errorlevel 1 (
    echo COMPILATION FAILED! Error output:
    type ts_build_log.txt
    pause
    exit /b 1
)
echo   fp_time_series.o compiled successfully!
echo.

echo Step 2: Building Time Series demo...
gcc examples\algorithms\demo_time_series.c build\obj\fp_time_series.o build\obj\fp_core_fused_folds.o build\obj\fp_core_fused_maps.o build\obj\fp_core_reductions.o -o time_series_demo.exe -I include -lm > ts_link_log.txt 2>&1
if errorlevel 1 (
    echo LINKING FAILED! Error output:
    type ts_link_log.txt
    pause
    exit /b 1
)
echo   time_series_demo.exe built successfully!
echo.

echo Step 3: Running Time Series demo...
echo ================================================================================
time_series_demo.exe
if errorlevel 1 (
    echo TESTS FAILED!
    pause
    exit /b 1
)

echo.
echo ================================================================================
echo TIME SERIES PATTERN 1 REFACTORING: ALL TESTS PASSED!
echo ================================================================================
pause
