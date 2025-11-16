@echo off
echo === Forcing clean rebuild ===
echo Deleting old object files...
del build\obj\fp_percentile_wrappers.o 2>nul
del build\obj\fp_outliers_wrappers.o 2>nul
del test_outliers.exe 2>nul

echo.
echo === Step 1: Recompiling percentile wrapper from scratch ===
gcc -c src\wrappers\fp_percentile_wrappers.c -o build\obj\fp_percentile_wrappers.o -I include -O3 -march=native
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile wrapper
    pause
    exit /b 1
)
echo SUCCESS: wrapper recompiled

echo.
echo === Step 2: Building test ===
gcc test_zscore_detailed.c build\obj\fp_percentile_wrappers.o build\obj\fp_core_descriptive_stats.o build\obj\fp_core_percentiles.o build\obj\fp_core_reductions.o build\obj\fp_core_fused_folds.o -o test_quick.exe -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile test
    pause
    exit /b 1
)

echo.
echo === Step 3: Running quick test ===
test_quick.exe

echo.
echo === Step 4: Running full test suite ===
call build_test_outliers.bat

pause
