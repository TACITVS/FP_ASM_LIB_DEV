@echo off
echo === Deleting old files ===
del build\obj\fp_percentile_wrappers.o 2>nul
del test_quick.exe 2>nul

echo.
echo === Recompiling wrapper ===
gcc -c src\wrappers\fp_percentile_wrappers.c -o build\obj\fp_percentile_wrappers.o -I include -O3 -march=native
if %ERRORLEVEL% NEQ 0 (
    echo FAILED
    pause
    exit /b 1
)

echo.
echo === Building test ===
gcc test_zscore_detailed.c build\obj\fp_percentile_wrappers.o build\obj\fp_core_descriptive_stats.o build\obj\fp_core_percentiles.o build\obj\fp_core_reductions.o build\obj\fp_core_fused_folds.o -o test_quick.exe -I include

echo.
echo === Running test ===
test_quick.exe

pause
