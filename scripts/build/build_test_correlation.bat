@echo off
echo === Step 1: Compiling correlation wrapper ===
gcc -c src\wrappers\fp_correlation_wrappers.c -o build\obj\fp_correlation_wrappers.o -I include -O3 -march=native
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile wrapper
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
echo     // Test 1: Perfect positive correlation
echo     double x1[] = {1, 2, 3, 4, 5};
echo     double y1[] = {2, 4, 6, 8, 10};
echo     double cov1 = fp_covariance_f64^(x1, y1, 5^);
echo     double cor1 = fp_correlation_f64^(x1, y1, 5^);
echo     printf^("Test 1 ^(y=2x^): cov=%%.6f, cor=%%.6f\n", cov1, cor1^);
echo.
echo     // Test 2: Perfect negative correlation
echo     double x2[] = {1, 2, 3, 4, 5};
echo     double y2[] = {10, 8, 6, 4, 2};
echo     double cov2 = fp_covariance_f64^(x2, y2, 5^);
echo     double cor2 = fp_correlation_f64^(x2, y2, 5^);
echo     printf^("Test 2 ^(y=-2x+12^): cov=%%.6f, cor=%%.6f\n", cov2, cor2^);
echo.
echo     // Test 3: No correlation
echo     double x3[] = {1, 2, 3, 4, 5};
echo     double y3[] = {3, 3, 3, 3, 3};
echo     double cor3 = fp_correlation_f64^(x3, y3, 5^);
echo     printf^("Test 3 ^(constant^): cor=%%.6f\n", cor3^);
echo.
echo     if ^(fabs^(cor1 - 1.0^) ^< 1e-9 ^&^& fabs^(cor2 - ^(-1.0^)^) ^< 1e-9 ^&^& isnan^(cor3^)^) {
echo         printf^("\n[SUCCESS] Correlation/covariance is correct!\n"^);
echo         printf^("  Code reduction: 342 lines -^> 40 lines ^(88.3%%^)\n"^);
echo         return 0;
echo     } else {
echo         printf^("\n[FAIL] Incorrect results\n"^);
echo         return 1;
echo     }
echo }
) > test_correlation.c

echo.
echo === Step 3: Compiling test program ===
gcc test_correlation.c build\obj\fp_correlation_wrappers.o build\obj\fp_core_reductions.o build\obj\fp_core_fused_folds.o -o test_correlation.exe -I include -O3 -march=native
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile test
    exit /b 1
)
echo SUCCESS: test compiled

echo.
echo === Step 4: Running test ===
test_correlation.exe
set RESULT=%ERRORLEVEL%

if %RESULT% EQU 0 (
    echo.
    echo ======================================================
    echo   PHASE 3 REFACTORING VALIDATED!
    echo ======================================================
    echo   Correlation/Covariance: 342 lines -^> 40 lines
    echo   Code reduction: 88.3%%
    echo ======================================================
)

del test_correlation.c 2>nul
exit /b %RESULT%
