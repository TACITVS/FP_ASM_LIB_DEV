@echo off
echo Testing batched vertex transform correctness...
echo.

nasm -f win64 src/asm/fp_core_matrix.asm -o build/obj/fp_core_matrix.o
if %ERRORLEVEL% NEQ 0 (
    echo ASSEMBLY FAILED
    pause
    exit /b 1
)

gcc test_batch_correctness.c build/obj/fp_core_matrix.o -o test_batch.exe -I include -lm
if %ERRORLEVEL% NEQ 0 (
    echo COMPILATION FAILED
    pause
    exit /b 1
)

echo Running correctness test...
echo.
test_batch.exe

pause
