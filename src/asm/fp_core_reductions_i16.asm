; fp_core_reductions_i16.asm
;
; Hand-optimized x64 AVX2 SIMD implementations for i16 reductions
; Windows x64 calling convention
;
; Performance target: 4x faster than i64 (16x i16 per YMM register!)
;
; Functions:
;   - fp_reduce_add_i16:  Sum of i16 array
;   - fp_reduce_mul_i16:  Product of i16 array (16-bit multiply available!)
;   - fp_reduce_min_i16:  Minimum of i16 array (signed comparison)
;   - fp_reduce_max_i16:  Maximum of i16 array (signed comparison)

bits 64
default rel

%include "macros.inc"

section .text

; ============================================================================
; fp_reduce_add_i16: Sum of i16 array
; ============================================================================
; Signature: int16_t fp_reduce_add_i16(const int16_t* input, size_t n);
;
; 16-wide SIMD - 16 elements per YMM register!

global fp_reduce_add_i16
fp_reduce_add_i16:
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

    vpaddw ymm0, ymm0, ymm4
    vpaddw ymm1, ymm1, ymm5
    vpaddw ymm2, ymm2, ymm6
    vpaddw ymm3, ymm3, ymm7

    add r12, 128
    sub rcx, 64
    jmp .loop64

.loop16:
    cmp rcx, 16
    jb .tail

    vmovdqu ymm4, [r12]
    vpaddw ymm0, ymm0, ymm4

    add r12, 32
    sub rcx, 16
    jmp .loop16

.tail:
    test rcx, rcx
    jz .horizontal_sum

.tail_loop:
    vpxor ymm4, ymm4, ymm4
    movsx eax, word [r12]
    vpinsrw xmm4, xmm4, eax, 0
    vpaddw ymm0, ymm0, ymm4
    add r12, 2
    dec rcx
    jnz .tail_loop

.horizontal_sum:
    vpaddw ymm0, ymm0, ymm1
    vpaddw ymm2, ymm2, ymm3
    vpaddw ymm0, ymm0, ymm2

    ; Horizontal sum using macro
    HSUM_I16_YMM 0, 1

    vpextrw eax, xmm0, 0
    cwde  ; Sign-extend AX to EAX, then to RAX

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_reduce_mul_i16: Product of i16 array
; ============================================================================
; Signature: int16_t fp_reduce_mul_i16(const int16_t* input, size_t n);
;
; AVX2 has vpmullw for 16-bit multiply!

global fp_reduce_mul_i16
fp_reduce_mul_i16:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Initialize to all 1s
    vpcmpeqw ymm0, ymm0, ymm0
    vpsrlw ymm0, ymm0, 15
    vmovdqa ymm1, ymm0
    vmovdqa ymm2, ymm0
    vmovdqa ymm3, ymm0

    mov r12, rcx
    mov rcx, rdx

.loop64:
    cmp rcx, 64
    jb .loop16

    vmovdqu ymm4, [r12]
    vmovdqu ymm5, [r12 + 32]
    vmovdqu ymm6, [r12 + 64]
    vmovdqu ymm7, [r12 + 96]

    vpmullw ymm0, ymm0, ymm4
    vpmullw ymm1, ymm1, ymm5
    vpmullw ymm2, ymm2, ymm6
    vpmullw ymm3, ymm3, ymm7

    add r12, 128
    sub rcx, 64
    jmp .loop64

.loop16:
    cmp rcx, 16
    jb .tail

    vmovdqu ymm4, [r12]
    vpmullw ymm0, ymm0, ymm4

    add r12, 32
    sub rcx, 16
    jmp .loop16

.tail:
    test rcx, rcx
    jz .horizontal_product

    ; Reduce accumulators first
    vpmullw ymm0, ymm0, ymm1
    vpmullw ymm2, ymm2, ymm3
    vpmullw ymm0, ymm0, ymm2

    vextracti128 xmm1, ymm0, 1
    vpmullw xmm0, xmm0, xmm1

    vpshufd xmm1, xmm0, 0x4E
    vpmullw xmm0, xmm0, xmm1

    vpshufd xmm1, xmm0, 0xB1
    vpmullw xmm0, xmm0, xmm1

    vpshuflw xmm1, xmm0, 0xB1
    vpmullw xmm0, xmm0, xmm1

    vpextrw r8d, xmm0, 0

.tail_loop:
    movsx eax, word [r12]
    imul r8d, eax
    add r12, 2
    dec rcx
    jnz .tail_loop

    movsx eax, r8w
    jmp .epilogue

.horizontal_product:
    vpmullw ymm0, ymm0, ymm1
    vpmullw ymm2, ymm2, ymm3
    vpmullw ymm0, ymm0, ymm2

    ; Horizontal product using macro
    HPROD_I16_YMM 0, 1

    vpextrw eax, xmm0, 0
    cwde

.epilogue:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_reduce_min_i16: Minimum of i16 array (signed)
; ============================================================================
; Signature: int16_t fp_reduce_min_i16(const int16_t* input, size_t n);

global fp_reduce_min_i16
fp_reduce_min_i16:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Load first element and broadcast
    movsx eax, word [rcx]
    vpinsrw xmm0, xmm0, eax, 0
    vpbroadcastw ymm0, xmm0
    vmovdqa ymm1, ymm0
    vmovdqa ymm2, ymm0
    vmovdqa ymm3, ymm0

    mov r12, rcx
    mov rcx, rdx

.loop64:
    cmp rcx, 64
    jb .loop16

    vmovdqu ymm4, [r12]
    vmovdqu ymm5, [r12 + 32]
    vmovdqu ymm6, [r12 + 64]
    vmovdqu ymm7, [r12 + 96]

    vpminsw ymm0, ymm0, ymm4
    vpminsw ymm1, ymm1, ymm5
    vpminsw ymm2, ymm2, ymm6
    vpminsw ymm3, ymm3, ymm7

    add r12, 128
    sub rcx, 64
    jmp .loop64

.loop16:
    cmp rcx, 16
    jb .tail

    vmovdqu ymm4, [r12]
    vpminsw ymm0, ymm0, ymm4

    add r12, 32
    sub rcx, 16
    jmp .loop16

.tail:
    test rcx, rcx
    jz .horizontal_min

.tail_loop:
    movsx eax, word [r12]
    vpinsrw xmm4, xmm4, eax, 0
    vpbroadcastw ymm4, xmm4
    vpminsw ymm0, ymm0, ymm4
    add r12, 2
    dec rcx
    jnz .tail_loop

.horizontal_min:
    vpminsw ymm0, ymm0, ymm1
    vpminsw ymm2, ymm2, ymm3
    vpminsw ymm0, ymm0, ymm2

    ; Horizontal min using macro
    HMIN_I16_YMM 0, 1

    vpextrw eax, xmm0, 0
    cwde

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_reduce_max_i16: Maximum of i16 array (signed)
; ============================================================================
; Signature: int16_t fp_reduce_max_i16(const int16_t* input, size_t n);

global fp_reduce_max_i16
fp_reduce_max_i16:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Load first element and broadcast
    movsx eax, word [rcx]
    vpinsrw xmm0, xmm0, eax, 0
    vpbroadcastw ymm0, xmm0
    vmovdqa ymm1, ymm0
    vmovdqa ymm2, ymm0
    vmovdqa ymm3, ymm0

    mov r12, rcx
    mov rcx, rdx

.loop64:
    cmp rcx, 64
    jb .loop16

    vmovdqu ymm4, [r12]
    vmovdqu ymm5, [r12 + 32]
    vmovdqu ymm6, [r12 + 64]
    vmovdqu ymm7, [r12 + 96]

    vpmaxsw ymm0, ymm0, ymm4
    vpmaxsw ymm1, ymm1, ymm5
    vpmaxsw ymm2, ymm2, ymm6
    vpmaxsw ymm3, ymm3, ymm7

    add r12, 128
    sub rcx, 64
    jmp .loop64

.loop16:
    cmp rcx, 16
    jb .tail

    vmovdqu ymm4, [r12]
    vpmaxsw ymm0, ymm0, ymm4

    add r12, 32
    sub rcx, 16
    jmp .loop16

.tail:
    test rcx, rcx
    jz .horizontal_max

.tail_loop:
    movsx eax, word [r12]
    vpinsrw xmm4, xmm4, eax, 0
    vpbroadcastw ymm4, xmm4
    vpmaxsw ymm0, ymm0, ymm4
    add r12, 2
    dec rcx
    jnz .tail_loop

.horizontal_max:
    vpmaxsw ymm0, ymm0, ymm1
    vpmaxsw ymm2, ymm2, ymm3
    vpmaxsw ymm0, ymm0, ymm2

    ; Horizontal max using macro
    HMAX_I16_YMM 0, 1

    vpextrw eax, xmm0, 0
    cwde

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret
