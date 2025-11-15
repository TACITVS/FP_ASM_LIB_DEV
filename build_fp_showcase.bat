@echo off
echo ========================================
echo Building FP-First Graphics Showcase
echo ========================================
echo.

gcc demo_fp_graphics_showcase.c ^
    build\obj\fp_core_fused_folds_f32.o ^
    build\obj\fp_core_fused_maps_f32.o ^
    -o fp_showcase.exe ^
    -I include ^
    -lm ^
    -Wall

if exist fp_showcase.exe (
    echo.
    echo ========================================
    echo Build Complete!
    echo ========================================
    echo Run: fp_showcase.exe
    echo Output: fp_graphics_showcase.ppm
) else (
    echo.
    echo ERROR: Build failed!
    exit /b 1
)
