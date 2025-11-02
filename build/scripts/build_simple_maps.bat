@echo off
echo Rebuilding bench_simple_maps.exe...
gcc demo_bench_simple_maps.c fp_core_simple_maps.o -o bench_simple_maps.exe
if exist bench_simple_maps.exe (
    echo SUCCESS: bench_simple_maps.exe created
    echo Running tests...
    bench_simple_maps.exe 100000 5
) else (
    echo FAILED: Could not create bench_simple_maps.exe
    echo Please check if Windows Defender or antivirus is blocking gcc
)
pause
