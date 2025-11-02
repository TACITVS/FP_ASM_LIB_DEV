@echo off
echo === Compiling WITHOUT optimizations ===
del build\obj\fp_percentile_wrappers.o 2>nul
gcc -c src\wrappers\fp_percentile_wrappers.c -o build\obj\fp_percentile_wrappers.o -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED
    pause
    exit /b 1
)
echo SUCCESS

echo.
echo === Building test ===
gcc test_zscore_detailed.c build\obj\fp_percentile_wrappers.o build\obj\fp_core_descriptive_stats.o build\obj\fp_core_percentiles.o build\obj\fp_core_reductions.o build\obj\fp_core_fused_folds.o -o test_no_opt.exe -I include

echo.
echo === Running test (no optimizations) ===
test_no_opt.exe

pause
