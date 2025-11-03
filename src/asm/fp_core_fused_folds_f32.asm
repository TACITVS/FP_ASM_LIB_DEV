; fp_core_fused_folds_f32.asm
;
; Hand-optimized x64 AVX2 SIMD implementations for f32 fused folds
; Windows x64 calling convention
;
; Performance target: 2-4x faster than gcc -O3 (8x f32 per YMM register)
;
; Functions:
;   - fp_fold_sumsq_f32:  Sum of squares (map square + reduce add)
;   - fp_fold_dotp_f32:   Dot product (zipWith (*) + reduce add) with FMA
;   - fp_fold_sad_f32:    Sum of absolute differences

bits 64
default rel

section .text

; ============================================================================
; fp_fold_sumsq_f32: Sum of squares
; ============================================================================
; Signature: float fp_fold_sumsq_f32(const float* input, size_t n);
;
; Haskell type: foldl (+) 0.0 . map (\x -> x * x) :: [Float] -> Float

global fp_fold_sumsq_f32
fp_fold_sumsq_f32:
    ; Windows x64 ABI: RCX = input, RDX = n
    ; Return: XMM0 = sum of squares

    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Initialize accumulators
    vxorps ymm0, ymm0, ymm0
    vxorps ymm1, ymm1, ymm1
    vxorps ymm2, ymm2, ymm2
    vxorps ymm3, ymm3, ymm3

    mov r12, rcx
    mov rcx, rdx

.loop32:
    cmp rcx, 32
    jb .loop8

    vmovups ymm4, [r12]
    vmovups ymm5, [r12 + 32]
    vmovups ymm6, [r12 + 64]
    vmovups ymm7, [r12 + 96]

    vmulps ymm4, ymm4, ymm4         ; x * x
    vmulps ymm5, ymm5, ymm5
    vmulps ymm6, ymm6, ymm6
    vmulps ymm7, ymm7, ymm7

    vaddps ymm0, ymm0, ymm4
    vaddps ymm1, ymm1, ymm5
    vaddps ymm2, ymm2, ymm6
    vaddps ymm3, ymm3, ymm7

    add r12, 128
    sub rcx, 32
    jmp .loop32

.loop8:
    cmp rcx, 8
    jb .tail

    vmovups ymm4, [r12]
    vmulps ymm4, ymm4, ymm4
    vaddps ymm0, ymm0, ymm4

    add r12, 32
    sub rcx, 8
    jmp .loop8

.tail:
    test rcx, rcx
    jz .horizontal_sum

.tail_loop:
    vmovss xmm4, [r12]
    vmulss xmm4, xmm4, xmm4         ; Scalar square
    vaddss xmm0, xmm0, xmm4

    add r12, 4
    dec rcx
    jnz .tail_loop

.horizontal_sum:
    vaddps ymm0, ymm0, ymm1
    vaddps ymm2, ymm2, ymm3
    vaddps ymm0, ymm0, ymm2

    vextractf128 xmm1, ymm0, 1
    vaddps xmm0, xmm0, xmm1

    vshufps xmm1, xmm0, xmm0, 0x4E
    vaddps xmm0, xmm0, xmm1

    vshufps xmm1, xmm0, xmm0, 0xB1
    vaddps xmm0, xmm0, xmm1

    ; Result already in xmm0

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_fold_dotp_f32: Dot product with FMA
; ============================================================================
; Signature: float fp_fold_dotp_f32(const float* a, const float* b, size_t n);
;
; Haskell type: foldl (+) 0.0 . zipWith (*) :: [Float] -> [Float] -> Float
;
; Uses FMA (Fused Multiply-Add) for maximum performance

global fp_fold_dotp_f32
fp_fold_dotp_f32:
    ; Windows x64 ABI: RCX = a, RDX = b, R8 = n
    ; Return: XMM0 = dot product

    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    vxorps ymm0, ymm0, ymm0
    vxorps ymm1, ymm1, ymm1
    vxorps ymm2, ymm2, ymm2
    vxorps ymm3, ymm3, ymm3

    mov r12, rcx                    ; r12 = a
    mov r13, rdx                    ; r13 = b
    mov rcx, r8                     ; rcx = count

.loop32:
    cmp rcx, 32
    jb .loop8

    vmovups ymm4, [r12]
    vmovups ymm5, [r12 + 32]
    vmovups ymm6, [r12 + 64]
    vmovups ymm7, [r12 + 96]

    vmovups ymm8, [r13]
    vmovups ymm9, [r13 + 32]
    vmovups ymm10, [r13 + 64]
    vmovups ymm11, [r13 + 96]

    ; FMA: acc = (a * b) + acc
    vfmadd231ps ymm0, ymm4, ymm8    ; ymm0 += ymm4 * ymm8
    vfmadd231ps ymm1, ymm5, ymm9
    vfmadd231ps ymm2, ymm6, ymm10
    vfmadd231ps ymm3, ymm7, ymm11

    add r12, 128
    add r13, 128
    sub rcx, 32
    jmp .loop32

.loop8:
    cmp rcx, 8
    jb .tail

    vmovups ymm4, [r12]
    vmovups ymm8, [r13]
    vfmadd231ps ymm0, ymm4, ymm8

    add r12, 32
    add r13, 32
    sub rcx, 8
    jmp .loop8

.tail:
    test rcx, rcx
    jz .horizontal_sum

.tail_loop:
    vmovss xmm4, [r12]
    vmovss xmm8, [r13]
    vfmadd231ss xmm0, xmm4, xmm8    ; Scalar FMA

    add r12, 4
    add r13, 4
    dec rcx
    jnz .tail_loop

.horizontal_sum:
    vaddps ymm0, ymm0, ymm1
    vaddps ymm2, ymm2, ymm3
    vaddps ymm0, ymm0, ymm2

    vextractf128 xmm1, ymm0, 1
    vaddps xmm0, xmm0, xmm1

    vshufps xmm1, xmm0, xmm0, 0x4E
    vaddps xmm0, xmm0, xmm1

    vshufps xmm1, xmm0, xmm0, 0xB1
    vaddps xmm0, xmm0, xmm1

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_fold_sad_f32: Sum of absolute differences
; ============================================================================
; Signature: float fp_fold_sad_f32(const float* a, const float* b, size_t n);
;
; Haskell type: foldl (+) 0.0 . zipWith (\x y -> abs (x - y)) :: [Float] -> [Float] -> Float

global fp_fold_sad_f32
fp_fold_sad_f32:
    ; Windows x64 ABI: RCX = a, RDX = b, R8 = n
    ; Return: XMM0 = sum of absolute differences

    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    vxorps ymm0, ymm0, ymm0
    vxorps ymm1, ymm1, ymm1
    vxorps ymm2, ymm2, ymm2
    vxorps ymm3, ymm3, ymm3

    ; Create absolute value mask (clear sign bit)
    ; 0x7FFFFFFF = all bits except sign bit
    mov eax, 0x7FFFFFFF
    vpxor xmm15, xmm15, xmm15
    vpinsrd xmm15, xmm15, eax, 0
    vpbroadcastd ymm15, xmm15       ; ymm15 = abs mask

    mov r12, rcx
    mov r13, rdx
    mov rcx, r8

.loop32:
    cmp rcx, 32
    jb .loop8

    vmovups ymm4, [r12]
    vmovups ymm5, [r12 + 32]
    vmovups ymm6, [r12 + 64]
    vmovups ymm7, [r12 + 96]

    vmovups ymm8, [r13]
    vmovups ymm9, [r13 + 32]
    vmovups ymm10, [r13 + 64]
    vmovups ymm11, [r13 + 96]

    vsubps ymm4, ymm4, ymm8         ; a - b
    vsubps ymm5, ymm5, ymm9
    vsubps ymm6, ymm6, ymm10
    vsubps ymm7, ymm7, ymm11

    ; Absolute value: AND with mask to clear sign bit
    vandps ymm4, ymm4, ymm15
    vandps ymm5, ymm5, ymm15
    vandps ymm6, ymm6, ymm15
    vandps ymm7, ymm7, ymm15

    vaddps ymm0, ymm0, ymm4
    vaddps ymm1, ymm1, ymm5
    vaddps ymm2, ymm2, ymm6
    vaddps ymm3, ymm3, ymm7

    add r12, 128
    add r13, 128
    sub rcx, 32
    jmp .loop32

.loop8:
    cmp rcx, 8
    jb .tail

    vmovups ymm4, [r12]
    vmovups ymm8, [r13]
    vsubps ymm4, ymm4, ymm8
    vandps ymm4, ymm4, ymm15
    vaddps ymm0, ymm0, ymm4

    add r12, 32
    add r13, 32
    sub rcx, 8
    jmp .loop8

.tail:
    test rcx, rcx
    jz .horizontal_sum

.tail_loop:
    vmovss xmm4, [r12]
    vmovss xmm8, [r13]
    vsubss xmm4, xmm4, xmm8
    vandps xmm4, xmm4, xmm15        ; Scalar absolute value
    vaddss xmm0, xmm0, xmm4

    add r12, 4
    add r13, 4
    dec rcx
    jnz .tail_loop

.horizontal_sum:
    vaddps ymm0, ymm0, ymm1
    vaddps ymm2, ymm2, ymm3
    vaddps ymm0, ymm0, ymm2

    vextractf128 xmm1, ymm0, 1
    vaddps xmm0, xmm0, xmm1

    vshufps xmm1, xmm0, xmm0, 0x4E
    vaddps xmm0, xmm0, xmm1

    vshufps xmm1, xmm0, xmm0, 0xB1
    vaddps xmm0, xmm0, xmm1

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret
