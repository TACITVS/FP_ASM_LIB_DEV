@echo off
echo ========================================================================
echo Building Refactoring Performance Benchmarks
echo ========================================================================
echo.

echo [1/2] Compiling benchmark program...
gcc bench_refactoring.c build\obj\fp_regression_wrappers.o build\obj\fp_correlation_wrappers.o build\obj\fp_core_reductions.o build\obj\fp_core_fused_folds.o -o bench_refactoring.exe -I include -O3 -march=native

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed
    exit /b 1
)

echo SUCCESS: bench_refactoring.exe created
echo.

echo [2/2] Running benchmarks...
echo ========================================================================
echo.
bench_refactoring.exe 100000 100
set RESULT=%ERRORLEVEL%

exit /b %RESULT%
