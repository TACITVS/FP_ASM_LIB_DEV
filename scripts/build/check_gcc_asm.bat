@echo off
echo Generating GCC assembly output for scalar mat4_mul...
gcc -S demo_bench_matrix.c -o gcc_output.asm -I include -O3 -march=native -masm=intel
echo Done! Check gcc_output.asm
echo.
echo Looking for SIMD instructions in scalar code...
findstr /C:"vmul" /C:"vadd" /C:"vfmadd" /C:"ymm" /C:"xmm" gcc_output.asm | head -20
pause
