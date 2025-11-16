; fp_core_fused_folds_i16.asm
;
; Hand-optimized x64 AVX2 SIMD implementations for i16 fused folds
; Windows x64 calling convention
;
; Performance target: 4x faster than i64 (16x i16 per YMM register!)
;
; Key advantage: vpmullw provides true SIMD 16-bit multiply!
;
; Functions:
;   - fp_fold_sumsq_i16:  Sum of squares (SIMD multiply available!)
;   - fp_fold_dotp_i16:   Dot product (SIMD multiply available!)
;   - fp_fold_sad_i16:    Sum of absolute differences

bits 64
default rel

%include "macros.inc"

section .text

; ============================================================================
; fp_fold_sumsq_i16: Sum of squares
; ============================================================================
; Signature: int16_t fp_fold_sumsq_i16(const int16_t* input, size_t n);
;
; Strategy: Use vpmullw to square, accumulate with vpaddw
; Note: Results will overflow for large arrays - this is expected for i16

global fp_fold_sumsq_i16
fp_fold_sumsq_i16:
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
    HSUM_I16_YMM 0, 1

    vpextrw r8d, xmm0, 0

.tail_loop:
    movsx eax, word [r12]
    imul eax, eax
    add r8d, eax
    add r12, 2
    dec rcx
    jnz .tail_loop

    movsx eax, r8w
    jmp .epilogue

.horizontal_sum:
    vpaddw ymm0, ymm0, ymm1
    vpaddw ymm2, ymm2, ymm3
    vpaddw ymm0, ymm0, ymm2

    ; Horizontal sum using macro
    HSUM_I16_YMM 0, 1

    vpextrw eax, xmm0, 0
    cwde

.epilogue:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_fold_dotp_i16: Dot product
; ============================================================================
; Signature: int16_t fp_fold_dotp_i16(const int16_t* a, const int16_t* b, size_t n);
;
; Strategy: vpmullw for parallel multiply, vpaddw for accumulation
; This is a HUGE win over i64 which has no SIMD multiply!

global fp_fold_dotp_i16
fp_fold_dotp_i16:
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
    HSUM_I16_YMM 0, 1

    vpextrw r8d, xmm0, 0

.tail_loop:
    movsx eax, word [r12]
    movsx r9d, word [r13]
    imul eax, r9d
    add r8d, eax
    add r12, 2
    add r13, 2
    dec rcx
    jnz .tail_loop

    movsx eax, r8w
    jmp .epilogue

.horizontal_sum:
    vpaddw ymm0, ymm0, ymm1
    vpaddw ymm2, ymm2, ymm3
    vpaddw ymm0, ymm0, ymm2

    ; Horizontal sum using macro
    HSUM_I16_YMM 0, 1

    vpextrw eax, xmm0, 0
    cwde

.epilogue:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_fold_sad_i16: Sum of absolute differences
; ============================================================================
; Signature: int16_t fp_fold_sad_i16(const int16_t* a, const int16_t* b, size_t n);
;
; Strategy:
; 1. Subtract with vpsubw
; 2. Get absolute value using: abs(x) = (x XOR sign_mask) - sign_mask
;    where sign_mask = x >> 15 (arithmetic shift to get all 1s if negative)
; 3. Accumulate with vpaddw

global fp_fold_sad_i16
fp_fold_sad_i16:
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

    ; Compute differences
    vpsubw ymm4, ymm4, ymm8
    vpsubw ymm5, ymm5, ymm9
    vpsubw ymm6, ymm6, ymm10
    vpsubw ymm7, ymm7, ymm11

    ; Absolute value using bitwise trick
    ; sign_mask = diff >> 15 (arithmetic right shift)
    vpsraw ymm12, ymm4, 15
    vpsraw ymm13, ymm5, 15
    vpsraw ymm14, ymm6, 15
    vpsraw ymm15, ymm7, 15

    ; abs = (diff XOR sign_mask) - sign_mask
    vpxor ymm4, ymm4, ymm12
    vpsubw ymm4, ymm4, ymm12

    vpxor ymm5, ymm5, ymm13
    vpsubw ymm5, ymm5, ymm13

    vpxor ymm6, ymm6, ymm14
    vpsubw ymm6, ymm6, ymm14

    vpxor ymm7, ymm7, ymm15
    vpsubw ymm7, ymm7, ymm15

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

    vpsubw ymm4, ymm4, ymm8

    vpsraw ymm12, ymm4, 15
    vpxor ymm4, ymm4, ymm12
    vpsubw ymm4, ymm4, ymm12

    vpaddw ymm0, ymm0, ymm4

    add r12, 32
    add r13, 32
    sub rcx, 16
    jmp .loop16

.tail:
    test rcx, rcx
    jz .horizontal_sum

.tail_loop:
    movsx eax, word [r12]
    movsx r9d, word [r13]
    sub eax, r9d

    ; Scalar absolute value
    mov r10d, eax
    sar r10d, 31
    xor eax, r10d
    sub eax, r10d

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
    HSUM_I16_YMM 0, 1

    vpextrw eax, xmm0, 0
    cwde

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret
