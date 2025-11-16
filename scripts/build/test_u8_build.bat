@echo off
echo Building u8 comprehensive test...
echo.

echo Step 1: Assembling modules (already done)
echo.

echo Step 2: Compiling test executable...
gcc test_u8_comprehensive.c build\obj\fp_core_reductions_u8.o build\obj\fp_core_fused_folds_u8.o build\obj\fp_core_fused_maps_u8.o -o test_u8_comprehensive.exe -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile!
    pause
    exit /b 1
)
echo Success!
echo.

echo Step 3: Running test...
test_u8_comprehensive.exe

pause
