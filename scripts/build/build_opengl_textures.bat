@echo off
echo ==============================================================
echo   Building OpenGL Texture Mapping Demo
echo   (Enhancement #1c: Procedural Textures)
echo ==============================================================
echo.

echo Compiling OpenGL texture demo...
gcc demo_opengl_textures.c -o opengl_textures.exe -I include -lopengl32 -lglu32 -lgdi32 -mwindows
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
echo   1-4     - Switch texture type
echo   T       - Toggle textures on/off
echo   ESC     - Exit
echo ==============================================================
echo.
echo Starting texture mapping demo...
opengl_textures.exe
