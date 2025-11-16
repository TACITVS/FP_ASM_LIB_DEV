#!/bin/bash

echo "=== CHECKING REGISTER PRESERVATION ==="
echo

for file in fp_core_reductions.asm fp_core_fused_folds.asm fp_core_fused_maps.asm; do
    echo "File: $file"
    echo "----------------------"
    
    # Check for YMM register saves (vmovdqa [rsp...], ymm)
    echo "YMM saves:"
    grep -n "vmovdqa \[.*\], ymm" "$file" | head -5
    
    # Check for YMM register restores (vmovdqa ymm, [rsp...])
    echo "YMM restores:"
    grep -n "vmovdqa ymm.*, \[.*\]" "$file" | head -5
    
    # Check for vzeroupper
    echo "vzeroupper calls:"
    grep -n "vzeroupper" "$file"
    
    echo
done
