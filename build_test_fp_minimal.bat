@echo off
echo Testing FP Library Minimal...
echo.

gcc test_fp_lib_minimal.c ^
    build\obj\fp_core_fused_folds_f32.o ^
    -o test_fp_minimal.exe ^
    -I include ^
    -Wall

if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build successful, running test...
echo.

test_fp_minimal.exe

echo.
pause
