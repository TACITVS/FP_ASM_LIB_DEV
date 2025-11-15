; fp_core_reductions_u8.asm
;
; Hand-optimized x64 AVX2 SIMD implementations for u8 reductions
; Windows x64 calling convention
;
; Performance target: 8x faster than u64 (32x u8 per YMM register!)
;
; CRITICAL LIMITATION: AVX2 has NO vpmullb (no 8-bit multiply instruction)
; Strategy: SIMD for add/min/max, scalar for multiply
;
; Functions:
;   - fp_reduce_add_u8:  Sum of u8 array (SIMD - 32-wide!)
;   - fp_reduce_mul_u8:  Product of u8 array (scalar - no SIMD multiply)
;   - fp_reduce_min_u8:  Minimum of u8 array (SIMD - 32-wide!)
;   - fp_reduce_max_u8:  Maximum of u8 array (SIMD - 32-wide!)

bits 64
default rel

section .text

; ============================================================================
; fp_reduce_add_u8: Sum of u8 array
; ============================================================================
; Signature: int8_t fp_reduce_add_u8(const int8_t* input, size_t n);
;
; 32-wide SIMD - 32 elements per YMM register! (8X throughput!)

global fp_reduce_add_u8
fp_reduce_add_u8:
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

.loop128:
    cmp rcx, 128
    jb .loop32

    vmovdqu ymm4, [r12]
    vmovdqu ymm5, [r12 + 32]
    vmovdqu ymm6, [r12 + 64]
    vmovdqu ymm7, [r12 + 96]

    vpaddb ymm0, ymm0, ymm4
    vpaddb ymm1, ymm1, ymm5
    vpaddb ymm2, ymm2, ymm6
    vpaddb ymm3, ymm3, ymm7

    add r12, 128
    sub rcx, 128
    jmp .loop128

.loop32:
    cmp rcx, 32
    jb .tail

    vmovdqu ymm4, [r12]
    vpaddb ymm0, ymm0, ymm4

    add r12, 32
    sub rcx, 32
    jmp .loop32

.tail:
    test rcx, rcx
    jz .horizontal_sum

.tail_loop:
    vpxor ymm4, ymm4, ymm4
    movzx eax, byte [r12]        ; Zero-extend for unsigned u8
    vpinsrb xmm4, xmm4, eax, 0
    vpaddb ymm0, ymm0, ymm4
    add r12, 1
    dec rcx
    jnz .tail_loop

.horizontal_sum:
    ; Reduce 4 accumulators to 1
    vpaddb ymm0, ymm0, ymm1
    vpaddb ymm2, ymm2, ymm3
    vpaddb ymm0, ymm0, ymm2

    ; For UNSIGNED u8, zero-extend to i16 then sum
    ; ymm0 has 32 unsigned bytes
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

; ============================================================================
; fp_reduce_mul_u8: Product of u8 array
; ============================================================================
; Signature: int8_t fp_reduce_mul_u8(const int8_t* input, size_t n);
;
; LIMITATION: No vpmullb in AVX2, must use scalar

global fp_reduce_mul_u8
fp_reduce_mul_u8:
    push rbp
    mov rbp, rsp

    mov r12, rcx
    mov rcx, rdx

    ; Initialize accumulator to 1
    mov eax, 1

    test rcx, rcx
    jz .done

.loop:
    movzx r8d, byte [r12]        ; Zero-extend for unsigned u8
    imul eax, r8d
    add r12, 1
    dec rcx
    jnz .loop

.done:
    movsx eax, al
    pop rbp
    ret

; ============================================================================
; fp_reduce_min_u8: Minimum of u8 array (unsigned)
; ============================================================================
; Signature: int8_t fp_reduce_min_u8(const int8_t* input, size_t n);

global fp_reduce_min_u8
fp_reduce_min_u8:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Load first element and broadcast
    movzx eax, byte [rcx]        ; Zero-extend for unsigned u8
    vpinsrb xmm0, xmm0, eax, 0
    vpbroadcastb ymm0, xmm0
    vmovdqa ymm1, ymm0
    vmovdqa ymm2, ymm0
    vmovdqa ymm3, ymm0

    mov r12, rcx
    mov rcx, rdx

.loop128:
    cmp rcx, 128
    jb .loop32

    vmovdqu ymm4, [r12]
    vmovdqu ymm5, [r12 + 32]
    vmovdqu ymm6, [r12 + 64]
    vmovdqu ymm7, [r12 + 96]

    vpminub ymm0, ymm0, ymm4
    vpminub ymm1, ymm1, ymm5
    vpminub ymm2, ymm2, ymm6
    vpminub ymm3, ymm3, ymm7

    add r12, 128
    sub rcx, 128
    jmp .loop128

.loop32:
    cmp rcx, 32
    jb .tail

    vmovdqu ymm4, [r12]
    vpminub ymm0, ymm0, ymm4

    add r12, 32
    sub rcx, 32
    jmp .loop32

.tail:
    test rcx, rcx
    jz .horizontal_min

.tail_loop:
    movzx eax, byte [r12]        ; Zero-extend for unsigned u8
    vpinsrb xmm4, xmm4, eax, 0
    vpbroadcastb ymm4, xmm4
    vpminub ymm0, ymm0, ymm4
    add r12, 1
    dec rcx
    jnz .tail_loop

.horizontal_min:
    ; Reduce 4 accumulators to 1
    vpminub ymm0, ymm0, ymm1
    vpminub ymm2, ymm2, ymm3
    vpminub ymm0, ymm0, ymm2

    ; Reduce 32 bytes to 16 bytes
    vextracti128 xmm1, ymm0, 1
    vpminub xmm0, xmm0, xmm1

    ; Reduce 16 bytes to 1 byte using byte-level shifts
    vpsrldq xmm1, xmm0, 8          ; Shift right 8 bytes
    vpminub xmm0, xmm0, xmm1       ; Min of lower 8 with upper 8

    vpsrldq xmm1, xmm0, 4          ; Shift right 4 bytes
    vpminub xmm0, xmm0, xmm1       ; Min of lower 4 with next 4

    vpsrldq xmm1, xmm0, 2          ; Shift right 2 bytes
    vpminub xmm0, xmm0, xmm1       ; Min of lower 2 with next 2

    vpsrldq xmm1, xmm0, 1          ; Shift right 1 byte
    vpminub xmm0, xmm0, xmm1       ; Min of byte 0 with byte 1

    vpextrb eax, xmm0, 0           ; Extract final minimum
    movsx eax, al

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_reduce_max_u8: Maximum of u8 array (unsigned)
; ============================================================================
; Signature: int8_t fp_reduce_max_u8(const int8_t* input, size_t n);

global fp_reduce_max_u8
fp_reduce_max_u8:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Load first element and broadcast
    movzx eax, byte [rcx]        ; Zero-extend for unsigned u8
    vpinsrb xmm0, xmm0, eax, 0
    vpbroadcastb ymm0, xmm0
    vmovdqa ymm1, ymm0
    vmovdqa ymm2, ymm0
    vmovdqa ymm3, ymm0

    mov r12, rcx
    mov rcx, rdx

.loop128:
    cmp rcx, 128
    jb .loop32

    vmovdqu ymm4, [r12]
    vmovdqu ymm5, [r12 + 32]
    vmovdqu ymm6, [r12 + 64]
    vmovdqu ymm7, [r12 + 96]

    vpmaxub ymm0, ymm0, ymm4
    vpmaxub ymm1, ymm1, ymm5
    vpmaxub ymm2, ymm2, ymm6
    vpmaxub ymm3, ymm3, ymm7

    add r12, 128
    sub rcx, 128
    jmp .loop128

.loop32:
    cmp rcx, 32
    jb .tail

    vmovdqu ymm4, [r12]
    vpmaxub ymm0, ymm0, ymm4

    add r12, 32
    sub rcx, 32
    jmp .loop32

.tail:
    test rcx, rcx
    jz .horizontal_max

.tail_loop:
    movzx eax, byte [r12]        ; Zero-extend for unsigned u8
    vpinsrb xmm4, xmm4, eax, 0
    vpbroadcastb ymm4, xmm4
    vpmaxub ymm0, ymm0, ymm4
    add r12, 1
    dec rcx
    jnz .tail_loop

.horizontal_max:
    ; Reduce 4 accumulators to 1
    vpmaxub ymm0, ymm0, ymm1
    vpmaxub ymm2, ymm2, ymm3
    vpmaxub ymm0, ymm0, ymm2

    ; Reduce 32 bytes to 16 bytes
    vextracti128 xmm1, ymm0, 1
    vpmaxub xmm0, xmm0, xmm1

    ; Reduce 16 bytes to 1 byte using byte-level shifts
    vpsrldq xmm1, xmm0, 8          ; Shift right 8 bytes
    vpmaxub xmm0, xmm0, xmm1       ; Max of lower 8 with upper 8

    vpsrldq xmm1, xmm0, 4          ; Shift right 4 bytes
    vpmaxub xmm0, xmm0, xmm1       ; Max of lower 4 with next 4

    vpsrldq xmm1, xmm0, 2          ; Shift right 2 bytes
    vpmaxub xmm0, xmm0, xmm1       ; Max of lower 2 with next 2

    vpsrldq xmm1, xmm0, 1          ; Shift right 1 byte
    vpmaxub xmm0, xmm0, xmm1       ; Max of byte 0 with byte 1

    vpextrb eax, xmm0, 0           ; Extract final maximum
    movsx eax, al

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret
