@echo off
echo ========================================
echo Building test_gpu_math.exe
echo ========================================

gcc tests\unit\test_gpu_math.c ^
    src\algorithms\fp_gpu_math.c ^
    -I include -O2 -Wall -Wextra ^
    -o tests\unit\test_gpu_math.exe

if errorlevel 1 (
    echo.
    echo ========================================
    echo COMPILATION FAILED
    echo ========================================
    exit /b 1
)

echo.
echo ========================================
echo Build successful! Running tests...
echo ========================================
echo.

tests\unit\test_gpu_math.exe

echo.
echo ========================================
echo Test run complete
echo ========================================
