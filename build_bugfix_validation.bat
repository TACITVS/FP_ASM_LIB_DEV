@echo off
echo Building bugfix validation test...

REM Assemble all required ASM files
nasm -f win64 -i src/asm/ src/asm/fp_core_reductions_u8.asm -o src/asm/fp_core_reductions_u8.obj
nasm -f win64 -i src/asm/ src/asm/fp_core_reductions_u64.asm -o src/asm/fp_core_reductions_u64.obj
nasm -f win64 -i src/asm/ src/asm/fp_core_reductions_f32.asm -o src/asm/fp_core_reductions_f32.obj
nasm -f win64 -i src/asm/ src/asm/fp_core_matrix.asm -o src/asm/fp_core_matrix.obj

REM Compile C code and link with object files
gcc -o bugfix_validation.exe benchmarks/demo_bugfix_validation.c ^
    src/asm/fp_core_reductions_u8.obj ^
    src/asm/fp_core_reductions_u64.obj ^
    src/asm/fp_core_reductions_f32.obj ^
    src/asm/fp_core_matrix.obj ^
    -Iinclude -O3 -mavx2 -mfma

if %ERRORLEVEL% EQU 0 (
    echo Build successful!
    echo Running validation tests...
    echo.
    bugfix_validation.exe
) else (
    echo Build failed!
    exit /b 1
)
