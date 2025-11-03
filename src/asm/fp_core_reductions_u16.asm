; fp_core_reductions_u16.asm
;
; Hand-optimized x64 AVX2 SIMD implementations for u16 reductions
; Windows x64 calling convention
;
; Performance target: 4x faster than u64 (16x u16 per YMM register!)
;
; Functions:
;   - fp_reduce_add_u16:  Sum of u16 array
;   - fp_reduce_mul_u16:  Product of u16 array (16-bit multiply available!)
;   - fp_reduce_min_u16:  Minimum of u16 array (unsigned comparison)
;   - fp_reduce_max_u16:  Maximum of u16 array (unsigned comparison)

bits 64
default rel

section .text

; ============================================================================
; fp_reduce_add_u16: Sum of u16 array
; ============================================================================
; Signature: uint16_t fp_reduce_add_u16(const uint16_t* input, size_t n);
;
; 16-wide SIMD - 16 elements per YMM register!

global fp_reduce_add_u16
fp_reduce_add_u16:
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
    movzx eax, word [r12]
    vpinsrw xmm4, xmm4, eax, 0
    vpaddw ymm0, ymm0, ymm4
    add r12, 2
    dec rcx
    jnz .tail_loop

.horizontal_sum:
    vpaddw ymm0, ymm0, ymm1
    vpaddw ymm2, ymm2, ymm3
    vpaddw ymm0, ymm0, ymm2

    vextracti128 xmm1, ymm0, 1
    vpaddw xmm0, xmm0, xmm1

    vpshufd xmm1, xmm0, 0x4E
    vpaddw xmm0, xmm0, xmm1

    vpshufd xmm1, xmm0, 0xB1
    vpaddw xmm0, xmm0, xmm1

    vpshuflw xmm1, xmm0, 0xB1
    vpaddw xmm0, xmm0, xmm1

    vpextrw eax, xmm0, 0
    ; Zero-extend already in EAX

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_reduce_mul_u16: Product of u16 array
; ============================================================================
; Signature: uint16_t fp_reduce_mul_u16(const uint16_t* input, size_t n);
;
; AVX2 has vpmullw for 16-bit multiply!

global fp_reduce_mul_u16
fp_reduce_mul_u16:
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
    movzx eax, word [r12]
    imul r8d, eax
    add r12, 2
    dec rcx
    jnz .tail_loop

    mov eax, r8d
    and eax, 0xFFFF
    jmp .epilogue

.horizontal_product:
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

    vpextrw eax, xmm0, 0

.epilogue:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_reduce_min_u16: Minimum of u16 array (unsigned)
; ============================================================================
; Signature: uint16_t fp_reduce_min_u16(const uint16_t* input, size_t n);

global fp_reduce_min_u16
fp_reduce_min_u16:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Load first element and broadcast
    movzx eax, word [rcx]
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

    vpminuw ymm0, ymm0, ymm4
    vpminuw ymm1, ymm1, ymm5
    vpminuw ymm2, ymm2, ymm6
    vpminuw ymm3, ymm3, ymm7

    add r12, 128
    sub rcx, 64
    jmp .loop64

.loop16:
    cmp rcx, 16
    jb .tail

    vmovdqu ymm4, [r12]
    vpminuw ymm0, ymm0, ymm4

    add r12, 32
    sub rcx, 16
    jmp .loop16

.tail:
    test rcx, rcx
    jz .horizontal_min

.tail_loop:
    movzx eax, word [r12]
    vpinsrw xmm4, xmm4, eax, 0
    vpbroadcastw ymm4, xmm4
    vpminuw ymm0, ymm0, ymm4
    add r12, 2
    dec rcx
    jnz .tail_loop

.horizontal_min:
    vpminuw ymm0, ymm0, ymm1
    vpminuw ymm2, ymm2, ymm3
    vpminuw ymm0, ymm0, ymm2

    vextracti128 xmm1, ymm0, 1
    vpminuw xmm0, xmm0, xmm1

    vpshufd xmm1, xmm0, 0x4E
    vpminuw xmm0, xmm0, xmm1

    vpshufd xmm1, xmm0, 0xB1
    vpminuw xmm0, xmm0, xmm1

    vpshuflw xmm1, xmm0, 0xB1
    vpminuw xmm0, xmm0, xmm1

    vpextrw eax, xmm0, 0

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_reduce_max_u16: Maximum of u16 array (unsigned)
; ============================================================================
; Signature: uint16_t fp_reduce_max_u16(const uint16_t* input, size_t n);

global fp_reduce_max_u16
fp_reduce_max_u16:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Load first element and broadcast
    movzx eax, word [rcx]
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

    vpmaxuw ymm0, ymm0, ymm4
    vpmaxuw ymm1, ymm1, ymm5
    vpmaxuw ymm2, ymm2, ymm6
    vpmaxuw ymm3, ymm3, ymm7

    add r12, 128
    sub rcx, 64
    jmp .loop64

.loop16:
    cmp rcx, 16
    jb .tail

    vmovdqu ymm4, [r12]
    vpmaxuw ymm0, ymm0, ymm4

    add r12, 32
    sub rcx, 16
    jmp .loop16

.tail:
    test rcx, rcx
    jz .horizontal_max

.tail_loop:
    movzx eax, word [r12]
    vpinsrw xmm4, xmm4, eax, 0
    vpbroadcastw ymm4, xmm4
    vpmaxuw ymm0, ymm0, ymm4
    add r12, 2
    dec rcx
    jnz .tail_loop

.horizontal_max:
    vpmaxuw ymm0, ymm0, ymm1
    vpmaxuw ymm2, ymm2, ymm3
    vpmaxuw ymm0, ymm0, ymm2

    vextracti128 xmm1, ymm0, 1
    vpmaxuw xmm0, xmm0, xmm1

    vpshufd xmm1, xmm0, 0x4E
    vpmaxuw xmm0, xmm0, xmm1

    vpshufd xmm1, xmm0, 0xB1
    vpmaxuw xmm0, xmm0, xmm1

    vpshuflw xmm1, xmm0, 0xB1
    vpmaxuw xmm0, xmm0, xmm1

    vpextrw eax, xmm0, 0

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret
