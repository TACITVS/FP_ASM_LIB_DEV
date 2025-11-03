; fp_core_fused_folds_u8.asm
;
; Hand-optimized x64 AVX2 SIMD implementations for u8 fused folds
; Windows x64 calling convention
;
; Performance target: 8x faster than u64 (32x u8 per YMM register!)
;
; CRITICAL LIMITATION: AVX2 has NO vpmullb (no 8-bit multiply instruction)
; Strategy: Scalar for multiply operations, SIMD for SAD
;
; Functions:
;   - fp_fold_sumsq_u8:  Sum of squares (scalar - no SIMD multiply)
;   - fp_fold_dotp_u8:   Dot product (scalar - no SIMD multiply)
;   - fp_fold_sad_u8:    Sum of absolute differences (SIMD - 32-wide!)

bits 64
default rel

section .text

; ============================================================================
; fp_fold_sumsq_u8: Sum of squares
; ============================================================================
; Signature: int8_t fp_fold_sumsq_u8(const int8_t* input, size_t n);
;
; LIMITATION: No vpmullb, must use scalar

global fp_fold_sumsq_u8
fp_fold_sumsq_u8:
    push rbp
    mov rbp, rsp

    mov r12, rcx
    mov rcx, rdx

    xor eax, eax

    test rcx, rcx
    jz .done

.loop:
    movsx r8d, byte [r12]
    imul r8d, r8d
    add eax, r8d
    add r12, 1
    dec rcx
    jnz .loop

.done:
    movsx eax, al
    pop rbp
    ret

; ============================================================================
; fp_fold_dotp_u8: Dot product
; ============================================================================
; Signature: int8_t fp_fold_dotp_u8(const int8_t* a, const int8_t* b, size_t n);
;
; LIMITATION: No vpmullb, must use scalar

global fp_fold_dotp_u8
fp_fold_dotp_u8:
    push rbp
    mov rbp, rsp

    mov r12, rcx
    mov r13, rdx
    mov rcx, r8

    xor eax, eax

    test rcx, rcx
    jz .done

.loop:
    movsx r8d, byte [r12]
    movsx r9d, byte [r13]
    imul r8d, r9d
    add eax, r8d
    add r12, 1
    add r13, 1
    dec rcx
    jnz .loop

.done:
    movsx eax, al
    pop rbp
    ret

; ============================================================================
; fp_fold_sad_u8: Sum of absolute differences
; ============================================================================
; Signature: int8_t fp_fold_sad_u8(const int8_t* a, const int8_t* b, size_t n);
;
; Strategy: Use bitwise absolute value trick
; abs(x) = (x XOR sign_mask) - sign_mask where sign_mask = x >> 7

global fp_fold_sad_u8
fp_fold_sad_u8:
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

.loop128:
    cmp rcx, 128
    jb .loop32

    vmovdqu ymm4, [r12]
    vmovdqu ymm5, [r12 + 32]
    vmovdqu ymm6, [r12 + 64]
    vmovdqu ymm7, [r12 + 96]

    vmovdqu ymm8, [r13]
    vmovdqu ymm9, [r13 + 32]
    vmovdqu ymm10, [r13 + 64]
    vmovdqu ymm11, [r13 + 96]

    ; Compute differences
    vpsubb ymm4, ymm4, ymm8
    vpsubb ymm5, ymm5, ymm9
    vpsubb ymm6, ymm6, ymm10
    vpsubb ymm7, ymm7, ymm11

    ; Absolute value using bitwise trick
    ; sign_mask = diff >> 7 (arithmetic right shift)
    ; abs = (diff XOR sign_mask) - sign_mask

    ; For AVX2, we need to create sign mask by comparing with zero
    vpxor ymm12, ymm12, ymm12
    vpcmpgtb ymm12, ymm12, ymm4
    vpxor ymm4, ymm4, ymm12
    vpsubb ymm4, ymm4, ymm12

    vpxor ymm13, ymm13, ymm13
    vpcmpgtb ymm13, ymm13, ymm5
    vpxor ymm5, ymm5, ymm13
    vpsubb ymm5, ymm5, ymm13

    vpxor ymm14, ymm14, ymm14
    vpcmpgtb ymm14, ymm14, ymm6
    vpxor ymm6, ymm6, ymm14
    vpsubb ymm6, ymm6, ymm14

    vpxor ymm15, ymm15, ymm15
    vpcmpgtb ymm15, ymm15, ymm7
    vpxor ymm7, ymm7, ymm15
    vpsubb ymm7, ymm7, ymm15

    ; Accumulate
    vpaddb ymm0, ymm0, ymm4
    vpaddb ymm1, ymm1, ymm5
    vpaddb ymm2, ymm2, ymm6
    vpaddb ymm3, ymm3, ymm7

    add r12, 128
    add r13, 128
    sub rcx, 128
    jmp .loop128

.loop32:
    cmp rcx, 32
    jb .tail

    vmovdqu ymm4, [r12]
    vmovdqu ymm8, [r13]

    vpsubb ymm4, ymm4, ymm8

    vpxor ymm12, ymm12, ymm12
    vpcmpgtb ymm12, ymm12, ymm4
    vpxor ymm4, ymm4, ymm12
    vpsubb ymm4, ymm4, ymm12

    vpaddb ymm0, ymm0, ymm4

    add r12, 32
    add r13, 32
    sub rcx, 32
    jmp .loop32

.tail:
    test rcx, rcx
    jz .horizontal_sum

.tail_loop:
    movsx eax, byte [r12]
    movsx r9d, byte [r13]
    sub eax, r9d

    ; Scalar absolute value
    mov r10d, eax
    sar r10d, 31
    xor eax, r10d
    sub eax, r10d

    vpxor ymm4, ymm4, ymm4
    vpinsrb xmm4, xmm4, eax, 0
    vpaddb ymm0, ymm0, ymm4

    add r12, 1
    add r13, 1
    dec rcx
    jnz .tail_loop

.horizontal_sum:
    ; Reduce 4 accumulators to 1
    vpaddb ymm0, ymm0, ymm1
    vpaddb ymm2, ymm2, ymm3
    vpaddb ymm0, ymm0, ymm2

    ; For SIGNED u8, sign-extend to i16 then sum
    ; ymm0 has 32 unsigned bytes (absolute values are positive, fit in u8)
    vextracti128 xmm1, ymm0, 1       ; Upper 16 bytes

    ; Sign-extend lower 16 bytes to 16-bit words
    vpmovzxbw ymm2, xmm0             ; xmm0[0:15] -> 16 i16 values in ymm2

    ; Sign-extend upper 16 bytes to 16-bit words
    vpmovzxbw ymm3, xmm1             ; xmm1[0:15] -> 16 i16 values in ymm3

    ; Add the two sets of 16-bit values
    vpaddw ymm2, ymm2, ymm3          ; Now have 16 i16 sums

    ; Horizontal sum of 16 words using phaddw
    vextracti128 xmm1, ymm2, 1
    vpaddw xmm0, xmm2, xmm1          ; 8 words

    vphaddw xmm0, xmm0, xmm0         ; 4 words (pairs summed)
    vphaddw xmm0, xmm0, xmm0         ; 2 words
    vphaddw xmm0, xmm0, xmm0         ; 1 word (at position 0)

    vpextrw eax, xmm0, 0             ; Extract as 16-bit
    movsx eax, ax                    ; Sign extend to 32-bit

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret
