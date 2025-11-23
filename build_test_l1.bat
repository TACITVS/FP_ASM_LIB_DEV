@echo off
setlocal enabledelayedexpansion
cd /d C:\Users\baian\C_CODE\fp_asm_lib_dev_working_copy

echo ============================================================
echo   FP-ASM L1 Complete Test - Build and Run
echo ============================================================
echo.

REM Check if tests directory exists
if not exist tests\test_l1_complete.c (
    echo ERROR: tests\test_l1_complete.c not found
    exit /b 1
)

REM Create bin directory if needed
if not exist build\bin mkdir build\bin

echo [1/2] Compiling...
gcc -o build\bin\test_l1_complete.exe ^
    tests\test_l1_complete.c ^
    src\wrappers\fp_compose.c ^
    src\wrappers\fp_monads.c ^
    src\wrappers\fp_general_hof.c ^
    build\obj\fp_core_reductions.o ^
    build\obj\fp_core_fused_folds.o ^
    build\obj\fp_core_fused_maps.o ^
    build\obj\fp_core_simple_maps.o ^
    -I include -Wall -O2 -lm

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [FAILED] Compilation failed with error code %ERRORLEVEL%
    exit /b %ERRORLEVEL%
)

echo [2/2] Running tests...
echo.
build\bin\test_l1_complete.exe

echo.
echo ============================================================
echo   Done
echo ============================================================
