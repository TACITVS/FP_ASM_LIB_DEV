@echo off
echo Checking if GCC is using SIMD...

gcc -S demo_bench_matrix_realworld.c -o scalar_output.asm -I include -O3 -march=native -fno-tree-vectorize -masm=intel 2>nul

for /f %%A in ('findstr /C:"vmov" /C:"vadd" /C:"vmul" /C:"vfma" scalar_output.asm 2^>nul ^| find /c /v ""') do set COUNT=%%A

echo.
echo AVX instructions found: %COUNT%
echo.

if %COUNT% GTR 100 (
    echo VERDICT: GCC IS AUTO-VECTORIZING ^(bad^)
    echo This explains the 1.00x speedup - both are using SIMD.
) else (
    echo VERDICT: GCC is NOT heavily vectorizing ^(good^)
    echo Both having same speed is unexpected. Needs investigation.
)

echo.
pause
