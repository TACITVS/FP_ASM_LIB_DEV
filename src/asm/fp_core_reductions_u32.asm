; fp_core_reductions_u32.asm
;
; Hand-optimized x64 AVX2 SIMD implementations for u32 reductions
; Windows x64 calling convention
;
; Performance target: 2-4x faster than gcc -O3 (8x u32 per YMM register)
;
; Functions:
;   - fp_reduce_add_u32:  Sum of u32 array
;   - fp_reduce_mul_u32:  Product of u32 array
;   - fp_reduce_min_u32:  Minimum of u32 array (unsigned comparison)
;   - fp_reduce_max_u32:  Maximum of u32 array (unsigned comparison)

bits 64
default rel

%include "macros.inc"

section .text

; ============================================================================
; fp_reduce_add_u32: Sum of u32 array
; ============================================================================
; Signature: uint32_t fp_reduce_add_u32(const uint32_t* input, size_t n);
;
; Same as i32 - addition is identical for signed and unsigned

global fp_reduce_add_u32
fp_reduce_add_u32:
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

    vpaddd ymm0, ymm0, ymm4
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
    vpaddd ymm0, ymm0, ymm4

    add r12, 32
    sub rcx, 8
    jmp .loop8

.tail:
    test rcx, rcx
    jz .horizontal_sum

.tail_loop:
    vpxor ymm4, ymm4, ymm4
    mov eax, [r12]
    vpinsrd xmm4, xmm4, eax, 0
    vpaddd ymm0, ymm0, ymm4
    add r12, 4
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

; ============================================================================
; fp_reduce_mul_u32: Product of u32 array
; ============================================================================
; Signature: uint32_t fp_reduce_mul_u32(const uint32_t* input, size_t n);
;
; Same as i32 - multiplication is identical

global fp_reduce_mul_u32
fp_reduce_mul_u32:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    vpcmpeqd ymm0, ymm0, ymm0
    vpsrld ymm0, ymm0, 31
    vmovdqa ymm1, ymm0
    vmovdqa ymm2, ymm0
    vmovdqa ymm3, ymm0

    mov r12, rcx
    mov rcx, rdx

.loop32:
    cmp rcx, 32
    jb .loop8

    vmovdqu ymm4, [r12]
    vmovdqu ymm5, [r12 + 32]
    vmovdqu ymm6, [r12 + 64]
    vmovdqu ymm7, [r12 + 96]

    vpmulld ymm0, ymm0, ymm4
    vpmulld ymm1, ymm1, ymm5
    vpmulld ymm2, ymm2, ymm6
    vpmulld ymm3, ymm3, ymm7

    add r12, 128
    sub rcx, 32
    jmp .loop32

.loop8:
    cmp rcx, 8
    jb .tail

    vmovdqu ymm4, [r12]
    vpmulld ymm0, ymm0, ymm4

    add r12, 32
    sub rcx, 8
    jmp .loop8

.tail:
    test rcx, rcx
    jz .horizontal_product

    vpmulld ymm0, ymm0, ymm1
    vpmulld ymm2, ymm2, ymm3
    vpmulld ymm0, ymm0, ymm2

    ; Horizontal product using macro
    HPROD_U32_YMM 0, 1

    vmovd r8d, xmm0

.tail_loop:
    mov eax, [r12]
    imul r8d, eax
    add r12, 4
    dec rcx
    jnz .tail_loop

    mov eax, r8d
    jmp .epilogue

.horizontal_product:
    vpmulld ymm0, ymm0, ymm1
    vpmulld ymm2, ymm2, ymm3
    vpmulld ymm0, ymm0, ymm2

    ; Horizontal product using macro
    HPROD_U32_YMM 0, 1

    vmovd eax, xmm0

.epilogue:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_reduce_min_u32: Minimum of u32 array (unsigned)
; ============================================================================
; Signature: uint32_t fp_reduce_min_u32(const uint32_t* input, size_t n);
;
; NOTE: AVX2 has vpminud for unsigned i32 min

global fp_reduce_min_u32
fp_reduce_min_u32:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Load first element and broadcast
    vbroadcastss xmm0, [rcx]
    vinserti128 ymm0, ymm0, xmm0, 1
    vmovdqa ymm1, ymm0
    vmovdqa ymm2, ymm0
    vmovdqa ymm3, ymm0

    mov r12, rcx
    mov rcx, rdx

.loop32:
    cmp rcx, 32
    jb .loop8

    vmovdqu ymm4, [r12]
    vmovdqu ymm5, [r12 + 32]
    vmovdqu ymm6, [r12 + 64]
    vmovdqu ymm7, [r12 + 96]

    vpminud ymm0, ymm0, ymm4        ; Unsigned min
    vpminud ymm1, ymm1, ymm5
    vpminud ymm2, ymm2, ymm6
    vpminud ymm3, ymm3, ymm7

    add r12, 128
    sub rcx, 32
    jmp .loop32

.loop8:
    cmp rcx, 8
    jb .tail

    vmovdqu ymm4, [r12]
    vpminud ymm0, ymm0, ymm4

    add r12, 32
    sub rcx, 8
    jmp .loop8

.tail:
    test rcx, rcx
    jz .horizontal_min

.tail_loop:
    mov eax, [r12]
    vpinsrd xmm4, xmm4, eax, 0
    vpbroadcastd ymm4, xmm4
    vpminud ymm0, ymm0, ymm4
    add r12, 4
    dec rcx
    jnz .tail_loop

.horizontal_min:
    vpminud ymm0, ymm0, ymm1
    vpminud ymm2, ymm2, ymm3
    vpminud ymm0, ymm0, ymm2

    ; Horizontal min using macro
    HMIN_U32_YMM 0, 1

    vmovd eax, xmm0

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_reduce_max_u32: Maximum of u32 array (unsigned)
; ============================================================================
; Signature: uint32_t fp_reduce_max_u32(const uint32_t* input, size_t n);
;
; NOTE: AVX2 has vpmaxud for unsigned i32 max

global fp_reduce_max_u32
fp_reduce_max_u32:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Load first element and broadcast
    vbroadcastss xmm0, [rcx]
    vinserti128 ymm0, ymm0, xmm0, 1
    vmovdqa ymm1, ymm0
    vmovdqa ymm2, ymm0
    vmovdqa ymm3, ymm0

    mov r12, rcx
    mov rcx, rdx

.loop32:
    cmp rcx, 32
    jb .loop8

    vmovdqu ymm4, [r12]
    vmovdqu ymm5, [r12 + 32]
    vmovdqu ymm6, [r12 + 64]
    vmovdqu ymm7, [r12 + 96]

    vpmaxud ymm0, ymm0, ymm4        ; Unsigned max
    vpmaxud ymm1, ymm1, ymm5
    vpmaxud ymm2, ymm2, ymm6
    vpmaxud ymm3, ymm3, ymm7

    add r12, 128
    sub rcx, 32
    jmp .loop32

.loop8:
    cmp rcx, 8
    jb .tail

    vmovdqu ymm4, [r12]
    vpmaxud ymm0, ymm0, ymm4

    add r12, 32
    sub rcx, 8
    jmp .loop8

.tail:
    test rcx, rcx
    jz .horizontal_max

.tail_loop:
    mov eax, [r12]
    vpinsrd xmm4, xmm4, eax, 0
    vpbroadcastd ymm4, xmm4
    vpmaxud ymm0, ymm0, ymm4
    add r12, 4
    dec rcx
    jnz .tail_loop

.horizontal_max:
    vpmaxud ymm0, ymm0, ymm1
    vpmaxud ymm2, ymm2, ymm3
    vpmaxud ymm0, ymm0, ymm2

    ; Horizontal max using macro
    HMAX_U32_YMM 0, 1

    vmovd eax, xmm0

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret
