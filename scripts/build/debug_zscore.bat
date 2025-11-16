@echo off
echo === Compiling detailed debug program ===
gcc test_zscore_detailed.c build\obj\fp_percentile_wrappers.o build\obj\fp_core_descriptive_stats.o build\obj\fp_core_percentiles.o build\obj\fp_core_reductions.o build\obj\fp_core_fused_folds.o -o test_zscore_detailed.exe -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile
    exit /b 1
)

echo.
echo === Running detailed debug ===
test_zscore_detailed.exe

echo.
pause
