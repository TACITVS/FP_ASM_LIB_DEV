@echo off
echo Rebuilding bench_scans.exe...
gcc demo_bench_scans.c fp_core_scans.o -o bench_scans.exe
if exist bench_scans.exe (
    echo SUCCESS: bench_scans.exe created
    echo Running tests...
    bench_scans.exe 100000 5
) else (
    echo FAILED: Could not create bench_scans.exe
    echo Please check if Windows Defender or antivirus is blocking gcc
)
pause
