nasm -f win64 src/asm/3d_math_kernels.asm -o build/obj/3d_math_kernels.o
gcc tests/test_simple.c build/obj/3d_math_kernels.o -I include -o build/bin/test_simple.exe