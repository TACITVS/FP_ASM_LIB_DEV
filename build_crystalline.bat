@echo off
echo ========================================
echo Building FP-First Crystalline Demo
echo ========================================
echo.

gcc demo_fp_crystalline.c ^
    -o fp_crystalline.exe ^
    -I include ^
    -lopengl32 -lglu32 -lgdi32 ^
    -mwindows ^
    -lm ^
    -Wall

if exist fp_crystalline.exe (
    echo.
    echo ========================================
    echo Build Complete!
    echo ========================================
    echo.
    echo Features:
    echo   - Rotating crystalline cube with Perlin noise
    echo   - Plasma flowing inside
    echo   - Checkered creamy floor
    echo   - Sky gradient horizon
    echo   - Soft shadows
    echo   - Orbiting camera
    echo.
    echo Run: fp_crystalline.exe
    echo Press ESC to exit
    echo.
) else (
    echo.
    echo ERROR: Build failed!
    exit /b 1
)
