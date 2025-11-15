@echo off
echo Building vector operations test...
if not exist build\obj mkdir build\obj
nasm -f win64 src/asm/3d_math_kernels.asm -o build/obj/3d_math_kernels.o
gcc -c src/algorithms/fp_vector_ops.c -o build/obj/fp_vector_ops.o -I include
gcc test_vector_ops.c build/obj/fp_vector_ops.o build/obj/3d_math_kernels.o -o test_vector_ops.exe -I include -lm
test_vector_ops.exe
pause
