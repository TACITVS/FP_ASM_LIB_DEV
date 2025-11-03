@echo off
echo Reassembling u32 fused_maps module with fix...
nasm -f win64 src/asm/fp_core_fused_maps_u32.asm -o build/obj/fp_core_fused_maps_u32.o
if %ERRORLEVEL% NEQ 0 (
    echo Assembly failed!
    pause
    exit /b 1
)
echo   Assembly OK!
echo.

echo Compiling u32 comprehensive test...
echo.

gcc test_u32_comprehensive.c build\obj\fp_core_reductions_u32.o build\obj\fp_core_fused_folds_u32.o build\obj\fp_core_fused_maps_u32.o -o test_u32_comprehensive.exe -I include

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build successful! Running test...
    echo.
    test_u32_comprehensive.exe
) else (
    echo.
    echo Build failed with error code %ERRORLEVEL%
)

pause
