@echo off
echo ==============================================================
echo   Building OpenGL Environment Reflections Demo
echo   (Enhancement #1d: Sphere-Mapped Reflections)
echo ==============================================================
echo.

echo Compiling OpenGL reflections demo...
gcc demo_opengl_reflections.c -o opengl_reflections.exe -I include -lopengl32 -lglu32 -lgdi32 -mwindows
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
echo   R       - Toggle reflections on/off
echo   1-3     - Adjust reflection intensity
echo   ESC     - Exit
echo ==============================================================
echo.
echo Starting reflections demo...
opengl_reflections.exe
