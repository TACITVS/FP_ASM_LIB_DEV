; fp_core_reductions_u64.asm
;
; Hand-optimized x64 AVX2 SIMD implementations for u64 reductions
; Windows x64 calling convention
;
; Performance target: Comparable to i64 (4x u64 per YMM register)
;
; Functions:
;   - fp_reduce_add_u64:  Sum of u64 array
;   - fp_reduce_mul_u64:  Product of u64 array (scalar - no SIMD i64 multiply)
;   - fp_reduce_min_u64:  Minimum of u64 array (scalar - no unsigned i64 compare)
;   - fp_reduce_max_u64:  Maximum of u64 array (scalar - no unsigned i64 compare)

bits 64
default rel

%include "macros.inc"

section .text

; ============================================================================
; fp_reduce_add_u64: Sum of u64 array
; ============================================================================
; Signature: uint64_t fp_reduce_add_u64(const uint64_t* input, size_t n);
;
; Same as i64 - addition is identical for signed and unsigned

global fp_reduce_add_u64
fp_reduce_add_u64:
    ; Null pointer check
    test rcx, rcx
    jz .error_null

    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    vpxor ymm0, ymm0, ymm0
    vpxor ymm1, ymm1, ymm1
    vpxor ymm2, ymm2, ymm2
    vpxor ymm3, ymm3, ymm3

    mov r10, rcx
    mov rcx, rdx

.loop16:
    cmp rcx, 16
    jb .loop4

    vmovdqu ymm4, [r10]
    vmovdqu ymm5, [r10 + 32]
    vmovdqu ymm6, [r10 + 64]
    vmovdqu ymm7, [r10 + 96]

    vpaddq ymm0, ymm0, ymm4
    vpaddq ymm1, ymm1, ymm5
    vpaddq ymm2, ymm2, ymm6
    vpaddq ymm3, ymm3, ymm7

    add r10, 128
    sub rcx, 16
    jmp .loop16

.loop4:
    cmp rcx, 4
    jb .tail

    vmovdqu ymm4, [r10]
    vpaddq ymm0, ymm0, ymm4

    add r10, 32
    sub rcx, 4
    jmp .loop4

.tail:
    test rcx, rcx
    jz .horizontal_sum

.tail_loop:
    mov rax, [r10]
    vpxor ymm4, ymm4, ymm4
    vpinsrq xmm4, xmm4, rax, 0
    vpaddq ymm0, ymm0, ymm4
    add r10, 8
    dec rcx
    jnz .tail_loop

.horizontal_sum:
    vpaddq ymm0, ymm0, ymm1
    vpaddq ymm2, ymm2, ymm3
    vpaddq ymm0, ymm0, ymm2

    ; Horizontal sum using macro
    HSUM_U64_YMM 0, 1

    vmovq rax, xmm0

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

.error_null:
    xor rax, rax                 ; Return 0 for null pointer
    ret

; ============================================================================
; fp_reduce_mul_u64: Product of u64 array
; ============================================================================
; Signature: uint64_t fp_reduce_mul_u64(const uint64_t* input, size_t n);
;
; No SIMD i64 multiply in AVX2, use scalar
; NOTE: For multiplication mod 2^64, IMUL and MUL produce identical lower 64 bits
; Using IMUL for convenience (allows multiple accumulators)

global fp_reduce_mul_u64
fp_reduce_mul_u64:
    ; Null pointer check
    test rcx, rcx
    jz .error_null

    push rbp
    mov rbp, rsp

    mov r10, rcx
    mov rcx, rdx

    ; Initialize 4 accumulators
    mov rax, 1
    mov r8, 1
    mov r9, 1
    mov rdx, 1

.loop4:
    cmp rcx, 4
    jb .loop1

    mov r11, [r10]
    imul rax, r11               ; Multiply rax by r11 (lower 64 bits)

    mov r11, [r10 + 8]
    imul r8, r11

    mov r11, [r10 + 16]
    imul r9, r11

    mov r11, [r10 + 24]
    imul rdx, r11

    add r10, 32
    sub rcx, 4
    jmp .loop4

.loop1:
    test rcx, rcx
    jz .reduce

.loop1_iter:
    mov r11, [r10]
    imul rax, r11
    add r10, 8
    dec rcx
    jnz .loop1_iter

.reduce:
    imul rax, r8
    imul rax, r9
    imul rax, rdx

    vzeroupper                  ; Clear YMM state to avoid AVX-SSE transition penalty
    pop rbp
    ret

.error_null:
    mov rax, 1                   ; Return 1 for null pointer (identity for multiply)
    ret

; ============================================================================
; fp_reduce_min_u64: Minimum of u64 array (unsigned)
; ============================================================================
; Signature: uint64_t fp_reduce_min_u64(const uint64_t* input, size_t n);
;
; AVX2 lacks unsigned i64 min, use scalar with multiple accumulators

global fp_reduce_min_u64
fp_reduce_min_u64:
    ; Null pointer check
    test rcx, rcx
    jz .error_null

    push rbp
    mov rbp, rsp

    ; Check for zero-length array before loading
    test rdx, rdx
    jz .return_max              ; Return UINT64_MAX for empty array

    mov r10, rcx
    mov rcx, rdx

    ; Initialize 4 accumulators with first element
    mov rax, [r10]
    mov r8, rax
    mov r9, rax
    mov rdx, rax

.loop4:
    cmp rcx, 4
    jb .loop1

    mov r11, [r10]
    cmp r11, rax
    cmovb rax, r11

    mov r11, [r10 + 8]
    cmp r11, r8
    cmovb r8, r11

    mov r11, [r10 + 16]
    cmp r11, r9
    cmovb r9, r11

    mov r11, [r10 + 24]
    cmp r11, rdx
    cmovb rdx, r11

    add r10, 32
    sub rcx, 4
    jmp .loop4

.loop1:
    test rcx, rcx
    jz .reduce

.loop1_iter:
    mov r11, [r10]
    cmp r11, rax
    cmovb rax, r11
    add r10, 8
    dec rcx
    jnz .loop1_iter

.reduce:
    cmp r8, rax
    cmovb rax, r8

    cmp r9, rax
    cmovb rax, r9

    cmp rdx, rax
    cmovb rax, rdx

    vzeroupper                  ; Clear YMM state to avoid AVX-SSE transition penalty
    pop rbp
    ret

.return_max:
    mov rax, 0xFFFFFFFFFFFFFFFF  ; Return UINT64_MAX for empty array
    vzeroupper
    pop rbp
    ret

.error_null:
    mov rax, 0xFFFFFFFFFFFFFFFF  ; Return UINT64_MAX for null pointer
    ret

; ============================================================================
; fp_reduce_max_u64: Maximum of u64 array (unsigned)
; ============================================================================
; Signature: uint64_t fp_reduce_max_u64(const uint64_t* input, size_t n);
;
; AVX2 lacks unsigned i64 max, use scalar with multiple accumulators

global fp_reduce_max_u64
fp_reduce_max_u64:
    ; Null pointer check
    test rcx, rcx
    jz .error_null

    push rbp
    mov rbp, rsp

    ; Check for zero-length array before loading
    test rdx, rdx
    jz .return_zero             ; Return 0 for empty array

    mov r10, rcx
    mov rcx, rdx

    ; Initialize 4 accumulators with first element
    mov rax, [r10]
    mov r8, rax
    mov r9, rax
    mov rdx, rax

.loop4:
    cmp rcx, 4
    jb .loop1

    mov r11, [r10]
    cmp r11, rax
    cmova rax, r11

    mov r11, [r10 + 8]
    cmp r11, r8
    cmova r8, r11

    mov r11, [r10 + 16]
    cmp r11, r9
    cmova r9, r11

    mov r11, [r10 + 24]
    cmp r11, rdx
    cmova rdx, r11

    add r10, 32
    sub rcx, 4
    jmp .loop4

.loop1:
    test rcx, rcx
    jz .reduce

.loop1_iter:
    mov r11, [r10]
    cmp r11, rax
    cmova rax, r11
    add r10, 8
    dec rcx
    jnz .loop1_iter

.reduce:
    cmp r8, rax
    cmova rax, r8

    cmp r9, rax
    cmova rax, r9

    cmp rdx, rax
    cmova rax, rdx

    vzeroupper                  ; Clear YMM state to avoid AVX-SSE transition penalty
    pop rbp
    ret

.return_zero:
    xor rax, rax                ; Return 0 for empty array
    vzeroupper
    pop rbp
    ret

.error_null:
    xor rax, rax                 ; Return 0 for null pointer
    ret
