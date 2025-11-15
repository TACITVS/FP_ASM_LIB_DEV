@echo off
echo ==============================================================
echo   Building Production Quality 3D Showcase
echo   (1920x1080 + MSAA + Enhanced Graphics)
echo ==============================================================
echo.

echo Compiling production OpenGL demo...
gcc demo_opengl_production.c -o opengl_production.exe -I include -lopengl32 -lglu32 -lgdi32 -mwindows -lm
if %ERRORLEVEL% NEQ 0 (
    echo *** BUILD FAILED ***
    pause
    exit /b 1
)

echo Build successful!
echo.
echo ==============================================================
echo   PRODUCTION QUALITY FEATURES:
echo   - Resolution: 1920x1080 (Full HD)
echo   - Multi-sample anti-aliasing (MSAA)
echo   - High-res environment maps (512x512)
echo   - Gradient dithering (eliminates banding)
echo   - PBR-like materials (metallic/roughness)
echo   - Smooth sphere-mapped reflections
echo   - Enhanced lighting model
echo.
echo   CONTROLS:
echo   W/A/S/D - Move camera
echo   Q/E     - Move up/down
echo   +/-     - Adjust rotation speed
echo   0       - Stop rotation
echo   L       - Toggle light rotation
echo   H       - Toggle shadows
echo   ESC     - Exit
echo ==============================================================
echo.
echo Starting production demo...
opengl_production.exe
