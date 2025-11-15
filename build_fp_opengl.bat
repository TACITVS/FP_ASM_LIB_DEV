@echo off
echo ================================================================
echo   Building FP OpenGL Demo - Step 3: Full FP Pipeline with MSAA
echo ================================================================
echo.

echo Step 1: Assembling ASM modules...
nasm -f win64 src/asm/fp_core_matrix.asm -o build/obj/fp_core_matrix.o
if %ERRORLEVEL% NEQ 0 ( echo FAILED on fp_core_matrix.asm & pause & exit /b 1)
echo   fp_core_matrix.o ✓
nasm -f win64 src/asm/fp_core_fused_folds_f32.asm -o build/obj/fp_core_fused_folds_f32.o
if %ERRORLEVEL% NEQ 0 ( echo FAILED on fp_core_fused_folds_f32.asm & pause & exit /b 1)
echo   fp_core_fused_folds_f32.o ✓
nasm -f win64 src/asm/fp_core_fused_maps_f32.asm -o build/obj/fp_core_fused_maps_f32.o
if %ERRORLEVEL% NEQ 0 ( echo FAILED on fp_core_fused_maps_f32.asm & pause & exit /b 1)
echo   fp_core_fused_maps_f32.o ✓
echo.

echo Step 2: Compiling C modules...
gcc -c src/engine_old/gl_extensions.c -o build/obj/gl_extensions.o -I include
if %ERRORLEVEL% NEQ 0 ( echo FAILED on gl_extensions.c & pause & exit /b 1)
echo   gl_extensions.o ✓
gcc -c src/engine/fp_mesh_generation.c -o build/obj/fp_mesh_generation.o -I include -DFP_MESH_ENABLE_OPENGL=1
if %ERRORLEVEL% NEQ 0 ( echo FAILED on fp_mesh_generation.c & pause & exit /b 1)
echo   fp_mesh_generation.o ✓
gcc -c src/engine/fp_transforms.c -o build/obj/fp_transforms.o -I include
if %ERRORLEVEL% NEQ 0 ( echo FAILED on fp_transforms.c & pause & exit /b 1)
echo   fp_transforms.o ✓
gcc -c src/engine/fp_lighting.c -o build/obj/fp_lighting.o -I include
if %ERRORLEVEL% NEQ 0 ( echo FAILED on fp_lighting.c & pause & exit /b 1)
echo   fp_lighting.o ✓
gcc -c src/engine/fp_shadow_mapping.c -o build/obj/fp_shadow_mapping.o -I include
if %ERRORLEVEL% NEQ 0 ( echo FAILED on fp_shadow_mapping.c & pause & exit /b 1)
echo   fp_shadow_mapping.o ✓
gcc -c src/engine/fp_msaa.c -o build/obj/fp_msaa.o -I include
if %ERRORLEVEL% NEQ 0 ( echo FAILED on fp_msaa.c & pause & exit /b 1)
echo   fp_msaa.o ✓
gcc -c src/engine/renderer.c -o build/obj/renderer.o -I include
if %ERRORLEVEL% NEQ 0 ( echo FAILED on renderer.c & pause & exit /b 1)
echo   renderer.o ✓
gcc -c src/platform/platform_win32.c -o build/obj/platform_win32.o -I include
if %ERRORLEVEL% NEQ 0 ( echo FAILED on platform_win32.c & pause & exit /b 1)
echo   platform_win32.o ✓
gcc -c src/algorithms/fp_vector_ops.c -o build/obj/fp_vector_ops.o -I include
if %ERRORLEVEL% NEQ 0 ( echo FAILED on fp_vector_ops.c & pause & exit /b 1)
echo   fp_vector_ops.o ✓
echo.

echo Step 3: Building demo executable...
gcc demo_fp_opengl.c ^
    build/obj/gl_extensions.o ^
    build/obj/fp_mesh_generation.o ^
    build/obj/fp_transforms.o ^
    build/obj/fp_lighting.o ^
    build/obj/fp_shadow_mapping.o ^
    build/obj/fp_msaa.o ^
    build/obj/renderer.o ^
    build/obj/platform_win32.o ^
    build/obj/fp_vector_ops.o ^
    build/obj/fp_matrix_ops.o ^
    build/obj/fp_core_matrix.o ^
    build/obj/fp_core_fused_folds_f32.o ^
    build/obj/fp_core_fused_maps_f32.o ^
    build/obj/3d_math_kernels.o ^
    -o fp_opengl.exe -I include -lgdi32 -lopengl32 -lm
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build demo executable
    pause
    exit /b 1
)
echo   fp_opengl.exe ✓

echo.
echo Step 4: Running demo...
echo ================================================================
echo   Rendering an MSAA-enabled transformed and lit cube. Press ESC to close.
echo ================================================================
fp_opengl.exe

echo.
echo Build and run complete.
pause



