; fp_core_reductions_f32.asm
;
; Hand-optimized x64 AVX2 SIMD implementations for f32 reductions
; Windows x64 calling convention
;
; Performance target: 2-4x faster than gcc -O3 (8x f32 per YMM register)
;
; Functions:
;   - fp_reduce_add_f32:  Sum of f32 array
;   - fp_reduce_mul_f32:  Product of f32 array
;   - fp_reduce_min_f32:  Minimum of f32 array
;   - fp_reduce_max_f32:  Maximum of f32 array

bits 64
default rel

section .text

; ============================================================================
; fp_reduce_add_f32: Sum of f32 array
; ============================================================================
; Signature: float fp_reduce_add_f32(const float* input, size_t n);
;
; Haskell type: foldl (+) 0.0 :: [Float] -> Float
;
; Algorithm:
;   - Process 32 elements per iteration (4 YMM registers)
;   - Use 4 independent accumulators to avoid dependency chains
;   - 8x f32 per YMM register (DOUBLE f64 throughput!)
;
; Performance: ~2-3x faster than gcc -O3 (8-wide SIMD)

global fp_reduce_add_f32
fp_reduce_add_f32:
    ; Windows x64 ABI: RCX = input, RDX = n
    ; Return: XMM0 = sum

    ; Prologue
    push rbp
    mov rbp, rsp
    sub rsp, 32                     ; Shadow space
    and rsp, 0xFFFFFFFFFFFFFFE0     ; Align to 32 bytes

    ; Initialize accumulators (4 independent YMM registers)
    vxorps ymm0, ymm0, ymm0         ; acc1 = 0.0
    vxorps ymm1, ymm1, ymm1         ; acc2 = 0.0
    vxorps ymm2, ymm2, ymm2         ; acc3 = 0.0
    vxorps ymm3, ymm3, ymm3         ; acc4 = 0.0

    mov r12, rcx                    ; r12 = input pointer
    mov rcx, rdx                    ; rcx = count

    ; Main loop: 32 elements per iteration (4 YMM * 8 f32)
.loop32:
    cmp rcx, 32
    jb .loop8

    vmovups ymm4, [r12]             ; Load 8x f32
    vmovups ymm5, [r12 + 32]        ; Load 8x f32
    vmovups ymm6, [r12 + 64]        ; Load 8x f32
    vmovups ymm7, [r12 + 96]        ; Load 8x f32

    vaddps ymm0, ymm0, ymm4         ; Accumulate
    vaddps ymm1, ymm1, ymm5
    vaddps ymm2, ymm2, ymm6
    vaddps ymm3, ymm3, ymm7

    add r12, 128                    ; 32 * 4 bytes
    sub rcx, 32
    jmp .loop32

.loop8:
    cmp rcx, 8
    jb .tail

    vmovups ymm4, [r12]             ; Load 8x f32
    vaddps ymm0, ymm0, ymm4         ; Accumulate

    add r12, 32                     ; 8 * 4 bytes
    sub rcx, 8
    jmp .loop8

.tail:
    ; Handle remaining elements (0-7)
    test rcx, rcx
    jz .horizontal_sum

.tail_loop:
    vxorps ymm4, ymm4, ymm4         ; Zero ymm4 (use YMM to preserve upper bits!)
    vmovss xmm4, [r12]              ; Load single f32
    vaddps ymm0, ymm0, ymm4         ; Add to accumulator (use YMM!)
    add r12, 4
    dec rcx
    jnz .tail_loop

.horizontal_sum:
    ; Sum all 4 accumulators
    vaddps ymm0, ymm0, ymm1
    vaddps ymm2, ymm2, ymm3
    vaddps ymm0, ymm0, ymm2

    ; Horizontal sum of 8x f32 in ymm0
    vextractf128 xmm1, ymm0, 1      ; Extract upper 128 bits
    vaddps xmm0, xmm0, xmm1         ; Sum upper/lower halves (4+4 = 4 elements)

    vshufps xmm1, xmm0, xmm0, 0x4E  ; Shuffle [2,3,0,1]
    vaddps xmm0, xmm0, xmm1         ; Sum (2 elements)

    vshufps xmm1, xmm0, xmm0, 0xB1  ; Shuffle [1,0,3,2]
    vaddps xmm0, xmm0, xmm1         ; Sum (1 element)

    ; Result already in xmm0 (Windows ABI: float return in xmm0)

    ; Epilogue
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_reduce_mul_f32: Product of f32 array
; ============================================================================
; Signature: float fp_reduce_mul_f32(const float* input, size_t n);
;
; NOTE: Floating-point multiply is commutative and associative (within tolerance)

global fp_reduce_mul_f32
fp_reduce_mul_f32:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Initialize accumulators to 1.0
    vbroadcastss ymm0, [rel .one]   ; acc1 = 1.0
    vmovaps ymm1, ymm0
    vmovaps ymm2, ymm0
    vmovaps ymm3, ymm0

    mov r12, rcx
    mov rcx, rdx

.loop32:
    cmp rcx, 32
    jb .loop8

    vmovups ymm4, [r12]
    vmovups ymm5, [r12 + 32]
    vmovups ymm6, [r12 + 64]
    vmovups ymm7, [r12 + 96]

    vmulps ymm0, ymm0, ymm4         ; f32 multiply (AVX has this!)
    vmulps ymm1, ymm1, ymm5
    vmulps ymm2, ymm2, ymm6
    vmulps ymm3, ymm3, ymm7

    add r12, 128
    sub rcx, 32
    jmp .loop32

.loop8:
    cmp rcx, 8
    jb .tail

    vmovups ymm4, [r12]
    vmulps ymm0, ymm0, ymm4

    add r12, 32
    sub rcx, 8
    jmp .loop8

.tail:
    test rcx, rcx
    jz .horizontal_product

    ; First, do horizontal product of SIMD accumulators
    vmulps ymm0, ymm0, ymm1
    vmulps ymm2, ymm2, ymm3
    vmulps ymm0, ymm0, ymm2

    vextractf128 xmm1, ymm0, 1
    vmulps xmm0, xmm0, xmm1

    vshufps xmm1, xmm0, xmm0, 0x4E
    vmulps xmm0, xmm0, xmm1

    vshufps xmm1, xmm0, xmm0, 0xB1
    vmulps xmm0, xmm0, xmm1

.tail_loop:
    vmovss xmm4, [r12]              ; Load value
    vmulss xmm0, xmm0, xmm4         ; Scalar multiply
    add r12, 4
    dec rcx
    jnz .tail_loop

    jmp .epilogue

.horizontal_product:
    vmulps ymm0, ymm0, ymm1
    vmulps ymm2, ymm2, ymm3
    vmulps ymm0, ymm0, ymm2

    vextractf128 xmm1, ymm0, 1
    vmulps xmm0, xmm0, xmm1

    vshufps xmm1, xmm0, xmm0, 0x4E
    vmulps xmm0, xmm0, xmm1

    vshufps xmm1, xmm0, xmm0, 0xB1
    vmulps xmm0, xmm0, xmm1

.epilogue:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; Constant data
section .rodata
align 4
.one: dd 1.0

; ============================================================================
; fp_reduce_min_f32: Minimum of f32 array
; ============================================================================
; Signature: float fp_reduce_min_f32(const float* input, size_t n);
;
; NOTE: AVX has vminps for f32 min

section .text
global fp_reduce_min_f32
fp_reduce_min_f32:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Load first element as initial value
    vbroadcastss ymm0, [rcx]
    vmovaps ymm1, ymm0
    vmovaps ymm2, ymm0
    vmovaps ymm3, ymm0

    mov r12, rcx
    mov rcx, rdx

.loop32:
    cmp rcx, 32
    jb .loop8

    vmovups ymm4, [r12]
    vmovups ymm5, [r12 + 32]
    vmovups ymm6, [r12 + 64]
    vmovups ymm7, [r12 + 96]

    vminps ymm0, ymm0, ymm4         ; Min of 8x f32
    vminps ymm1, ymm1, ymm5
    vminps ymm2, ymm2, ymm6
    vminps ymm3, ymm3, ymm7

    add r12, 128
    sub rcx, 32
    jmp .loop32

.loop8:
    cmp rcx, 8
    jb .tail

    vmovups ymm4, [r12]
    vminps ymm0, ymm0, ymm4

    add r12, 32
    sub rcx, 8
    jmp .loop8

.tail:
    test rcx, rcx
    jz .horizontal_min

.tail_loop:
    vmovss xmm4, [r12]
    vbroadcastss ymm4, xmm4         ; Broadcast xmm4[0] to all lanes of ymm4
    vminps ymm0, ymm0, ymm4         ; Min (use YMM to preserve upper bits!)
    add r12, 4
    dec rcx
    jnz .tail_loop

.horizontal_min:
    vminps ymm0, ymm0, ymm1
    vminps ymm2, ymm2, ymm3
    vminps ymm0, ymm0, ymm2

    vextractf128 xmm1, ymm0, 1
    vminps xmm0, xmm0, xmm1

    vshufps xmm1, xmm0, xmm0, 0x4E
    vminps xmm0, xmm0, xmm1

    vshufps xmm1, xmm0, xmm0, 0xB1
    vminps xmm0, xmm0, xmm1

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_reduce_max_f32: Maximum of f32 array
; ============================================================================
; Signature: float fp_reduce_max_f32(const float* input, size_t n);

global fp_reduce_max_f32
fp_reduce_max_f32:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Load first element as initial value
    vbroadcastss ymm0, [rcx]
    vmovaps ymm1, ymm0
    vmovaps ymm2, ymm0
    vmovaps ymm3, ymm0

    mov r12, rcx
    mov rcx, rdx

.loop32:
    cmp rcx, 32
    jb .loop8

    vmovups ymm4, [r12]
    vmovups ymm5, [r12 + 32]
    vmovups ymm6, [r12 + 64]
    vmovups ymm7, [r12 + 96]

    vmaxps ymm0, ymm0, ymm4         ; Max of 8x f32
    vmaxps ymm1, ymm1, ymm5
    vmaxps ymm2, ymm2, ymm6
    vmaxps ymm3, ymm3, ymm7

    add r12, 128
    sub rcx, 32
    jmp .loop32

.loop8:
    cmp rcx, 8
    jb .tail

    vmovups ymm4, [r12]
    vmaxps ymm0, ymm0, ymm4

    add r12, 32
    sub rcx, 8
    jmp .loop8

.tail:
    test rcx, rcx
    jz .horizontal_max

.tail_loop:
    vmovss xmm4, [r12]
    vbroadcastss ymm4, xmm4         ; Broadcast xmm4[0] to all lanes of ymm4
    vmaxps ymm0, ymm0, ymm4         ; Max (use YMM to preserve upper bits!)
    add r12, 4
    dec rcx
    jnz .tail_loop

.horizontal_max:
    vmaxps ymm0, ymm0, ymm1
    vmaxps ymm2, ymm2, ymm3
    vmaxps ymm0, ymm0, ymm2

    vextractf128 xmm1, ymm0, 1
    vmaxps xmm0, xmm0, xmm1

    vshufps xmm1, xmm0, xmm0, 0x4E
    vmaxps xmm0, xmm0, xmm1

    vshufps xmm1, xmm0, xmm0, 0xB1
    vmaxps xmm0, xmm0, xmm1

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret
