@echo off
echo ================================================================
echo   Building f32 Reductions Test
echo   8-wide SIMD (2X throughput vs f64!)
echo ================================================================
echo.

echo Step 1: Assembling f32 reductions...
nasm -f win64 src/asm/fp_core_reductions_f32.asm -o build/obj/fp_core_reductions_f32.o
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to assemble f32 reductions
    pause
    exit /b 1
)
echo   fp_core_reductions_f32.o assembled successfully!

echo.
echo Step 2: Building test executable...
gcc test_f32_reductions.c build/obj/fp_core_reductions_f32.o build/obj/fp_core_reductions.o -o test_f32.exe -I include -lm
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build test_f32.exe
    pause
    exit /b 1
)
echo   test_f32.exe built successfully!

echo.
echo Step 3: Running tests...
echo ================================================================
test_f32.exe

pause
