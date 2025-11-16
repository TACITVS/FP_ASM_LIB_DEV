; fp_core_fused_folds_u16.asm
;
; Hand-optimized x64 AVX2 SIMD implementations for u16 fused folds
; Windows x64 calling convention
;
; Performance target: 4x faster than u64 (16x u16 per YMM register!)
;
; Key advantage: vpmullw provides true SIMD 16-bit multiply!
;
; Functions:
;   - fp_fold_sumsq_u16:  Sum of squares (SIMD multiply available!)
;   - fp_fold_dotp_u16:   Dot product (SIMD multiply available!)
;   - fp_fold_sad_u16:    Sum of absolute differences (unsigned)

bits 64
default rel

%include "macros.inc"

section .text

; ============================================================================
; fp_fold_sumsq_u16: Sum of squares
; ============================================================================
; Signature: uint16_t fp_fold_sumsq_u16(const uint16_t* input, size_t n);
;
; Strategy: Use vpmullw to square, accumulate with vpaddw

global fp_fold_sumsq_u16
fp_fold_sumsq_u16:
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

.loop64:
    cmp rcx, 64
    jb .loop16

    vmovdqu ymm4, [r12]
    vmovdqu ymm5, [r12 + 32]
    vmovdqu ymm6, [r12 + 64]
    vmovdqu ymm7, [r12 + 96]

    vpmullw ymm8, ymm4, ymm4
    vpmullw ymm9, ymm5, ymm5
    vpmullw ymm10, ymm6, ymm6
    vpmullw ymm11, ymm7, ymm7

    vpaddw ymm0, ymm0, ymm8
    vpaddw ymm1, ymm1, ymm9
    vpaddw ymm2, ymm2, ymm10
    vpaddw ymm3, ymm3, ymm11

    add r12, 128
    sub rcx, 64
    jmp .loop64

.loop16:
    cmp rcx, 16
    jb .tail

    vmovdqu ymm4, [r12]
    vpmullw ymm4, ymm4, ymm4
    vpaddw ymm0, ymm0, ymm4

    add r12, 32
    sub rcx, 16
    jmp .loop16

.tail:
    test rcx, rcx
    jz .horizontal_sum

    ; Reduce accumulators before tail
    vpaddw ymm0, ymm0, ymm1
    vpaddw ymm2, ymm2, ymm3
    vpaddw ymm0, ymm0, ymm2

    ; Horizontal sum using macro
    HSUM_U16_YMM 0, 1

    vpextrw r8d, xmm0, 0

.tail_loop:
    movzx eax, word [r12]
    imul eax, eax
    add r8d, eax
    add r12, 2
    dec rcx
    jnz .tail_loop

    mov eax, r8d
    and eax, 0xFFFF
    jmp .epilogue

.horizontal_sum:
    vpaddw ymm0, ymm0, ymm1
    vpaddw ymm2, ymm2, ymm3
    vpaddw ymm0, ymm0, ymm2

    ; Horizontal sum using macro
    HSUM_U16_YMM 0, 1

    vpextrw eax, xmm0, 0

.epilogue:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_fold_dotp_u16: Dot product
; ============================================================================
; Signature: uint16_t fp_fold_dotp_u16(const uint16_t* a, const uint16_t* b, size_t n);
;
; Strategy: vpmullw for parallel multiply, vpaddw for accumulation

global fp_fold_dotp_u16
fp_fold_dotp_u16:
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

.loop64:
    cmp rcx, 64
    jb .loop16

    vmovdqu ymm4, [r12]
    vmovdqu ymm5, [r12 + 32]
    vmovdqu ymm6, [r12 + 64]
    vmovdqu ymm7, [r12 + 96]

    vmovdqu ymm8, [r13]
    vmovdqu ymm9, [r13 + 32]
    vmovdqu ymm10, [r13 + 64]
    vmovdqu ymm11, [r13 + 96]

    vpmullw ymm4, ymm4, ymm8
    vpmullw ymm5, ymm5, ymm9
    vpmullw ymm6, ymm6, ymm10
    vpmullw ymm7, ymm7, ymm11

    vpaddw ymm0, ymm0, ymm4
    vpaddw ymm1, ymm1, ymm5
    vpaddw ymm2, ymm2, ymm6
    vpaddw ymm3, ymm3, ymm7

    add r12, 128
    add r13, 128
    sub rcx, 64
    jmp .loop64

.loop16:
    cmp rcx, 16
    jb .tail

    vmovdqu ymm4, [r12]
    vmovdqu ymm8, [r13]
    vpmullw ymm4, ymm4, ymm8
    vpaddw ymm0, ymm0, ymm4

    add r12, 32
    add r13, 32
    sub rcx, 16
    jmp .loop16

.tail:
    test rcx, rcx
    jz .horizontal_sum

    ; Reduce accumulators before tail
    vpaddw ymm0, ymm0, ymm1
    vpaddw ymm2, ymm2, ymm3
    vpaddw ymm0, ymm0, ymm2

    ; Horizontal sum using macro
    HSUM_U16_YMM 0, 1

    vpextrw r8d, xmm0, 0

.tail_loop:
    movzx eax, word [r12]
    movzx r9d, word [r13]
    imul eax, r9d
    add r8d, eax
    add r12, 2
    add r13, 2
    dec rcx
    jnz .tail_loop

    mov eax, r8d
    and eax, 0xFFFF
    jmp .epilogue

.horizontal_sum:
    vpaddw ymm0, ymm0, ymm1
    vpaddw ymm2, ymm2, ymm3
    vpaddw ymm0, ymm0, ymm2

    ; Horizontal sum using macro
    HSUM_U16_YMM 0, 1

    vpextrw eax, xmm0, 0

.epilogue:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_fold_sad_u16: Sum of absolute differences (unsigned)
; ============================================================================
; Signature: uint16_t fp_fold_sad_u16(const uint16_t* a, const uint16_t* b, size_t n);
;
; Strategy for unsigned: |a - b| = max(a,b) - min(a,b)

global fp_fold_sad_u16
fp_fold_sad_u16:
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

.loop64:
    cmp rcx, 64
    jb .loop16

    vmovdqu ymm4, [r12]
    vmovdqu ymm5, [r12 + 32]
    vmovdqu ymm6, [r12 + 64]
    vmovdqu ymm7, [r12 + 96]

    vmovdqu ymm8, [r13]
    vmovdqu ymm9, [r13 + 32]
    vmovdqu ymm10, [r13 + 64]
    vmovdqu ymm11, [r13 + 96]

    ; Compute unsigned absolute difference: max(a,b) - min(a,b)
    vpmaxuw ymm12, ymm4, ymm8
    vpmaxuw ymm13, ymm5, ymm9
    vpmaxuw ymm14, ymm6, ymm10
    vpmaxuw ymm15, ymm7, ymm11

    vpminuw ymm4, ymm4, ymm8
    vpminuw ymm5, ymm5, ymm9
    vpminuw ymm6, ymm6, ymm10
    vpminuw ymm7, ymm7, ymm11

    vpsubw ymm4, ymm12, ymm4
    vpsubw ymm5, ymm13, ymm5
    vpsubw ymm6, ymm14, ymm6
    vpsubw ymm7, ymm15, ymm7

    ; Accumulate
    vpaddw ymm0, ymm0, ymm4
    vpaddw ymm1, ymm1, ymm5
    vpaddw ymm2, ymm2, ymm6
    vpaddw ymm3, ymm3, ymm7

    add r12, 128
    add r13, 128
    sub rcx, 64
    jmp .loop64

.loop16:
    cmp rcx, 16
    jb .tail

    vmovdqu ymm4, [r12]
    vmovdqu ymm8, [r13]

    vpmaxuw ymm12, ymm4, ymm8
    vpminuw ymm4, ymm4, ymm8
    vpsubw ymm4, ymm12, ymm4

    vpaddw ymm0, ymm0, ymm4

    add r12, 32
    add r13, 32
    sub rcx, 16
    jmp .loop16

.tail:
    test rcx, rcx
    jz .horizontal_sum

.tail_loop:
    movzx eax, word [r12]
    movzx r9d, word [r13]

    ; Compute unsigned absolute difference
    cmp eax, r9d
    jae .a_greater_equal
    sub r9d, eax
    mov eax, r9d
    jmp .add_diff
.a_greater_equal:
    sub eax, r9d
.add_diff:

    vpxor ymm4, ymm4, ymm4
    vpinsrw xmm4, xmm4, eax, 0
    vpaddw ymm0, ymm0, ymm4

    add r12, 2
    add r13, 2
    dec rcx
    jnz .tail_loop

.horizontal_sum:
    vpaddw ymm0, ymm0, ymm1
    vpaddw ymm2, ymm2, ymm3
    vpaddw ymm0, ymm0, ymm2

    ; Horizontal sum using macro
    HSUM_U16_YMM 0, 1

    vpextrw eax, xmm0, 0

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret
