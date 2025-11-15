@echo off
echo ==============================================================
echo   Building OpenGL Planar Shadows Demo
echo   (Enhancement #1: Real-time Dynamic Shadows)
echo ==============================================================
echo.

echo Compiling OpenGL planar shadow demo...
gcc demo_opengl_planar_shadows.c -o opengl_shadows.exe -I include -lopengl32 -lglu32 -lgdi32 -mwindows
if %ERRORLEVEL% NEQ 0 (
    echo *** BUILD FAILED ***
    pause
    exit /b 1
)

echo Build successful!
echo.
echo ==============================================================
echo   CONTROLS:
echo   W/A/S/D - Move camera
echo   Q/E     - Move up/down
echo   L       - Toggle light rotation
echo   ESC     - Exit
echo ==============================================================
echo.
echo Starting planar shadow demo...
opengl_shadows.exe
