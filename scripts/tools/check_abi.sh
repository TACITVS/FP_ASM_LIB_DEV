#!/bin/bash

echo "=== WINDOWS X64 ABI COMPLIANCE CHECK ==="
echo

echo "1. Checking argument usage (RCX=1st, RDX=2nd, R8=3rd, R9=4th):"
grep -n "mov.*r12.*rcx\|mov.*r13.*rdx\|mov.*r14.*r8\|mov.*rcx.*r9" fp_core_*.asm | head -10

echo
echo "2. Checking non-volatile register preservation (push/pop pairs):"
for reg in rbx rbp rdi rsi r12 r13 r14 r15; do
    push_count=$(grep -c "push $reg" fp_core_*.asm)
    pop_count=$(grep -c "pop $reg" fp_core_*.asm)
    if [ $push_count -eq $pop_count ]; then
        echo "  $reg: $push_count pushes, $pop_count pops ✓"
    else
        echo "  $reg: $push_count pushes, $pop_count pops ✗ MISMATCH!"
    fi
done

echo
echo "3. Checking return value placement (RAX for int, XMM0 for double):"
echo "  Functions returning to RAX:"
grep -B20 "^\.done:" fp_core_reductions.asm | grep "xor  rax\|mov  rax\|add  rax" | head -5
echo "  Functions using XMM0 for FP returns should have result in XMM0 at .done"

