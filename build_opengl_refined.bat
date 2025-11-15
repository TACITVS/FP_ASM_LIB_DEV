@echo off
echo ==============================================================
echo   Building Refined OpenGL Showcase
echo   (All Visual Enhancements + Refinements)
echo ==============================================================
echo.

echo Compiling refined OpenGL demo...
gcc demo_opengl_refined.c -o opengl_refined.exe -I include -lopengl32 -lglu32 -lgdi32 -mwindows -lm
if %ERRORLEVEL% NEQ 0 (
    echo *** BUILD FAILED ***
    pause
    exit /b 1
)

echo Build successful!
echo.
echo ==============================================================
echo   REFINEMENTS:
echo   - 10x slower, organic rotation (Perlin noise)
echo   - Soft shadows (30%% opacity)
echo   - Ambient occlusion approximation
echo   - Anti-aliasing hints
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
echo Starting refined demo...
opengl_refined.exe
