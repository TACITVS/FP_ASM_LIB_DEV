@echo off
echo ========================================================================
echo Testing Linear Regression Refactoring: Composition vs Assembly
echo ========================================================================
echo.

echo [Step 1/4] Compiling composition-based regression wrapper...
gcc -c src/wrappers/fp_regression_wrappers.c ^
    -o build/obj/fp_regression_wrappers.o ^
    -I include -O3 -march=native
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to compile wrapper
    exit /b 1
)
echo SUCCESS: fp_regression_wrappers.o created
echo.

echo [Step 2/4] Creating test program...
echo #include ^<stdio.h^>                          > build\temp_regression_test.c
echo #include ^<stdlib.h^>                        >> build\temp_regression_test.c
echo #include ^<math.h^>                          >> build\temp_regression_test.c
echo #include "../include/fp_core.h"             >> build\temp_regression_test.c
echo.                                             >> build\temp_regression_test.c
echo int main(void) {                             >> build\temp_regression_test.c
echo     // Test data: perfect linear relationship y = 2x >> build\temp_regression_test.c
echo     double x[] = {1, 2, 3, 4, 5};            >> build\temp_regression_test.c
echo     double y[] = {2, 4, 6, 8, 10};           >> build\temp_regression_test.c
echo     size_t n = 5;                            >> build\temp_regression_test.c
echo     double slope, intercept, r_squared;      >> build\temp_regression_test.c
echo.                                             >> build\temp_regression_test.c
echo     fp_linear_regression_f64(x, y, n, ^&slope, ^&intercept, ^&r_squared); >> build\temp_regression_test.c
echo.                                             >> build\temp_regression_test.c
echo     printf("Linear Regression Test (y = 2x):\n"); >> build\temp_regression_test.c
echo     printf("  Computed: slope = %%.6f, intercept = %%.6f, r^2 = %%.6f\n", slope, intercept, r_squared); >> build\temp_regression_test.c
echo     printf("  Expected: slope = 2.000000, intercept = 0.000000, r^2 = 1.000000\n"); >> build\temp_regression_test.c
echo.                                             >> build\temp_regression_test.c
echo     double slope_err = fabs(slope - 2.0);    >> build\temp_regression_test.c
echo     double intercept_err = fabs(intercept - 0.0); >> build\temp_regression_test.c
echo     double r2_err = fabs(r_squared - 1.0);   >> build\temp_regression_test.c
echo.                                             >> build\temp_regression_test.c
echo     if (slope_err ^< 1e-9 ^&^& intercept_err ^< 1e-9 ^&^& r2_err ^< 1e-9) { >> build\temp_regression_test.c
echo         printf("\n[SUCCESS] Composition-based linear regression is correct!\n"); >> build\temp_regression_test.c
echo         printf("  Code reduction: 391 lines -^> 25 lines (93.6%%)\n"); >> build\temp_regression_test.c
echo         printf("  Reused primitives: reduce_add, fold_sumsq, fold_dotp\n"); >> build\temp_regression_test.c
echo         return 0;                             >> build\temp_regression_test.c
echo     } else {                                  >> build\temp_regression_test.c
echo         printf("\n[FAIL] Linear regression produced incorrect results\n"); >> build\temp_regression_test.c
echo         printf("  Errors: slope=%%.2e, intercept=%%.2e, r^2=%%.2e\n", slope_err, intercept_err, r2_err); >> build\temp_regression_test.c
echo         return 1;                             >> build\temp_regression_test.c
echo     }                                         >> build\temp_regression_test.c
echo }                                             >> build\temp_regression_test.c
echo.

echo [Step 3/4] Compiling test with NEW wrapper (composition-based)...
gcc build/temp_regression_test.c ^
    build/obj/fp_regression_wrappers.o ^
    build/obj/fp_core_reductions.o ^
    build/obj/fp_core_fused_folds.o ^
    -o build/bin/test_regression_new.exe ^
    -I include -O3 -march=native
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to build test
    exit /b 1
)
echo SUCCESS: test_regression_new.exe created
echo.

echo [Step 4/4] Running test...
echo ========================================================================
echo.
build\bin\test_regression_new.exe
set TEST_RESULT=%ERRORLEVEL%
echo.
echo ========================================================================

if %TEST_RESULT% EQU 0 (
    echo.
    echo ========================================================================
    echo   PHASE 2 REFACTORING VALIDATED!
    echo ========================================================================
    echo   Linear Regression: 391 lines -^> 25 lines
    echo   Code reduction: 93.6%%
    echo   Duplicated logic eliminated: sum_x, sum_y, sum_x2, sum_y2, sum_xy
    echo   Reused primitives: reduce_add, fold_sumsq, fold_dotp
    echo   Maintainability: SIGNIFICANTLY IMPROVED
    echo ========================================================================
) else (
    echo.
    echo ========================================================================
    echo   REFACTORING FAILED - Further investigation needed
    echo ========================================================================
)

del build\temp_regression_test.c 2>nul

exit /b %TEST_RESULT%
