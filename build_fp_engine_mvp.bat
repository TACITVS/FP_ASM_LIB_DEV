@echo off
echo ================================================================
echo   Building FP Engine MVP Demo
echo ================================================================
echo.

echo Step 1: Assembling required ASM modules...
nasm -f win64 src/asm/fp_core_matrix.asm -o build/obj/fp_core_matrix.o
if %ERRORLEVEL% NEQ 0 ( echo FAILED on fp_core_matrix.asm & pause & exit /b 1)
echo   fp_core_matrix.o ✓

nasm -f win64 src/asm/fp_core_fused_folds_f32.asm -o build/obj/fp_core_fused_folds_f32.o
if %ERRORLEVEL% NEQ 0 ( echo FAILED on fp_core_fused_folds_f32.asm & pause & exit /b 1)
echo   fp_core_fused_folds_f32.o ✓

nasm -f win64 src/asm/fp_core_fused_maps_f32.asm -o build/obj/fp_core_fused_maps_f32.o
if %ERRORLEVEL% NEQ 0 ( echo FAILED on fp_core_fused_maps_f32.asm & pause & exit /b 1)
echo   fp_core_fused_maps_f32.o ✓

echo   fp_core_fused_maps_f32.o �

nasm -f win64 src/asm/3d_math_kernels.asm -o build/obj/3d_math_kernels.o
if %ERRORLEVEL% NEQ 0 ( echo FAILED on 3d_math_kernels.asm & pause & exit /b 1)
echo   3d_math_kernels.o �

echo Step 2: Compiling C modules...
gcc -c src/algorithms/fp_matrix_ops.c -o build/obj/fp_matrix_ops.o -I include -O3 -march=native -Wall
if %ERRORLEVEL% NEQ 0 ( echo FAILED on fp_matrix_ops.c & pause & exit /b 1)
echo   fp_matrix_ops.o ✓

gcc -c src/engine/fp_mesh_generation.c -o build/obj/fp_mesh_generation.o -I include -O3 -march=native -Wall
if %ERRORLEVEL% NEQ 0 ( echo FAILED on fp_mesh_generation.c & pause & exit /b 1)
echo   fp_mesh_generation.o ✓

gcc -c src/engine/fp_transforms.c -o build/obj/fp_transforms.o -I include -O3 -march=native -Wall
if %ERRORLEVEL% NEQ 0 ( echo FAILED on fp_transforms.c & pause & exit /b 1)
echo   fp_transforms.o ✓

gcc -c src/engine/fp_lighting.c -o build/obj/fp_lighting.o -I include -O3 -march=native -Wall
if %ERRORLEVEL% NEQ 0 ( echo FAILED on fp_lighting.c & pause & exit /b 1)
echo   fp_lighting.o ✓

gcc -c src/algorithms/fp_vector_ops.c -o build/obj/fp_vector_ops.o -I include -O3 -march=native -Wall
if %ERRORLEVEL% NEQ 0 ( echo FAILED on fp_vector_ops.c & pause & exit /b 1)
echo   fp_vector_ops.o �

echo.
echo Step 3: Building demo executable...
gcc demo_fp_engine_mvp.c ^
    build/obj/fp_mesh_generation.o ^
    build/obj/fp_transforms.o ^
    build/obj/fp_lighting.o ^
    build/obj/fp_vector_ops.o ^
    build/obj/fp_matrix_ops.o ^
    build/obj/fp_core_matrix.o ^
    build/obj/fp_core_fused_folds_f32.o ^
    build/obj/fp_core_fused_maps_f32.o ^
    build/obj/3d_math_kernels.o ^
    -o fp_engine_mvp.exe -I include -lm
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build demo executable
    pause
    exit /b 1
)
echo   fp_engine_mvp.exe ✓

echo.
echo Step 4: Running FP Engine MVP Demo...
echo ================================================================
fp_engine_mvp.exe
echo ================================================================

echo.
echo Build and run complete.
pause





