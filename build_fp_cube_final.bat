@echo off
echo ================================================================
echo   Building FP-First 3D Cube - FINAL VERSION
echo ================================================================
echo.

gcc demo_fp_cube_final.c ^
    build\obj\fp_core_fused_folds_f32.o ^
    -o fp_cube_final.exe ^
    -I include ^
    -lopengl32 -lgdi32 -luser32 ^
    -O3 -march=native

if errorlevel 1 (
    echo Build FAILED!
    pause
    exit /b 1
)

echo.
echo ================================================================
echo   BUILD SUCCESSFUL
echo ================================================================
echo.
echo Running FP-First Cube Demo...
echo The window will stay open until you press ESC or close it.
echo.

fp_cube_final.exe

echo.
pause
