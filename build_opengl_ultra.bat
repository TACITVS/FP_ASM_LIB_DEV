@echo off
echo ==============================================================
echo   Building ULTRA Quality Demo
echo   (Near-Raytracing Visuals at Real-Time Speeds)
echo ==============================================================
echo.

echo Compiling ultra quality OpenGL demo...
gcc demo_opengl_ultra.c -o opengl_ultra.exe -I include -lopengl32 -lglu32 -lgdi32 -mwindows -lm
if %ERRORLEVEL% NEQ 0 (
    echo *** BUILD FAILED ***
    pause
    exit /b 1
)

echo Build successful!
echo.
echo ==============================================================
echo   ULTRA QUALITY FEATURES:
echo   - Resolution: 1920x1080 (Full HD)
echo   - Cubemap reflections (6-face environment mapping)
echo   - Fresnel-based reflection strength
echo   - Multiple light sources (main + fill)
echo   - PBR metallic workflow
echo   - Ultra-soft shadows (15%% opacity)
echo   - 300 cubes for optimal performance
echo.
echo   GOAL: Raytracing-quality visuals at 50-60 FPS
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
echo Starting ultra quality demo...
opengl_ultra.exe
