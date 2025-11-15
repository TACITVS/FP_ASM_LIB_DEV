@echo off
REM Build script for SSAO demo using renderer library

echo Building SSAO Demo (Renderer Library)...

REM Create build directories if needed
if not exist build\obj mkdir build\obj

REM Compile library components
echo [1/5] Compiling ECS library...
gcc -c src/engine/ecs.c -o build/obj/ecs.o -I include -std=c99 || goto :error

echo [2/5] Compiling OpenGL extensions...
gcc -c src/engine/gl_extensions.c -o build/obj/gl_extensions.o -I include -std=c99 || goto :error

echo [3/5] Compiling shader library...
gcc -c src/engine/shaders_embedded.c -o build/obj/shaders_embedded.o -I include -std=c99 || goto :error

echo [4/5] Compiling renderer library...
gcc -c src/engine/renderer_modern.c -o build/obj/renderer_modern.o -I include -std=c99 || goto :error

echo [5/5] Linking SSAO demo...
gcc demo_renderer_ssao.c ^
    build/obj/ecs.o ^
    build/obj/gl_extensions.o ^
    build/obj/shaders_embedded.o ^
    build/obj/renderer_modern.o ^
    -o ssao_demo.exe ^
    -I include ^
    -lopengl32 -lgdi32 ^
    -std=c99 || goto :error

echo.
echo ========================================
echo Build successful!
echo Run: ssao_demo.exe
echo ========================================
goto :eof

:error
echo.
echo ========================================
echo Build FAILED!
echo ========================================
exit /b 1
