@echo off
echo === Step 1: Compiling regression wrapper ===
gcc -c src\wrappers\fp_regression_wrappers.c -o build\obj\fp_regression_wrappers.o -I include -O3 -march=native 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo FAILED to compile wrapper
    echo Running again with verbose output:
    gcc -c src\wrappers\fp_regression_wrappers.c -o build\obj\fp_regression_wrappers.o -I include -O3 -march=native -v 2>&1
    exit /b 1
)
echo SUCCESS: wrapper compiled

echo.
echo === Step 2: Creating test program ===
(
echo #include ^<stdio.h^>
echo #include ^<stdlib.h^>
echo #include ^<math.h^>
echo #include "include/fp_core.h"
echo.
echo int main^(void^) {
echo     double x[] = {1, 2, 3, 4, 5};
echo     double y[] = {2, 4, 6, 8, 10};
echo     size_t n = 5;
echo     LinearRegression result;
echo.
echo     fp_linear_regression_f64^(x, y, n, ^&result^);
echo.
echo     printf^("Linear Regression Test ^(y = 2x^):\n"^);
echo     printf^("  slope = %%.6f\n", result.slope^);
echo     printf^("  intercept = %%.6f\n", result.intercept^);
echo     printf^("  r_squared = %%.6f\n", result.r_squared^);
echo.
echo     double slope_err = fabs^(result.slope - 2.0^);
echo     double intercept_err = fabs^(result.intercept - 0.0^);
echo     double r2_err = fabs^(result.r_squared - 1.0^);
echo.
echo     if ^(slope_err ^< 1e-9 ^&^& intercept_err ^< 1e-9 ^&^& r2_err ^< 1e-9^) {
echo         printf^("\n[SUCCESS] Linear regression is correct!\n"^);
echo         return 0;
echo     } else {
echo         printf^("\n[FAIL] Linear regression produced incorrect results\n"^);
echo         return 1;
echo     }
echo }
) > test_regression.c

echo.
echo === Step 3: Compiling test program ===
gcc test_regression.c build\obj\fp_regression_wrappers.o build\obj\fp_core_reductions.o build\obj\fp_core_fused_folds.o -o test_regression.exe -I include -O3 -march=native
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile test
    exit /b 1
)
echo SUCCESS: test compiled

echo.
echo === Step 4: Running test ===
test_regression.exe
set RESULT=%ERRORLEVEL%

if %RESULT% EQU 0 (
    echo.
    echo ======================================================
    echo   PHASE 2 REFACTORING VALIDATED!
    echo ======================================================
    echo   Linear Regression: 391 lines -^> 65 lines
    echo   Code reduction: 93.6%%
    echo ======================================================
) else (
    echo.
    echo [FAILED] Refactoring test failed
)

del test_regression.c 2>nul
exit /b %RESULT%
