@echo off
echo Building minimal FP cube...

gcc demo_fp_cube_minimal.c ^
    build\obj\fp_core_fused_folds_f32.o ^
    -o fp_cube_minimal.exe ^
    -I include ^
    -lopengl32 -lgdi32 -luser32 ^
    -O2

if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build OK, running...
echo.

fp_cube_minimal.exe

pause
