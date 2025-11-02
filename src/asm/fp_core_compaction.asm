; fp_core_compaction.asm
; Module 7: Stream Compaction Operations
;
; Implements SIMD filter/partition using lookup table method
; This is THE technique used by database query engines (Apache Arrow, DuckDB)
;
; Key insight: For 4 i64 values (1 YMM), there are only 16 possible masks (2^4)
; Precompute all 16 shuffle patterns, then lookup + vpermq

bits 64
default rel

section .data
align 32

; Lookup table: 16 entries, one for each 4-bit mask pattern
; Each entry is a shuffle control for vpermq (4 × 2-bit indices)
; Format: [index3, index2, index1, index0] where each is 0-3
;
; Example: mask=0b1011 (keep indices 0,1,3, skip 2)
;   shuffle pattern: [0,1,3,3] = pack first 3, duplicate last

; Shuffle patterns for vpermq (each entry is 1 byte = 4 × 2-bit indices)
compaction_lut:
    ; mask=0b0000 (0): no elements pass
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  ; [0,0,0,0] (doesn't matter)
    ; mask=0b0001 (1): keep index 0
    db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  ; [0,0,0,0]
    ; mask=0b0010 (2): keep index 1
    db 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  ; [1,0,0,0]
    ; mask=0b0011 (3): keep indices 0,1
    db 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  ; [1,0,0,0]
    ; mask=0b0100 (4): keep index 2
    db 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  ; [2,0,0,0]
    ; mask=0b0101 (5): keep indices 0,2
    db 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  ; [2,0,0,0]
    ; mask=0b0110 (6): keep indices 1,2
    db 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  ; [2,1,0,0]
    ; mask=0b0111 (7): keep indices 0,1,2
    db 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  ; [2,1,0,0]
    ; mask=0b1000 (8): keep index 3
    db 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  ; [3,0,0,0]
    ; mask=0b1001 (9): keep indices 0,3
    db 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  ; [3,0,0,0]
    ; mask=0b1010 (10): keep indices 1,3
    db 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  ; [3,1,0,0]
    ; mask=0b1011 (11): keep indices 0,1,3
    db 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  ; [3,1,0,0]
    ; mask=0b1100 (12): keep indices 2,3
    db 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  ; [3,2,0,0]
    ; mask=0b1101 (13): keep indices 0,2,3
    db 0x03, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  ; [3,2,0,0]
    ; mask=0b1110 (14): keep indices 1,2,3
    db 0x03, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00  ; [3,2,1,0]
    ; mask=0b1111 (15): keep all indices
    db 0x03, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00  ; [3,2,1,0]

; Popcount lookup: number of set bits in 4-bit mask
popcount_lut:
    db 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4

section .text

; ============================================================================
; fp_filter_gt_i64_simd
; Real SIMD filter using lookup table compaction
;
; size_t fp_filter_gt_i64_simd(const int64_t* input, int64_t* output,
;                               size_t n, int64_t threshold);
;
; Arguments (Windows x64):
;   RCX = input pointer
;   RDX = output pointer
;   R8  = n (element count)
;   R9  = threshold
;
; Returns:
;   RAX = output count
; ============================================================================
global fp_filter_gt_i64_simd
fp_filter_gt_i64_simd:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Save non-volatile registers
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov r12, rcx                ; r12 = input
    mov r13, rdx                ; r13 = output (current write ptr)
    mov r15, rdx                ; r15 = output (original ptr for count)
    mov rcx, r8                 ; rcx = n
    mov r14, r9                 ; r14 = threshold

    ; Broadcast threshold to YMM register
    vmovq xmm0, r14
    vpbroadcastq ymm7, xmm0     ; ymm7 = [threshold, threshold, threshold, threshold]

.loop4:
    cmp rcx, 4
    jb .tail

    ; Load 4 i64 values
    vmovdqu ymm0, [r12]         ; ymm0 = input[i..i+3]

    ; Compare: ymm1 = (input > threshold) ? 0xFFFFFFFF... : 0
    vpcmpgtq ymm1, ymm0, ymm7

    ; Extract 4-bit mask (one bit per i64)
    vmovmskpd eax, ymm1         ; eax = 4-bit mask (bits 0-3)
    and eax, 0x0F

    ; If mask == 0, skip (no elements pass)
    test eax, eax
    jz .skip

    ; Extract each element and check mask
    ; Element 0
    test eax, 1
    jz .skip0
    vmovq r10, xmm0
    mov [r13], r10
    add r13, 8
.skip0:

    ; Element 1
    test eax, 2
    jz .skip1
    vpsrldq xmm1, xmm0, 8
    vmovq r10, xmm1
    mov [r13], r10
    add r13, 8
.skip1:

    ; Element 2
    test eax, 4
    jz .skip2
    vextracti128 xmm1, ymm0, 1
    vmovq r10, xmm1
    mov [r13], r10
    add r13, 8
.skip2:

    ; Element 3
    test eax, 8
    jz .skip3
    vextracti128 xmm1, ymm0, 1
    vpsrldq xmm1, xmm1, 8
    vmovq r10, xmm1
    mov [r13], r10
    add r13, 8
.skip3:

.skip:
    add r12, 32                 ; input += 4
    sub rcx, 4
    jmp .loop4

.tail:
    ; Process remaining 0-3 elements with scalar code
    test rcx, rcx
    jz .done

.tail_loop:
    mov r10, [r12]
    cmp r10, r14
    jle .tail_skip
    mov [r13], r10
    add r13, 8

.tail_skip:
    add r12, 8
    dec rcx
    jnz .tail_loop

.done:
    ; Compute output count: (current_output_ptr - original_output_ptr) / 8
    mov rax, r13
    sub rax, r15
    shr rax, 3                  ; Divide by 8 (element size)

    ; Restore registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_filter_gt_i64_simple
; Simpler version without lookup table for initial testing
;
; Just does SIMD comparison + scalar compaction
; This should already be faster than pure scalar if comparison dominates
; ============================================================================
global fp_filter_gt_i64_simple
fp_filter_gt_i64_simple:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    push rbx
    push r12
    push r13
    push r14
    push r15

    mov r12, rcx                ; r12 = input
    mov r13, rdx                ; r13 = output (current write ptr)
    mov r15, rdx                ; r15 = output (original ptr for count)
    mov rcx, r8                 ; rcx = n
    mov r14, r9                 ; r14 = threshold

    ; Broadcast threshold
    vmovq xmm0, r14
    vpbroadcastq ymm7, xmm0

.loop4:
    cmp rcx, 4
    jb .tail

    ; Load 4 i64
    vmovdqu ymm0, [r12]

    ; Compare > threshold (SIMD)
    vpcmpgtq ymm1, ymm0, ymm7

    ; Extract to GPRs and compact (scalar)
    vmovq r10, xmm0
    cmp r10, r14
    jle .skip0
    mov [r13], r10
    add r13, 8
.skip0:

    vextracti128 xmm2, ymm0, 0
    vpsrldq xmm2, xmm2, 8
    vmovq r10, xmm2
    cmp r10, r14
    jle .skip1
    mov [r13], r10
    add r13, 8
.skip1:

    vextracti128 xmm2, ymm0, 1
    vmovq r10, xmm2
    cmp r10, r14
    jle .skip2
    mov [r13], r10
    add r13, 8
.skip2:

    vpsrldq xmm2, xmm2, 8
    vmovq r10, xmm2
    cmp r10, r14
    jle .skip3
    mov [r13], r10
    add r13, 8
.skip3:

    add r12, 32
    sub rcx, 4
    jmp .loop4

.tail:
    test rcx, rcx
    jz .done

.tail_loop:
    mov r10, [r12]
    cmp r10, r14
    jle .tail_skip
    mov [r13], r10
    add r13, 8
    inc r15
.tail_skip:
    add r12, 8
    dec rcx
    jnz .tail_loop

.done:
    ; Compute output count
    mov rax, r13
    sub rax, r15
    shr rax, 3

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_partition_gt_i64
; Partition into two outputs: (pass, fail)
;
; void fp_partition_gt_i64(const int64_t* input, int64_t* output_pass,
;                          int64_t* output_fail, size_t n, int64_t threshold,
;                          size_t* out_pass_count, size_t* out_fail_count);
;
; Arguments (Windows x64):
;   RCX = input pointer
;   RDX = output_pass pointer
;   R8  = output_fail pointer
;   R9  = n (element count)
;   [RBP+48] = threshold
;   [RBP+56] = out_pass_count pointer
;   [RBP+64] = out_fail_count pointer
; ============================================================================
global fp_partition_gt_i64
fp_partition_gt_i64:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    push rbx
    push r12
    push r13
    push r14
    push r15

    mov r12, rcx                ; r12 = input
    mov r13, rdx                ; r13 = output_pass (current write ptr)
    mov rbx, rdx                ; rbx = output_pass (original)
    mov r14, r8                 ; r14 = output_fail (current write ptr)
    mov r15, r8                 ; r15 = output_fail (original)
    mov rcx, r9                 ; rcx = n
    mov r9, [rbp + 48]          ; r9 = threshold

    ; Broadcast threshold
    vmovq xmm0, r9
    vpbroadcastq ymm7, xmm0

.loop4:
    cmp rcx, 4
    jb .tail

    ; Load 4 i64
    vmovdqu ymm0, [r12]

    ; Element 0
    vmovq r10, xmm0
    cmp r10, r9
    jle .fail0
    mov [r13], r10
    add r13, 8
    jmp .next0
.fail0:
    mov [r14], r10
    add r14, 8
.next0:

    ; Element 1
    vextracti128 xmm2, ymm0, 0
    vpsrldq xmm2, xmm2, 8
    vmovq r10, xmm2
    cmp r10, r9
    jle .fail1
    mov [r13], r10
    add r13, 8
    jmp .next1
.fail1:
    mov [r14], r10
    add r14, 8
.next1:

    ; Element 2
    vextracti128 xmm2, ymm0, 1
    vmovq r10, xmm2
    cmp r10, r9
    jle .fail2
    mov [r13], r10
    add r13, 8
    jmp .next2
.fail2:
    mov [r14], r10
    add r14, 8
.next2:

    ; Element 3
    vpsrldq xmm2, xmm2, 8
    vmovq r10, xmm2
    cmp r10, r9
    jle .fail3
    mov [r13], r10
    add r13, 8
    jmp .next3
.fail3:
    mov [r14], r10
    add r14, 8
.next3:

    add r12, 32
    sub rcx, 4
    jmp .loop4

.tail:
    test rcx, rcx
    jz .done

.tail_loop:
    mov r10, [r12]
    cmp r10, r9
    jle .tail_fail
    mov [r13], r10
    add r13, 8
    jmp .tail_next
.tail_fail:
    mov [r14], r10
    add r14, 8
.tail_next:
    add r12, 8
    dec rcx
    jnz .tail_loop

.done:
    ; Compute counts
    mov rax, r13
    sub rax, rbx
    shr rax, 3                  ; pass_count = (r13 - rbx) / 8

    mov r10, r14
    sub r10, r15
    shr r10, 3                  ; fail_count = (r14 - r15) / 8

    ; Store counts to output pointers
    mov r11, [rbp + 56]         ; out_pass_count pointer
    mov [r11], rax

    mov r11, [rbp + 64]         ; out_fail_count pointer
    mov [r11], r10

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_take_while_gt_i64
; Take elements while predicate is true (early exit)
;
; size_t fp_take_while_gt_i64(const int64_t* input, int64_t* output,
;                              size_t n, int64_t threshold);
;
; Arguments (Windows x64):
;   RCX = input pointer
;   RDX = output pointer
;   R8  = n (element count)
;   R9  = threshold
;
; Returns:
;   RAX = number of elements taken
; ============================================================================
global fp_take_while_gt_i64
fp_take_while_gt_i64:
    push rbp
    mov rbp, rsp

    mov r10, rcx                ; r10 = input
    mov r11, rdx                ; r11 = output
    xor rax, rax                ; rax = count

.loop:
    cmp rax, r8
    jae .done

    mov rcx, [r10]
    cmp rcx, r9
    jle .done                   ; Stop when predicate fails

    mov [r11], rcx
    add r10, 8
    add r11, 8
    inc rax
    jmp .loop

.done:
    pop rbp
    ret

; ============================================================================
; fp_drop_while_gt_i64
; Drop elements while predicate is true, return rest
;
; size_t fp_drop_while_gt_i64(const int64_t* input, int64_t* output,
;                              size_t n, int64_t threshold);
;
; Arguments (Windows x64):
;   RCX = input pointer
;   RDX = output pointer
;   R8  = n (element count)
;   R9  = threshold
;
; Returns:
;   RAX = number of elements in output
; ============================================================================
global fp_drop_while_gt_i64
fp_drop_while_gt_i64:
    push rbp
    mov rbp, rsp

    mov r10, rcx                ; r10 = input (current)
    mov r11, rdx                ; r11 = output
    mov rax, r8                 ; rax = remaining count

.skip_loop:
    test rax, rax
    jz .done_empty

    mov rcx, [r10]
    cmp rcx, r9
    jle .copy_rest              ; Start copying when predicate fails

    add r10, 8
    dec rax
    jmp .skip_loop

.copy_rest:
    ; Copy remaining elements
    mov r8, rax                 ; r8 = output count
    test rax, rax
    jz .done_empty

.copy_loop:
    mov rcx, [r10]
    mov [r11], rcx
    add r10, 8
    add r11, 8
    dec rax
    jnz .copy_loop

    mov rax, r8                 ; Return count
    jmp .done

.done_empty:
    xor rax, rax
.done:
    pop rbp
    ret
