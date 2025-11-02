@echo off
echo Building FP-ASM List FP Early-Exit Operations Test Suite...
echo.

echo Step 1: Assembling fp_core_compaction.asm
nasm -f win64 fp_core_compaction.asm -o fp_core_compaction.o
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Assembly failed
    exit /b 1
)
echo OK

echo.
echo Step 2: Compiling and linking demo_take_drop.c
gcc demo_take_drop.c fp_core_compaction.o -o demo_take_drop.exe
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Compilation failed
    exit /b 1
)
echo OK

echo.
echo Step 3: Running tests and benchmarks
demo_take_drop.exe 10000000 10

echo.
echo Build complete!
