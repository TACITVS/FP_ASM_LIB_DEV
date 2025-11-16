; fp_core_fused_folds_u32.asm
;
; Hand-optimized x64 AVX2 SIMD implementations for u32 fused folds
; Windows x64 calling convention
;
; Performance target: 2-4x faster than gcc -O3 (8x u32 per YMM register)
;
; Functions:
;   - fp_fold_sumsq_u32:  Sum of squares (map square + reduce add)
;   - fp_fold_dotp_u32:   Dot product (map multiply + reduce add)
;   - fp_fold_sad_u32:    Sum of absolute differences

bits 64
default rel

%include "macros.inc"

section .text

; ============================================================================
; fp_fold_sumsq_u32: Sum of squares
; ============================================================================
; Signature: uint32_t fp_fold_sumsq_u32(const uint32_t* input, size_t n);
;
; Same as i32 - squaring is identical for signed and unsigned

global fp_fold_sumsq_u32
fp_fold_sumsq_u32:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    vpxor ymm0, ymm0, ymm0
    vpxor ymm1, ymm1, ymm1
    vpxor ymm2, ymm2, ymm2
    vpxor ymm3, ymm3, ymm3

    mov r12, rcx
    mov rcx, rdx

.loop32:
    cmp rcx, 32
    jb .loop8

    vmovdqu ymm4, [r12]
    vmovdqu ymm5, [r12 + 32]
    vmovdqu ymm6, [r12 + 64]
    vmovdqu ymm7, [r12 + 96]

    vpmulld ymm8, ymm4, ymm4
    vpmulld ymm9, ymm5, ymm5
    vpmulld ymm10, ymm6, ymm6
    vpmulld ymm11, ymm7, ymm7

    vpaddd ymm0, ymm0, ymm8
    vpaddd ymm1, ymm1, ymm9
    vpaddd ymm2, ymm2, ymm10
    vpaddd ymm3, ymm3, ymm11

    add r12, 128
    sub rcx, 32
    jmp .loop32

.loop8:
    cmp rcx, 8
    jb .tail

    vmovdqu ymm4, [r12]
    vpmulld ymm4, ymm4, ymm4
    vpaddd ymm0, ymm0, ymm4

    add r12, 32
    sub rcx, 8
    jmp .loop8

.tail:
    test rcx, rcx
    jz .horizontal_sum

    ; Reduce accumulators first
    vpaddd ymm0, ymm0, ymm1
    vpaddd ymm2, ymm2, ymm3
    vpaddd ymm0, ymm0, ymm2

    ; Horizontal sum using macro
    HSUM_U32_YMM 0, 1

    vmovd r8d, xmm0

.tail_loop:
    mov eax, [r12]
    imul eax, eax
    add r8d, eax
    add r12, 4
    dec rcx
    jnz .tail_loop

    mov eax, r8d
    jmp .epilogue

.horizontal_sum:
    vpaddd ymm0, ymm0, ymm1
    vpaddd ymm2, ymm2, ymm3
    vpaddd ymm0, ymm0, ymm2

    ; Horizontal sum using macro
    HSUM_U32_YMM 0, 1

    vmovd eax, xmm0

.epilogue:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_fold_dotp_u32: Dot product
; ============================================================================
; Signature: uint32_t fp_fold_dotp_u32(const uint32_t* a, const uint32_t* b, size_t n);
;
; Same as i32 - multiplication is identical

global fp_fold_dotp_u32
fp_fold_dotp_u32:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    vpxor ymm0, ymm0, ymm0
    vpxor ymm1, ymm1, ymm1
    vpxor ymm2, ymm2, ymm2
    vpxor ymm3, ymm3, ymm3

    mov r12, rcx
    mov r13, rdx
    mov rcx, r8

.loop32:
    cmp rcx, 32
    jb .loop8

    vmovdqu ymm4, [r12]
    vmovdqu ymm5, [r12 + 32]
    vmovdqu ymm6, [r12 + 64]
    vmovdqu ymm7, [r12 + 96]

    vmovdqu ymm8, [r13]
    vmovdqu ymm9, [r13 + 32]
    vmovdqu ymm10, [r13 + 64]
    vmovdqu ymm11, [r13 + 96]

    vpmulld ymm4, ymm4, ymm8
    vpmulld ymm5, ymm5, ymm9
    vpmulld ymm6, ymm6, ymm10
    vpmulld ymm7, ymm7, ymm11

    vpaddd ymm0, ymm0, ymm4
    vpaddd ymm1, ymm1, ymm5
    vpaddd ymm2, ymm2, ymm6
    vpaddd ymm3, ymm3, ymm7

    add r12, 128
    add r13, 128
    sub rcx, 32
    jmp .loop32

.loop8:
    cmp rcx, 8
    jb .tail

    vmovdqu ymm4, [r12]
    vmovdqu ymm8, [r13]
    vpmulld ymm4, ymm4, ymm8
    vpaddd ymm0, ymm0, ymm4

    add r12, 32
    add r13, 32
    sub rcx, 8
    jmp .loop8

.tail:
    test rcx, rcx
    jz .horizontal_sum

    ; Reduce accumulators first
    vpaddd ymm0, ymm0, ymm1
    vpaddd ymm2, ymm2, ymm3
    vpaddd ymm0, ymm0, ymm2

    ; Horizontal sum using macro
    HSUM_U32_YMM 0, 1

    vmovd r8d, xmm0

.tail_loop:
    mov eax, [r12]
    mov r9d, [r13]
    imul eax, r9d
    add r8d, eax
    add r12, 4
    add r13, 4
    dec rcx
    jnz .tail_loop

    mov eax, r8d
    jmp .epilogue

.horizontal_sum:
    vpaddd ymm0, ymm0, ymm1
    vpaddd ymm2, ymm2, ymm3
    vpaddd ymm0, ymm0, ymm2

    ; Horizontal sum using macro
    HSUM_U32_YMM 0, 1

    vmovd eax, xmm0

.epilogue:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_fold_sad_u32: Sum of absolute differences
; ============================================================================
; Signature: uint32_t fp_fold_sad_u32(const uint32_t* a, const uint32_t* b, size_t n);
;
; For unsigned, we need to compute |a - b| where both are unsigned.
; This requires checking which is larger first.

global fp_fold_sad_u32
fp_fold_sad_u32:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    vpxor ymm0, ymm0, ymm0
    vpxor ymm1, ymm1, ymm1
    vpxor ymm2, ymm2, ymm2
    vpxor ymm3, ymm3, ymm3

    mov r12, rcx
    mov r13, rdx
    mov rcx, r8

.loop32:
    cmp rcx, 32
    jb .loop8

    vmovdqu ymm4, [r12]
    vmovdqu ymm5, [r12 + 32]
    vmovdqu ymm6, [r12 + 64]
    vmovdqu ymm7, [r12 + 96]

    vmovdqu ymm8, [r13]
    vmovdqu ymm9, [r13 + 32]
    vmovdqu ymm10, [r13 + 64]
    vmovdqu ymm11, [r13 + 96]

    ; Compute unsigned absolute difference: max(a,b) - min(a,b)
    vpmaxud ymm12, ymm4, ymm8
    vpminud ymm13, ymm4, ymm8
    vpsubd ymm4, ymm12, ymm13

    vpmaxud ymm12, ymm5, ymm9
    vpminud ymm13, ymm5, ymm9
    vpsubd ymm5, ymm12, ymm13

    vpmaxud ymm12, ymm6, ymm10
    vpminud ymm13, ymm6, ymm10
    vpsubd ymm6, ymm12, ymm13

    vpmaxud ymm12, ymm7, ymm11
    vpminud ymm13, ymm7, ymm11
    vpsubd ymm7, ymm12, ymm13

    vpaddd ymm0, ymm0, ymm4
    vpaddd ymm1, ymm1, ymm5
    vpaddd ymm2, ymm2, ymm6
    vpaddd ymm3, ymm3, ymm7

    add r12, 128
    add r13, 128
    sub rcx, 32
    jmp .loop32

.loop8:
    cmp rcx, 8
    jb .tail

    vmovdqu ymm4, [r12]
    vmovdqu ymm8, [r13]

    vpmaxud ymm12, ymm4, ymm8
    vpminud ymm13, ymm4, ymm8
    vpsubd ymm4, ymm12, ymm13

    vpaddd ymm0, ymm0, ymm4

    add r12, 32
    add r13, 32
    sub rcx, 8
    jmp .loop8

.tail:
    test rcx, rcx
    jz .horizontal_sum

.tail_loop:
    mov eax, [r12]
    mov r9d, [r13]

    ; Compute unsigned |a - b|
    cmp eax, r9d
    jae .a_greater_equal
    sub r9d, eax
    vpxor ymm4, ymm4, ymm4
    vpinsrd xmm4, xmm4, r9d, 0
    vpaddd ymm0, ymm0, ymm4
    jmp .next_tail
.a_greater_equal:
    sub eax, r9d
    vpxor ymm4, ymm4, ymm4
    vpinsrd xmm4, xmm4, eax, 0
    vpaddd ymm0, ymm0, ymm4

.next_tail:
    add r12, 4
    add r13, 4
    dec rcx
    jnz .tail_loop

.horizontal_sum:
    vpaddd ymm0, ymm0, ymm1
    vpaddd ymm2, ymm2, ymm3
    vpaddd ymm0, ymm0, ymm2

    ; Horizontal sum using macro
    HSUM_U32_YMM 0, 1

    vmovd eax, xmm0

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret
