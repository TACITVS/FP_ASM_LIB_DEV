@echo off
echo Building debug version...

gcc demo_fp_cube_debug.c ^
    -o fp_cube_debug.exe ^
    -I include ^
    -lopengl32 -lgdi32 -luser32 ^
    -O0 -g

if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build OK, running...
echo.

fp_cube_debug.exe

pause
