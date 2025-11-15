@echo off
echo ========================================
echo Building Post-Processing Test (FP-First Phase 4)
echo ========================================
echo.

gcc demo_postprocess_test.c ^
    build\obj\fp_core_fused_maps_f32.o ^
    -o postprocess_test.exe ^
    -I include ^
    -lm ^
    -Wall

if exist postprocess_test.exe (
    echo.
    echo ========================================
    echo Build Complete!
    echo ========================================
    echo Run: postprocess_test.exe
) else (
    echo.
    echo ERROR: Build failed!
    exit /b 1
)
