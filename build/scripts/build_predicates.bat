@echo off
echo Rebuilding bench_predicates.exe...
gcc demo_bench_predicates.c fp_core_predicates.o -o bench_predicates.exe
if exist bench_predicates.exe (
    echo SUCCESS: bench_predicates.exe created
    echo Running tests...
    bench_predicates.exe 100000 10
) else (
    echo FAILED: Could not create bench_predicates.exe
    echo Please check if Windows Defender or antivirus is blocking gcc
)
pause
