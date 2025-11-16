@echo off
echo === Compiling correlation wrapper ===
gcc -c src\wrappers\fp_correlation_wrappers.c -o build\obj\fp_correlation_wrappers.o -I include -O3 -march=native
if %ERRORLEVEL% NEQ 0 (
    echo FAILED
    exit /b 1
)
echo SUCCESS

echo.
echo === Compiling test ===
gcc test_corr_standalone.c build\obj\fp_core_reductions.o build\obj\fp_core_fused_folds.o -o test_corr.exe -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED
    exit /b 1
)
echo SUCCESS

echo.
echo === Running test ===
test_corr.exe
exit /b %ERRORLEVEL%
