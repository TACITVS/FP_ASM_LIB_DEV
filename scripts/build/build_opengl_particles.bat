@echo off
echo ==============================================================
echo   Building OpenGL Particle Systems Demo
echo   (Enhancement #1b: Particle Effects)
echo ==============================================================
echo.

echo Compiling OpenGL particle demo...
gcc demo_opengl_particles.c -o opengl_particles.exe -I include -lopengl32 -lglu32 -lgdi32 -mwindows
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
echo   1-4     - Switch effect type
echo   P       - Pause/Resume
echo   ESC     - Exit
echo ==============================================================
echo.
echo Starting particle systems demo...
opengl_particles.exe
