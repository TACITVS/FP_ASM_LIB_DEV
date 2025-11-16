@echo off
echo ================================================================
echo   Building Performance Comparison Benchmark
echo   Generic vs. Specialized Functions
echo ================================================================
echo.

echo Compiling benchmark...
gcc bench_generic_vs_specialized.c build/obj/fp_generic.o build/obj/fp_core_reductions.o -o bench_generic_vs_specialized.exe -I include

if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build benchmark
    pause
    exit /b 1
)

echo Build successful!
echo.
echo Running benchmark...
echo ================================================================
bench_generic_vs_specialized.exe

pause
