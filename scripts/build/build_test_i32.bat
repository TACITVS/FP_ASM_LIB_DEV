@echo off
echo ================================================================
echo   Building i32 Reductions Test
echo   8-wide SIMD (2X throughput vs i64!)
echo ================================================================
echo.

echo Step 1: Assembling i32 reductions...
nasm -f win64 src/asm/fp_core_reductions_i32.asm -o build/obj/fp_core_reductions_i32.o
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to assemble i32 reductions
    pause
    exit /b 1
)
echo   fp_core_reductions_i32.o assembled successfully!

echo.
echo Step 2: Building test executable...
gcc test_i32_reductions.c build/obj/fp_core_reductions_i32.o build/obj/fp_core_reductions.o -o test_i32.exe -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build test_i32.exe
    pause
    exit /b 1
)
echo   test_i32.exe built successfully!

echo.
echo Step 3: Running tests...
echo ================================================================
test_i32.exe

pause
