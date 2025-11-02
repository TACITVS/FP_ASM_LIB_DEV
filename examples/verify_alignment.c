#include <stdio.h>
#include <stdint.h>

void verify_pattern_1() {
    printf("=== PATTERN 1: push r11/r12/r13, mov r11, rsp ===\n");
    
    // Assume function entry with 16-byte aligned stack (RSP % 16 == 8 after CALL)
    uint64_t rsp_entry = 0x7FFFFFF8;  // Example: RSP % 16 == 8
    printf("After CALL (return address pushed): RSP = 0x%llX, RSP %% 16 = %llu\n", 
           rsp_entry, rsp_entry % 16);
    
    // After 3 pushes (24 bytes)
    uint64_t rsp_after_pushes = rsp_entry - 24;
    printf("After push r11/r12/r13:             RSP = 0x%llX, RSP %% 32 = %llu\n", 
           rsp_after_pushes, rsp_after_pushes % 32);
    
    // After alignment
    uint64_t rsp_aligned = rsp_after_pushes & 0xFFFFFFFFFFFFFFE0ULL;
    printf("After and rsp, 0xFFFFFFFFFFFFFFE0:  RSP = 0x%llX, RSP %% 32 = %llu ✓\n", 
           rsp_aligned, rsp_aligned % 32);
    
    // After space allocation
    uint64_t rsp_final = rsp_aligned - 128;
    printf("After sub rsp, 128:                 RSP = 0x%llX, RSP %% 32 = %llu ✓\n\n", 
           rsp_final, rsp_final % 32);
    
    if (rsp_final % 32 != 0) {
        printf("ERROR: Final RSP not 32-byte aligned!\n");
    }
}

void verify_pattern_2() {
    printf("=== PATTERN 2: push rbp, mov rbp, rsp ===\n");
    
    uint64_t rsp_entry = 0x7FFFFFF8;
    printf("After CALL:                         RSP = 0x%llX, RSP %% 16 = %llu\n", 
           rsp_entry, rsp_entry % 16);
    
    // After 1 push (8 bytes)
    uint64_t rsp_after_push = rsp_entry - 8;
    printf("After push rbp:                     RSP = 0x%llX, RSP %% 32 = %llu\n", 
           rsp_after_push, rsp_after_push % 32);
    
    // After alignment
    uint64_t rsp_aligned = rsp_after_push & 0xFFFFFFFFFFFFFFE0ULL;
    printf("After and rsp, -32:                 RSP = 0x%llX, RSP %% 32 = %llu ✓\n", 
           rsp_aligned, rsp_aligned % 32);
    
    // After space allocation
    uint64_t rsp_final = rsp_aligned - 32;
    printf("After sub rsp, 32:                  RSP = 0x%llX, RSP %% 32 = %llu ✓\n\n", 
           rsp_final, rsp_final % 32);
    
    if (rsp_final % 32 != 0) {
        printf("ERROR: Final RSP not 32-byte aligned!\n");
    }
}

int main() {
    printf("Stack Alignment Verification\n");
    printf("============================\n\n");
    
    verify_pattern_1();
    verify_pattern_2();
    
    printf("=== VERIFICATION COMPLETE ===\n");
    printf("Both patterns guarantee 32-byte alignment for vmovdqa instructions.\n");
    
    return 0;
}
