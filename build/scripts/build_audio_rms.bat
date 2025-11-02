@echo off
echo ========================================
echo Building Audio RMS Normalization Demo
echo ========================================
echo.
echo Compiling demo_audio_rms.c with FP-ASM library...
gcc demo_audio_rms.c fp_core_fused_folds.o fp_core_fused_maps.o -o audio_rms.exe -lm 2^>^&1
if exist audio_rms.exe (
    echo SUCCESS: audio_rms.exe created
    echo.
    echo ========================================
    echo Running Audio RMS Benchmark
    echo ========================================
    echo.
    echo Usage: audio_rms.exe [n_samples] [iterations]
    echo Example: audio_rms.exe 20000000 30
    echo.
    echo Running with defaults (10M samples, 50 iterations)...
    echo.
    audio_rms.exe
) else (
    echo FAILED: Could not create audio_rms.exe
    echo Please check if Windows Defender or antivirus is blocking gcc
)
pause
