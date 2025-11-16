; fp_core_fused_folds_i32.asm
;
; Hand-optimized x64 AVX2 SIMD implementations for i32 fused folds
; Windows x64 calling convention
;
; Performance target: 2-4x faster than gcc -O3 (8x i32 per YMM register)
;
; Functions:
;   - fp_fold_sumsq_i32:  Sum of squares (map square + reduce add)
;   - fp_fold_dotp_i32:   Dot product (zipWith (*) + reduce add)
;   - fp_fold_sad_i32:    Sum of absolute differences

bits 64
default rel

%include "macros.inc"

section .text

; ============================================================================
; fp_fold_sumsq_i32: Sum of squares
; ============================================================================
; Signature: int32_t fp_fold_sumsq_i32(const int32_t* input, size_t n);
;
; Haskell type: foldl (+) 0 . map (\x -> x * x) :: [Int32] -> Int32
;
; Fused operation: Combines map (square) and reduce (sum) in single pass
; Avoids materializing intermediate array

global fp_fold_sumsq_i32
fp_fold_sumsq_i32:
    ; Windows x64 ABI: RCX = input, RDX = n
    ; Return: EAX = sum of squares

    ; Prologue
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Initialize accumulators
    vpxor ymm0, ymm0, ymm0          ; acc1 = 0
    vpxor ymm1, ymm1, ymm1          ; acc2 = 0
    vpxor ymm2, ymm2, ymm2          ; acc3 = 0
    vpxor ymm3, ymm3, ymm3          ; acc4 = 0

    mov r12, rcx                    ; r12 = input pointer
    mov rcx, rdx                    ; rcx = count

    ; Main loop: 32 elements per iteration
.loop32:
    cmp rcx, 32
    jb .loop8

    vmovdqu ymm4, [r12]             ; Load 8x i32
    vmovdqu ymm5, [r12 + 32]
    vmovdqu ymm6, [r12 + 64]
    vmovdqu ymm7, [r12 + 96]

    vpmulld ymm4, ymm4, ymm4        ; Square (x * x)
    vpmulld ymm5, ymm5, ymm5
    vpmulld ymm6, ymm6, ymm6
    vpmulld ymm7, ymm7, ymm7

    vpaddd ymm0, ymm0, ymm4         ; Accumulate
    vpaddd ymm1, ymm1, ymm5
    vpaddd ymm2, ymm2, ymm6
    vpaddd ymm3, ymm3, ymm7

    add r12, 128
    sub rcx, 32
    jmp .loop32

.loop8:
    cmp rcx, 8
    jb .tail

    vmovdqu ymm4, [r12]
    vpmulld ymm4, ymm4, ymm4        ; Square
    vpaddd ymm0, ymm0, ymm4         ; Accumulate

    add r12, 32
    sub rcx, 8
    jmp .loop8

.tail:
    test rcx, rcx
    jz .horizontal_sum

.tail_loop:
    mov eax, [r12]                  ; Load value
    imul eax, eax                   ; Square (scalar)
    vpxor ymm4, ymm4, ymm4
    vpinsrd xmm4, xmm4, eax, 0
    vpaddd ymm0, ymm0, ymm4         ; Add to accumulator

    add r12, 4
    dec rcx
    jnz .tail_loop

.horizontal_sum:
    ; Sum all 4 accumulators
    vpaddd ymm0, ymm0, ymm1
    vpaddd ymm2, ymm2, ymm3
    vpaddd ymm0, ymm0, ymm2

    ; Horizontal sum using macro
    HSUM_I32_YMM 0, 1

    vmovd eax, xmm0

    ; Epilogue
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_fold_dotp_i32: Dot product
; ============================================================================
; Signature: int32_t fp_fold_dotp_i32(const int32_t* a, const int32_t* b, size_t n);
;
; Haskell type: foldl (+) 0 . zipWith (*) :: [Int32] -> [Int32] -> Int32
;
; Fused operation: Combines zipWith (multiply) and reduce (sum)

global fp_fold_dotp_i32
fp_fold_dotp_i32:
    ; Windows x64 ABI: RCX = a, RDX = b, R8 = n
    ; Return: EAX = dot product

    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Initialize accumulators
    vpxor ymm0, ymm0, ymm0
    vpxor ymm1, ymm1, ymm1
    vpxor ymm2, ymm2, ymm2
    vpxor ymm3, ymm3, ymm3

    mov r12, rcx                    ; r12 = a
    mov r13, rdx                    ; r13 = b
    mov rcx, r8                     ; rcx = count

.loop32:
    cmp rcx, 32
    jb .loop8

    vmovdqu ymm4, [r12]             ; Load a[i..i+7]
    vmovdqu ymm5, [r12 + 32]
    vmovdqu ymm6, [r12 + 64]
    vmovdqu ymm7, [r12 + 96]

    vmovdqu ymm8, [r13]             ; Load b[i..i+7]
    vmovdqu ymm9, [r13 + 32]
    vmovdqu ymm10, [r13 + 64]
    vmovdqu ymm11, [r13 + 96]

    vpmulld ymm4, ymm4, ymm8        ; a[i] * b[i]
    vpmulld ymm5, ymm5, ymm9
    vpmulld ymm6, ymm6, ymm10
    vpmulld ymm7, ymm7, ymm11

    vpaddd ymm0, ymm0, ymm4         ; Accumulate
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

.tail_loop:
    mov eax, [r12]                  ; Load a[i]
    imul eax, [r13]                 ; a[i] * b[i] (scalar)
    vpxor ymm4, ymm4, ymm4
    vpinsrd xmm4, xmm4, eax, 0
    vpaddd ymm0, ymm0, ymm4

    add r12, 4
    add r13, 4
    dec rcx
    jnz .tail_loop

.horizontal_sum:
    vpaddd ymm0, ymm0, ymm1
    vpaddd ymm2, ymm2, ymm3
    vpaddd ymm0, ymm0, ymm2

    vextracti128 xmm1, ymm0, 1
    vpaddd xmm0, xmm0, xmm1

    vpshufd xmm1, xmm0, 0x4E
    vpaddd xmm0, xmm0, xmm1

    vpshufd xmm1, xmm0, 0xB1
    vpaddd xmm0, xmm0, xmm1

    vmovd eax, xmm0

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_fold_sad_i32: Sum of absolute differences
; ============================================================================
; Signature: int32_t fp_fold_sad_i32(const int32_t* a, const int32_t* b, size_t n);
;
; Haskell type: foldl (+) 0 . zipWith (\x y -> abs (x - y)) :: [Int32] -> [Int32] -> Int32
;
; Fused operation: Combines zipWith (abs difference) and reduce (sum)

global fp_fold_sad_i32
fp_fold_sad_i32:
    ; Windows x64 ABI: RCX = a, RDX = b, R8 = n
    ; Return: EAX = sum of absolute differences

    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Initialize accumulators
    vpxor ymm0, ymm0, ymm0
    vpxor ymm1, ymm1, ymm1
    vpxor ymm2, ymm2, ymm2
    vpxor ymm3, ymm3, ymm3

    mov r12, rcx                    ; r12 = a
    mov r13, rdx                    ; r13 = b
    mov rcx, r8                     ; rcx = count

.loop32:
    cmp rcx, 32
    jb .loop8

    vmovdqu ymm4, [r12]             ; Load a[i..i+7]
    vmovdqu ymm5, [r12 + 32]
    vmovdqu ymm6, [r12 + 64]
    vmovdqu ymm7, [r12 + 96]

    vmovdqu ymm8, [r13]             ; Load b[i..i+7]
    vmovdqu ymm9, [r13 + 32]
    vmovdqu ymm10, [r13 + 64]
    vmovdqu ymm11, [r13 + 96]

    vpsubd ymm4, ymm4, ymm8         ; a[i] - b[i]
    vpsubd ymm5, ymm5, ymm9
    vpsubd ymm6, ymm6, ymm10
    vpsubd ymm7, ymm7, ymm11

    ; Absolute value using bitwise trick: abs(x) = (x XOR mask) - mask, where mask = x >> 31
    vpxor ymm12, ymm12, ymm12
    vpcmpgtd ymm12, ymm12, ymm4     ; mask = (0 > x) ? 0xFFFFFFFF : 0
    vpxor ymm4, ymm4, ymm12
    vpsubd ymm4, ymm4, ymm12

    vpxor ymm12, ymm12, ymm12
    vpcmpgtd ymm12, ymm12, ymm5
    vpxor ymm5, ymm5, ymm12
    vpsubd ymm5, ymm5, ymm12

    vpxor ymm12, ymm12, ymm12
    vpcmpgtd ymm12, ymm12, ymm6
    vpxor ymm6, ymm6, ymm12
    vpsubd ymm6, ymm6, ymm12

    vpxor ymm12, ymm12, ymm12
    vpcmpgtd ymm12, ymm12, ymm7
    vpxor ymm7, ymm7, ymm12
    vpsubd ymm7, ymm7, ymm12

    vpaddd ymm0, ymm0, ymm4         ; Accumulate
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
    vpsubd ymm4, ymm4, ymm8         ; a - b

    ; Absolute value
    vpxor ymm12, ymm12, ymm12
    vpcmpgtd ymm12, ymm12, ymm4
    vpxor ymm4, ymm4, ymm12
    vpsubd ymm4, ymm4, ymm12

    vpaddd ymm0, ymm0, ymm4

    add r12, 32
    add r13, 32
    sub rcx, 8
    jmp .loop8

.tail:
    test rcx, rcx
    jz .horizontal_sum

.tail_loop:
    mov eax, [r12]                  ; Load a[i]
    sub eax, [r13]                  ; a[i] - b[i]

    ; Scalar absolute value: if (eax < 0) eax = -eax
    mov edx, eax
    sar edx, 31                     ; Sign extend
    xor eax, edx
    sub eax, edx

    vpxor ymm4, ymm4, ymm4
    vpinsrd xmm4, xmm4, eax, 0
    vpaddd ymm0, ymm0, ymm4

    add r12, 4
    add r13, 4
    dec rcx
    jnz .tail_loop

.horizontal_sum:
    vpaddd ymm0, ymm0, ymm1
    vpaddd ymm2, ymm2, ymm3
    vpaddd ymm0, ymm0, ymm2

    vextracti128 xmm1, ymm0, 1
    vpaddd xmm0, xmm0, xmm1

    vpshufd xmm1, xmm0, 0x4E
    vpaddd xmm0, xmm0, xmm1

    vpshufd xmm1, xmm0, 0xB1
    vpaddd xmm0, xmm0, xmm1

    vmovd eax, xmm0

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret
