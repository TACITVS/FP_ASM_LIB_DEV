@echo off
echo ================================================================
echo   Building General HOF Performance Benchmark
echo ================================================================

echo.
echo === Step 1: Compiling general HOF wrapper ===
gcc -c src/wrappers/fp_general_hof.c -o build/obj/fp_general_hof.o -I include -O3 -march=native
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile general HOF wrapper
    pause
    exit /b 1
)
echo SUCCESS

echo.
echo === Step 2: Assembling required modules ===
nasm -f win64 src/asm/fp_core_reductions.asm -o build/obj/fp_core_reductions.o
nasm -f win64 src/asm/fp_core_simple_maps.asm -o build/obj/fp_core_simple_maps.o
nasm -f win64 src/asm/fp_core_compaction.asm -o build/obj/fp_core_compaction.o
nasm -f win64 src/asm/fp_core_fused_maps.asm -o build/obj/fp_core_fused_maps.o
echo SUCCESS

echo.
echo === Step 3: Compiling benchmark program ===
gcc bench_general_hof.c ^
    build/obj/fp_general_hof.o ^
    build/obj/fp_core_reductions.o ^
    build/obj/fp_core_simple_maps.o ^
    build/obj/fp_core_compaction.o ^
    build/obj/fp_core_fused_maps.o ^
    -o bench_general_hof.exe -I include -O3 -march=native
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile benchmark
    pause
    exit /b 1
)
echo SUCCESS

echo.
echo === Step 4: Running performance benchmark ===
echo.
bench_general_hof.exe

echo.
pause
