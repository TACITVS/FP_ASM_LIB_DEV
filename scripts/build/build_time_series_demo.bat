@echo off
echo ================================================================
echo   Building Time Series Forecasting Demo
echo   Showcasing FP-ASM Statistical Prediction
echo ================================================================
echo.

echo Step 1: Compiling time series algorithm...
gcc -c src/algorithms/fp_time_series.c -o build/obj/fp_time_series.o -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile fp_time_series.c
    pause
    exit /b 1
)
echo   fp_time_series.o ✓
echo.

echo Step 2: Building demo executable...
gcc demo_time_series.c build/obj/fp_time_series.o -o time_series_demo.exe -I include -lm
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build time_series_demo.exe
    pause
    exit /b 1
)
echo   time_series_demo.exe ✓
echo.

echo Step 3: Running time series demo...
echo ================================================================
time_series_demo.exe

pause
