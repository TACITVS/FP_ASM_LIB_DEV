@echo off
cls
echo ================================================================
echo   FP-ASM Modern Game Engine - BUILD SCRIPT
echo   (ECS + PBR + Shadows + FXAA + SSAO)
echo ================================================================
echo.

REM Create build/obj directory if it doesn't exist
if not exist "build\obj" mkdir "build\obj"

REM Step 0: Assemble FP-ASM Library components
echo [0/9] Assembling FP-ASM Library components...
nasm -f win64 src\asm\fp_core_matrix.asm -o build\obj\fp_core_matrix.o
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo *** FAILED: FP-ASM Core Matrix assembly failed ***
    pause
    exit /b 1
)
nasm -f win64 src\asm\fp_core_fused_folds_f32.asm -o build\obj\fp_core_fused_folds_f32.o
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo *** FAILED: FP-ASM Fused Folds assembly failed ***
    pause
    exit /b 1
)
echo      [OK] FP-ASM Library components assembled

REM Step 1: Compiling FP-ASM C-based algorithms
echo [1/9] Compiling FP-ASM C-based algorithms...
gcc -c src/algorithms/fp_matrix_ops.c -o build/obj/fp_matrix_ops.o -I include -std=c99 -O2 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo *** FAILED: fp_matrix_ops.c compilation failed ***
    pause
    exit /b 1
)
gcc -c src/algorithms/fp_vector_ops.c -o build/obj/fp_vector_ops.o -I include -std=c99 -O2 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo *** FAILED: fp_vector_ops.c compilation failed ***
    pause
    exit /b 1
)
gcc -c src/engine/fp_mesh_generation.c -o build/obj/fp_mesh_generation.o -I include -std=c99 -O2 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo *** FAILED: fp_mesh_generation.c compilation failed ***
    pause
    exit /b 1
)
gcc -c src/engine/fp_engine_algorithms.c -o build/obj/fp_engine_algorithms.o -I include -std=c99 -O2 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo *** FAILED: fp_engine_algorithms.c compilation failed ***
    pause
    exit /b 1
)
gcc -c src/algorithms/fp_quaternion_ops.c -o build/obj/fp_quaternion_ops.o -I include -std=c99 -O2 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo *** FAILED: fp_quaternion_ops.c compilation failed ***
    pause
    exit /b 1
)
echo      [OK] FP-ASM C-based algorithms compiled

REM Step 2: ECS Core
echo [2/9] Compiling ECS core (ecs.c)...
gcc -c src/ecs.c -o build/obj/ecs.o -I include -std=c99 -O2 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo *** FAILED: ECS compilation failed ***
    pause
    exit /b 1
)
echo      [OK] ecs.o created

REM Step 3: OpenGL Extensions
echo [3/9] Compiling OpenGL extension loader (gl_extensions.c)...
gcc -c src/engine_old/gl_extensions.c -o build/obj/gl_extensions.o -I include -std=c99 -O2 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo *** FAILED: OpenGL extensions compilation failed ***
    pause
    exit /b 1
)
echo      [OK] gl_extensions.o created

REM Step 4: Embedded Shaders
echo [4/9] Compiling embedded shaders (shaders_embedded.c)...
gcc -c src/engine_old/shaders_embedded.c -o build/obj/shaders_embedded.o -I include -std=c99 -O2 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo *** FAILED: Shaders compilation failed ***
    pause
    exit /b 1
)
echo      [OK] shaders_embedded.o created

REM Step 5: Modern Renderer
echo [5/9] Compiling modern renderer (renderer_modern.c)...
gcc -c src/engine_old/renderer_modern.c -o build/obj/renderer_modern.o -I include -std=c99 -O2 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo *** FAILED: Renderer compilation failed ***
    pause
    exit /b 1
)
echo      [OK] renderer_modern.o created

REM Step 6: Demo Application
echo [6/9] Compiling demo application (demo_engine_mvp.c)...
gcc -c demo_engine_mvp.c -o build/obj/demo_engine_mvp.o -I include -std=c99 -O2 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo *** FAILED: Demo compilation failed ***
    pause
    exit /b 1
)
echo      [OK] demo_engine_mvp.o created

REM Step 7: Linking
echo.
echo [LINK] Linking final executable (engine_mvp.exe)...
gcc build/obj/ecs.o ^
    build/obj/gl_extensions.o ^
    build/obj/shaders_embedded.o ^
    build/obj/renderer_modern.o ^
    build/obj/demo_engine_mvp.o ^
    build/obj/fp_core_matrix.o ^
    build/obj/fp_core_fused_folds_f32.o ^
    build/obj/fp_matrix_ops.o ^
    build/obj/fp_vector_ops.o ^
    build/obj/fp_mesh_generation.o ^
    build/obj/fp_engine_algorithms.o ^
    build/obj/fp_quaternion_ops.o ^
    -o engine_mvp.exe ^
    -lopengl32 -lgdi32 -lm 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo *** FAILED: Linking failed ***
    pause
    exit /b 1
)
echo       [OK] engine_mvp.exe created

echo.
echo ================================================================
echo   BUILD SUCCESSFUL!
echo ================================================================
echo.
echo Executable: engine_mvp.exe
echo.
echo FEATURES:
echo   - ECS architecture (10,000+ entities supported)
echo   - PBR materials (Cook-Torrance BRDF)
echo   - Shadow mapping with PCF (soft shadows)
echo   - FXAA anti-aliasing
echo   - Tone mapping + gamma correction
echo   - 500 animated cubes with varying materials
echo   - Dual directional lights
echo   - Screen-Space Ambient Occlusion (SSAO)
echo.
echo CONTROLS:
echo   W/A/S/D - Move camera
echo   Q/E     - Move up/down
echo   ESC     - Exit
echo.
echo TARGET: 60 FPS @ 1920x1080
echo ================================================================
echo.
pause