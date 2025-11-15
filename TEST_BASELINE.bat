@echo off
echo ================================================================
echo   Ray Tracer Baseline Test (Quick Sanity Check)
echo   This tests the ORIGINAL working version
echo ================================================================
echo.

echo Running the original demo (already tested and working)...
echo.

if exist ray_tracer_demo.exe (
    ray_tracer_demo.exe
) else (
    echo ray_tracer_demo.exe not found. Building it first...
    call build_ray_tracer_demo.bat
)

echo.
echo ================================================================
echo   Baseline Test Complete!
echo   If this worked, you should see:
echo   - output_realtime.ppm (400x300)
echo   - output_offline.ppm (800x600)
echo   - output_hq.ppm (1920x1080)
echo   - Render times printed above
echo ================================================================
pause
