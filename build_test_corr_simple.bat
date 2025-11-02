@echo off
echo === Compiling standalone correlation test ===
gcc test_corr_standalone.c build\obj\fp_core_reductions.o build\obj\fp_core_fused_folds.o -o test_corr.exe -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED
    exit /b 1
)
echo SUCCESS

echo.
echo === Running test ===
test_corr.exe
