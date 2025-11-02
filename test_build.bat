@echo off
gcc demo_audio_rms.c fp_core_fused_folds.o fp_core_fused_maps.o -o audio_rms.exe -lm
if exist audio_rms.exe (
    echo SUCCESS: audio_rms.exe created
) else (
    echo FAILED: Could not create audio_rms.exe
)
