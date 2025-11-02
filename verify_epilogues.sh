#!/bin/bash

echo "=== VERIFYING FUNCTION EPILOGUES ==="
echo

# Check each function has proper cleanup
for func in fp_reduce_add_i64 fp_reduce_add_f64 fp_reduce_max_i64 fp_reduce_max_f64; do
    echo "Function: $func"
    grep -A15 "^$func:" fp_core_reductions.asm | grep -A10 "^\.done" | head -12
    echo "---"
done
