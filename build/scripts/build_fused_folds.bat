@echo off
echo Rebuilding bench_fused_folds.exe...
gcc demo_bench_fused_folds.c fp_core_fused_folds.o -o bench_fused_folds.exe
if exist bench_fused_folds.exe (
    echo SUCCESS: bench_fused_folds.exe created
    echo Running tests...
    bench_fused_folds.exe 100000 5
) else (
    echo FAILED: Could not create bench_fused_folds.exe
    echo Please check if Windows Defender or antivirus is blocking gcc
)
pause
