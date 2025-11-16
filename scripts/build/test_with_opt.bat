@echo off
echo === Compiling WITH optimizations (after fix) ===
del build\obj\fp_percentile_wrappers.o 2>nul
gcc -c src\wrappers\fp_percentile_wrappers.c -o build\obj\fp_percentile_wrappers.o -I include -O3 -march=native
if %ERRORLEVEL% NEQ 0 (
    echo FAILED
    pause
    exit /b 1
)
echo SUCCESS

echo.
echo === Building test ===
gcc test_zscore_detailed.c build\obj\fp_percentile_wrappers.o build\obj\fp_core_descriptive_stats.o build\obj\fp_core_percentiles.o build\obj\fp_core_reductions.o build\obj\fp_core_fused_folds.o -o test_with_opt.exe -I include

echo.
echo === Running test (with O3 optimizations) ===
test_with_opt.exe

echo.
echo === If test passes, running full test suite ===
pause
build_test_outliers.bat
