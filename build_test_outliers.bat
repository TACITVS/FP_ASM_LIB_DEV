@echo off
echo === Step 1: Compiling percentile wrapper (includes outlier functions) ===
gcc -c src\wrappers\fp_percentile_wrappers.c -o build\obj\fp_percentile_wrappers.o -I include -O3 -march=native
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile percentile wrapper
    exit /b 1
)
echo SUCCESS: percentile wrapper compiled (with outlier functions)

echo.
echo === Step 2: Creating test program ===
(
echo #include ^<stdio.h^>
echo #include ^<stdlib.h^>
echo #include ^<stdint.h^>
echo #include "include/fp_core.h"
echo.
echo int main^(void^) {
echo     // Test 1: Z-score outlier detection
echo     // Data: 1,2,3,4,5,100  ^(100 is outlier with Z=2.23, threshold=2.0^)
echo     double data1[] = {1.0, 2.0, 3.0, 4.0, 5.0, 100.0};
echo     size_t n1 = 6;
echo     uint8_t outliers1[6];
echo.
echo     size_t count1 = fp_detect_outliers_zscore_f64^(data1, n1, 2.0, outliers1^);
echo.
echo     printf^("Test 1 ^(Z-score^): Detected %%zu outliers\n", count1^);
echo     printf^("  Outliers: "^);
echo     for ^(size_t i = 0; i ^< n1; i++^) {
echo         if ^(outliers1[i]^) printf^("%%.0f ", data1[i]^);
echo     }
echo     printf^("\n"^);
echo.
echo     // Test 2: IQR outlier detection
echo     // Data with outliers on both ends
echo     double data2[] = {-100.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 100.0};
echo     size_t n2 = 10;
echo     uint8_t outliers2[10];
echo.
echo     size_t count2 = fp_detect_outliers_iqr_f64^(data2, n2, 1.5, outliers2^);
echo.
echo     printf^("\nTest 2 ^(IQR^): Detected %%zu outliers\n", count2^);
echo     printf^("  Outliers: "^);
echo     for ^(size_t i = 0; i ^< n2; i++^) {
echo         if ^(outliers2[i]^) printf^("%%.0f ", data2[i]^);
echo     }
echo     printf^("\n"^);
echo.
echo     // Test 3: No outliers in uniform data
echo     double data3[] = {1.0, 2.0, 3.0, 4.0, 5.0};
echo     size_t n3 = 5;
echo     uint8_t outliers3[5];
echo.
echo     size_t count3 = fp_detect_outliers_zscore_f64^(data3, n3, 2.0, outliers3^);
echo.
echo     printf^("\nTest 3 ^(No outliers^): Detected %%zu outliers\n", count3^);
echo.
echo     // Validation
echo     int pass = 1;
echo.
echo     // Test 1 should detect 1 outlier ^(100^)
echo     if ^(count1 != 1^) {
echo         printf^("[FAIL] Test 1: Expected 1 outlier, got %%zu\n", count1^);
echo         pass = 0;
echo     }
echo.
echo     // Test 2 should detect 2 outliers ^(-100, 100^)
echo     if ^(count2 != 2^) {
echo         printf^("[FAIL] Test 2: Expected 2 outliers, got %%zu\n", count2^);
echo         pass = 0;
echo     }
echo.
echo     // Test 3 should detect 0 outliers
echo     if ^(count3 != 0^) {
echo         printf^("[FAIL] Test 3: Expected 0 outliers, got %%zu\n", count3^);
echo         pass = 0;
echo     }
echo.
echo     if ^(pass^) {
echo         printf^("\n[SUCCESS] Outlier detection is correct!\n"^);
echo         printf^("  Code reduction: 341 lines -^> ~50 lines ^(~85%%^)\n"^);
echo         printf^("  Z-score: Composes from fp_descriptive_stats_f64\n"^);
echo         printf^("  IQR: Composes from fp_quartiles_f64\n"^);
echo         return 0;
echo     } else {
echo         printf^("\n[FAIL] Incorrect results\n"^);
echo         return 1;
echo     }
echo }
) > test_outliers.c

echo.
echo === Step 3: Compiling test program ===
gcc test_outliers.c build\obj\fp_percentile_wrappers.o build\obj\fp_core_descriptive_stats.o build\obj\fp_core_percentiles.o build\obj\fp_core_reductions.o build\obj\fp_core_fused_folds.o -o test_outliers.exe -I include -O3 -march=native
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile test
    exit /b 1
)
echo SUCCESS: test compiled

echo.
echo === Step 4: Running test ===
test_outliers.exe
set RESULT=%ERRORLEVEL%

if %RESULT% EQU 0 (
    echo.
    echo ======================================================
    echo   PHASE 4 REFACTORING VALIDATED!
    echo ======================================================
    echo   Outlier Detection: 341 lines -^> ~50 lines
    echo   Code reduction: ~85%%
    echo ======================================================
)

del test_outliers.c 2>nul
exit /b %RESULT%
