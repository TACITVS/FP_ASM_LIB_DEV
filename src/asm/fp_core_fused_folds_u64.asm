; fp_core_fused_folds_u64.asm
;
; Hand-optimized x64 AVX2 SIMD implementations for u64 fused folds
; Windows x64 calling convention
;
; Performance target: Comparable to i64 (4x u64 per YMM register)
;
; Functions:
;   - fp_fold_sumsq_u64:  Sum of squares (scalar - no SIMD i64 multiply)
;   - fp_fold_dotp_u64:   Dot product (scalar - no SIMD i64 multiply)
;   - fp_fold_sad_u64:    Sum of absolute differences

bits 64
default rel

section .text

; ============================================================================
; fp_fold_sumsq_u64: Sum of squares
; ============================================================================
; Signature: uint64_t fp_fold_sumsq_u64(const uint64_t* input, size_t n);
;
; No SIMD i64 multiply, use scalar with multiple accumulators

global fp_fold_sumsq_u64
fp_fold_sumsq_u64:
    push rbp
    mov rbp, rsp

    mov r12, rcx
    mov rcx, rdx

    xor rax, rax
    xor r8, r8
    xor r9, r9
    xor r10, r10

.loop4:
    cmp rcx, 4
    jb .loop1

    mov r11, [r12]
    imul r11, r11
    add rax, r11

    mov r11, [r12 + 8]
    imul r11, r11
    add r8, r11

    mov r11, [r12 + 16]
    imul r11, r11
    add r9, r11

    mov r11, [r12 + 24]
    imul r11, r11
    add r10, r11

    add r12, 32
    sub rcx, 4
    jmp .loop4

.loop1:
    test rcx, rcx
    jz .reduce

.loop1_iter:
    mov r11, [r12]
    imul r11, r11
    add rax, r11
    add r12, 8
    dec rcx
    jnz .loop1_iter

.reduce:
    add rax, r8
    add rax, r9
    add rax, r10

    pop rbp
    ret

; ============================================================================
; fp_fold_dotp_u64: Dot product
; ============================================================================
; Signature: uint64_t fp_fold_dotp_u64(const uint64_t* a, const uint64_t* b, size_t n);
;
; No SIMD i64 multiply, use scalar with multiple accumulators

global fp_fold_dotp_u64
fp_fold_dotp_u64:
    push rbp
    mov rbp, rsp

    mov r12, rcx
    mov r13, rdx
    mov rcx, r8

    xor rax, rax
    xor r8, r8
    xor r9, r9
    xor r10, r10

.loop4:
    cmp rcx, 4
    jb .loop1

    mov r11, [r12]
    imul r11, [r13]
    add rax, r11

    mov r11, [r12 + 8]
    imul r11, [r13 + 8]
    add r8, r11

    mov r11, [r12 + 16]
    imul r11, [r13 + 16]
    add r9, r11

    mov r11, [r12 + 24]
    imul r11, [r13 + 24]
    add r10, r11

    add r12, 32
    add r13, 32
    sub rcx, 4
    jmp .loop4

.loop1:
    test rcx, rcx
    jz .reduce

.loop1_iter:
    mov r11, [r12]
    imul r11, [r13]
    add rax, r11
    add r12, 8
    add r13, 8
    dec rcx
    jnz .loop1_iter

.reduce:
    add rax, r8
    add rax, r9
    add rax, r10

    pop rbp
    ret

; ============================================================================
; fp_fold_sad_u64: Sum of absolute differences
; ============================================================================
; Signature: uint64_t fp_fold_sad_u64(const uint64_t* a, const uint64_t* b, size_t n);
;
; For unsigned, |a - b| = max(a,b) - min(a,b)

global fp_fold_sad_u64
fp_fold_sad_u64:
    push rbp
    mov rbp, rsp

    mov r12, rcx
    mov r13, rdx
    mov rcx, r8

    xor rax, rax
    xor r8, r8
    xor r9, r9
    xor r10, r10

.loop4:
    cmp rcx, 4
    jb .loop1

    ; First pair
    mov r11, [r12]
    mov r14, [r13]
    cmp r11, r14
    jae .first_a_greater
    sub r14, r11
    add rax, r14
    jmp .second_pair
.first_a_greater:
    sub r11, r14
    add rax, r11

.second_pair:
    mov r11, [r12 + 8]
    mov r14, [r13 + 8]
    cmp r11, r14
    jae .second_a_greater
    sub r14, r11
    add r8, r14
    jmp .third_pair
.second_a_greater:
    sub r11, r14
    add r8, r11

.third_pair:
    mov r11, [r12 + 16]
    mov r14, [r13 + 16]
    cmp r11, r14
    jae .third_a_greater
    sub r14, r11
    add r9, r14
    jmp .fourth_pair
.third_a_greater:
    sub r11, r14
    add r9, r11

.fourth_pair:
    mov r11, [r12 + 24]
    mov r14, [r13 + 24]
    cmp r11, r14
    jae .fourth_a_greater
    sub r14, r11
    add r10, r14
    jmp .next_iter
.fourth_a_greater:
    sub r11, r14
    add r10, r11

.next_iter:
    add r12, 32
    add r13, 32
    sub rcx, 4
    jmp .loop4

.loop1:
    test rcx, rcx
    jz .reduce

.loop1_iter:
    mov r11, [r12]
    mov r14, [r13]
    cmp r11, r14
    jae .loop1_a_greater
    sub r14, r11
    add rax, r14
    jmp .loop1_next
.loop1_a_greater:
    sub r11, r14
    add rax, r11

.loop1_next:
    add r12, 8
    add r13, 8
    dec rcx
    jnz .loop1_iter

.reduce:
    add rax, r8
    add rax, r9
    add rax, r10

    pop rbp
    ret
