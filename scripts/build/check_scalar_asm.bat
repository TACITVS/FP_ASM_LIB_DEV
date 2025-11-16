@echo off
echo Generating assembly for scalar vertex transform...
echo.

gcc -S demo_bench_matrix_realworld.c -o scalar_output.asm -I include -O3 -march=native -fno-tree-vectorize -masm=intel

echo Looking for SIMD instructions in transform_vertices_scalar...
echo.
echo === Searching for vmovups, vmulps, vaddps, vfmadd ===
findstr /C:"vmovups" /C:"vmulps" /C:"vaddps" /C:"vfmadd" scalar_output.asm

echo.
echo === Looking for transform_vertices_scalar function ===
findstr /N /C:"transform_vertices_scalar" scalar_output.asm

echo.
echo === Searching for any v-prefixed SIMD instructions (AVX) ===
findstr /C:"vmov" /C:"vadd" /C:"vmul" /C:"vfma" scalar_output.asm > simd_found.txt
echo Found instructions saved to simd_found.txt
type simd_found.txt | more

pause
