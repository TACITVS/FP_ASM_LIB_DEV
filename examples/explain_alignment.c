#include <stdio.h>
#include <stdint.h>

int main() {
    uint64_t rsp_start = 0x7FFF8000;
    uint64_t rsp_after_pushes = rsp_start - 24;
    
    printf("Stack Alignment Analysis\n");
    printf("========================\n\n");
    
    printf("After 3 pushes: RSP = 0x%llX (mod 32 = %llu)\n\n", 
           rsp_after_pushes, rsp_after_pushes % 32);
    
    // OLD WAY
    uint64_t rsp_old = rsp_after_pushes - 32;
    uint64_t rsp_old_aligned = rsp_old & 0xFFFFFFFFFFFFFFE0ULL;
    uint64_t rsp_old_final = rsp_old_aligned - 128;
    
    printf("OLD WAY (with sub rsp, 32):\n");
    printf("  After sub: 0x%llX\n", rsp_old);
    printf("  After and: 0x%llX (aligned: %llu)\n", rsp_old_aligned, rsp_old_aligned % 32);
    printf("  Final:     0x%llX\n", rsp_old_final);
    printf("  Space:     %llu bytes\n\n", rsp_after_pushes - rsp_old_final);
    
    // NEW WAY
    uint64_t rsp_new_aligned = rsp_after_pushes & 0xFFFFFFFFFFFFFFE0ULL;
    uint64_t rsp_new_final = rsp_new_aligned - 128;
    
    printf("NEW WAY (without sub rsp, 32):\n");
    printf("  After and: 0x%llX (aligned: %llu)\n", rsp_new_aligned, rsp_new_aligned % 32);
    printf("  Final:     0x%llX\n", rsp_new_final);
    printf("  Space:     %llu bytes\n\n", rsp_after_pushes - rsp_new_final);
    
    printf("Result: Both aligned correctly, NEW saves 32 bytes\n");
    
    return 0;
}
