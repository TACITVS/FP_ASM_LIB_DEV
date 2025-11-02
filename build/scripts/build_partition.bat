@echo off
echo ========================================
echo Building Partition Demo
echo ========================================
echo.
echo Assembling fp_core_compaction.asm...
nasm -f win64 fp_core_compaction.asm -o fp_core_compaction.o

if exist fp_core_compaction.o (
    echo SUCCESS: fp_core_compaction.o created
    echo.
    echo Building partition demo...
    gcc demo_partition.c fp_core_compaction.o -o partition.exe

    if exist partition.exe (
        echo SUCCESS: partition.exe created
        echo.
        echo ========================================
        echo Testing Partition - List FP Operation
        echo ========================================
        echo.
        partition.exe
    ) else (
        echo FAILED: Could not create partition.exe
    )
) else (
    echo FAILED: Could not assemble fp_core_compaction.o
)
pause
