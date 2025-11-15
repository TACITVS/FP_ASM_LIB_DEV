@echo off
echo ================================================================
echo   Building FP-First Shadow Mapping Demo
echo ================================================================
echo.

echo [1/2] Compiling shadow mapping library...
gcc -c src/engine/fp_shadow_mapping.c -o build/obj/fp_shadow_mapping.o -I include -O3 -march=native 2>&1

if errorlevel 1 (
    echo.
    echo ❌ Shadow mapping compilation FAILED!
    pause
    exit /b 1
)

echo ✓ Shadow mapping compiled successfully
echo.

echo [2/2] Linking demo executable...
gcc demo_fp_shadow_mapping.c build/obj/fp_shadow_mapping.o build/obj/fp_core_fused_folds_f32.o build/obj/fp_core_reductions_f32.o -o fp_shadow_mapping.exe -I include -lm -O3 -march=native 2>&1

if errorlevel 1 (
    echo.
    echo ❌ Demo linking FAILED!
    pause
    exit /b 1
)

echo ✓ Demo linked successfully
echo.
echo ================================================================
echo   BUILD SUCCESSFUL
echo ================================================================
echo.
echo Running FP-First Shadow Mapping Tests...
echo.
echo Expected output:
echo   - Light space transform computation
echo   - Vertex transformations to light space
echo   - Shadow visibility testing
echo   - PCF soft shadow tests
echo   - Performance benchmark
echo.

fp_shadow_mapping.exe

echo.
pause
