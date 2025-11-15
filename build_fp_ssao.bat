@echo off
REM Build FP-First SSAO Demo
REM Links with FP library for fp_reduce_add/min/max functions

echo ========================================
echo Building FP-First SSAO Demo
echo ========================================

echo.
echo [1/2] Compiling demo_fp_ssao.c...
gcc demo_fp_ssao.c ^
    build\obj\fp_core_reductions_f32.o ^
    -o fp_ssao_demo.exe ^
    -I include ^
    -lm ^
    -O2 ^
    -Wall

if errorlevel 1 (
    echo.
    echo ERROR: Compilation failed!
    exit /b 1
)

echo   ✓ Compilation successful

echo.
echo [2/2] Checking output...
if exist fp_ssao_demo.exe (
    dir fp_ssao_demo.exe | find "fp_ssao_demo.exe"
    echo   ✓ Executable created
    echo.
    echo ========================================
    echo Build Complete!
    echo ========================================
    echo.
    echo Run: fp_ssao_demo.exe
    echo Output: ssao_output.png
) else (
    echo   ERROR: Executable not created!
    exit /b 1
)
