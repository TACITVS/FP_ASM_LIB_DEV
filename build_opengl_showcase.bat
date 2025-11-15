@echo off
echo ==============================================================
echo   Building OpenGL Visual Showcase
echo   (1000 Rotating Cubes with Phong Lighting)
echo ==============================================================
echo.

echo Compiling OpenGL demo...
gcc demo_opengl_showcase.c -o opengl_showcase.exe -I include -lopengl32 -lglu32 -lgdi32 -mwindows
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
echo   ESC     - Exit
echo ==============================================================
echo.
echo Starting visual showcase...
opengl_showcase.exe
