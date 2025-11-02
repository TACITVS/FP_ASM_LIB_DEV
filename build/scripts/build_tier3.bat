@echo off
echo ========================================
echo Building TIER 3 Test Suite
echo ========================================

echo.
echo [1/2] Compiling demo_tier3.c with fp_core_tier3.o...
gcc demo_tier3.c fp_core_tier3.o -o tier3.exe -v 2>&1

if errorlevel 1 (
    echo.
    echo ERROR: Compilation failed!
    exit /b 1
)

echo.
echo [2/2] Build successful!
echo.
echo Executable: tier3.exe
echo.
echo Run with: tier3.exe
echo ========================================
