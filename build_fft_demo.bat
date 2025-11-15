@echo off
echo ================================================================
echo   Building Fast Fourier Transform (FFT) Demo
echo   Showcasing FP-ASM Signal Processing
echo ================================================================
echo.

echo Step 1: Compiling FFT algorithm...
gcc -c src/algorithms/fp_fft.c -o build/obj/fp_fft.o -I include
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to compile fp_fft.c
    pause
    exit /b 1
)
echo   fp_fft.o ✓
echo.

echo Step 2: Building demo executable...
gcc demo_fft.c build/obj/fp_fft.o -o fft_demo.exe -I include -lm
if %ERRORLEVEL% NEQ 0 (
    echo FAILED to build fft_demo.exe
    pause
    exit /b 1
)
echo   fft_demo.exe ✓
echo.

echo Step 3: Running FFT demo...
echo ================================================================
fft_demo.exe

pause
