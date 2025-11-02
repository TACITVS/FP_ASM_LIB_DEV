; =============================================================================
; FP-ASM Core Library: Fused Folds Module (Module 2)
;
; *** Corrected v5: Implement i64 abs using AVX2 compare, not AVX-512 shift ***
;
; Implements:
;   - fp_fold_sumsq_i64  (Scalar Win)
;   - fp_fold_dotp_i64   (Scalar Win)
;   - fp_fold_dotp_f64   (FMA Win)
;   - fp_fold_sad_i64    (SIMD Match - Correct AVX2 abs)
; =============================================================================
default rel
section .data
    ; Removed unused masks

section .text
    global fp_fold_sumsq_i64
    global fp_fold_dotp_i64
    global fp_fold_dotp_f64
    global fp_fold_sad_i64

; =============================================================================
; int64_t fp_fold_sumsq_i64(const int64_t* in, size_t n)
; (Unchanged v4 - Passed tests)
; =============================================================================
fp_fold_sumsq_i64:
    push rbx
    push r12
    push r13
    push r14
    push r15
    push rsi
    push rdi
    sub  rsp, 32
    mov  r12, rcx
    mov  r13, rdx
    xor  rax, rax
    xor  r14, r14
    xor  r15, r15
    xor  rbx, rbx
    test r13, r13
    jz   .ssq_done
.ssq_loop8:
    cmp  r13, 8
    jb   .ssq_tail
    prefetcht0 [r12 + 256]
    mov  r8,  [r12]
    mov  r9,  [r12+8]
    mov  r10, [r12+16]
    mov  r11, [r12+24]
    mov  rcx, [r12+32]
    mov  rdx, [r12+40]
    mov  rsi, [r12+48]
    mov  rdi, [r12+56]
    imul r8,  r8
    imul r9,  r9
    imul r10, r10
    imul r11, r11
    imul rcx, rcx
    imul rdx, rdx
    imul rsi, rsi
    imul rdi, rdi
    add  rax, r8
    add  r14, r9
    add  r15, r10
    add  rbx, r11
    add  rax, rcx
    add  r14, rdx
    add  r15, rsi
    add  rbx, rdi
    add  r12, 64
    sub  r13, 8
    jmp  .ssq_loop8
.ssq_tail:
    test r13, r13
    jz   .ssq_accum
.ssq_tail_loop:
    mov  r8, [r12]
    imul r8, r8
    add  rax, r8
    add  r12, 8
    dec  r13
    jnz  .ssq_tail_loop
.ssq_accum:
    add  rax, r14
    add  rax, r15
    add  rax, rbx
.ssq_done:
    add  rsp, 32
    pop  rdi
    pop  rsi
    pop  r15
    pop  r14
    pop  r13
    pop  r12
    pop  rbx
    ret

; =============================================================================
; int64_t fp_fold_dotp_i64(const int64_t* a, const int64_t* b, size_t n)
; (Unchanged v4 - Passed tests)
; =============================================================================
fp_fold_dotp_i64:
    push rbx
    push r12
    push r13
    push r14
    push r15
    push rsi
    push rdi
    sub  rsp, 32
    mov  r12, rcx
    mov  r13, rdx
    mov  r14, r8
    xor  rax, rax
    xor  r15, r15
    xor  rbx, rbx
    xor  rsi, rsi
    test r14, r14
    jz   .dotp_i64_done
.dotp_i64_loop4:
    cmp  r14, 4
    jb   .dotp_i64_tail
    prefetcht0 [r12 + 256]
    prefetcht0 [r13 + 256]
    mov  r8,  [r12]
    mov  r9,  [r13]
    mov  r10, [r12+8]
    mov  r11, [r13+8]
    mov  rcx, [r12+16]
    mov  rdx, [r13+16]
    mov  rdi, [r12+24]
    imul r9,  r8
    imul r11, r10
    imul rdx, rcx
    mov  r8,  [r13+24]
    imul r8,  rdi
    add  rax, r9
    add  r15, r11
    add  rbx, rdx
    add  rsi, r8
    add  r12, 32
    add  r13, 32
    sub  r14, 4
    jmp  .dotp_i64_loop4
.dotp_i64_tail:
    test r14, r14
    jz   .dotp_i64_accum
.dotp_i64_tail_loop:
    mov  r8, [r12]
    imul r8, [r13]
    add  rax, r8
    add  r12, 8
    add  r13, 8
    dec  r14
    jnz  .dotp_i64_tail_loop
.dotp_i64_accum:
    add  rax, r15
    add  rax, rbx
    add  rax, rsi
.dotp_i64_done:
    add  rsp, 32
    pop  rdi
    pop  rsi
    pop  r15
    pop  r14
    pop  r13
    pop  r12
    pop  rbx
    ret

; =============================================================================
; double fp_fold_dotp_f64(const double* a, const double* b, size_t n)
; (Unchanged v3 - Passed tests)
; =============================================================================
fp_fold_dotp_f64:
    push r11
    push r12
    push r13
    mov  r11, rsp
    and  rsp, 0xFFFFFFFFFFFFFFE0
    sub  rsp, 128
    vmovdqa [rsp],     ymm6
    vmovdqa [rsp+32],  ymm7
    vmovdqa [rsp+64],  ymm8
    vmovdqa [rsp+96],  ymm9
    mov  r12, rcx
    mov  r13, rdx
    mov  rcx, r8
    vpxor  xmm0, xmm0, xmm0
    vpxor  ymm6, ymm6, ymm6
    vpxor  ymm7, ymm7, ymm7
    vpxor  ymm8, ymm8, ymm8
    vpxor  ymm9, ymm9, ymm9
.dotp_f64_loop16:
    cmp  rcx, 16
    jb   .dotp_f64_tail
    vmovupd ymm0, [r12]       ; a0-a3
    vmovupd ymm1, [r12+32]    ; a4-a7
    vmovupd ymm2, [r12+64]    ; a8-a11
    vmovupd ymm3, [r12+96]    ; a12-a15
    vmovupd ymm4, [r13]       ; b0-b3
    vfmadd231pd ymm6, ymm0, ymm4 ; acc0 += a0..3 * b0..3
    vmovupd ymm4, [r13+32]    ; b4-b7
    vfmadd231pd ymm7, ymm1, ymm4 ; acc1 += a4..7 * b4..7
    vmovupd ymm4, [r13+64]    ; b8-b11
    vfmadd231pd ymm8, ymm2, ymm4 ; acc2 += a8..11 * b8..11
    vmovupd ymm4, [r13+96]    ; b12-b15
    vfmadd231pd ymm9, ymm3, ymm4 ; acc3 += a12..15 * b12..15
    add  r12, 128
    add  r13, 128
    sub  rcx, 16
    jmp  .dotp_f64_loop16
.dotp_f64_tail:
    test rcx, rcx
    jz   .dotp_f64_accum
.dotp_f64_tail_loop:
    vmovsd  xmm1, [r12]
    vfmadd231sd xmm0, xmm1, [r13]
    add  r12, 8
    add  r13, 8
    dec  rcx
    jnz  .dotp_f64_tail_loop
.dotp_f64_accum:
    vaddpd ymm6, ymm6, ymm7
    vaddpd ymm8, ymm8, ymm9
    vaddpd ymm6, ymm6, ymm8
    vextractf128 xmm1, ymm6, 1
    vaddpd       xmm2, xmm6, xmm1
    vpermilpd    xmm1, xmm2, 1
    vaddpd       xmm2, xmm2, xmm1
    vaddsd xmm0, xmm0, xmm2
.dotp_f64_done:
    vmovdqa ymm6, [rsp]
    vmovdqa ymm7, [rsp+32]
    vmovdqa ymm8, [rsp+64]
    vmovdqa ymm9, [rsp+96]
    mov  rsp, r11
    vzeroupper
    pop  r13
    pop  r12
    pop  r11
    ret

; =============================================================================
; int64_t fp_fold_sad_i64(const int64_t* a, const int64_t* b, size_t n)
; *** Corrected v5: Use AVX2 compare instructions for abs(i64) ***
; Strategy: SIMD Match. AVX2, 4 accumulators, 16 elements/loop.
; =============================================================================
fp_fold_sad_i64:
    push r11
    push r12
    push r13
    mov  r11, rsp
    and  rsp, 0xFFFFFFFFFFFFFFE0
    sub  rsp, 128
    vmovdqa [rsp],     ymm6
    vmovdqa [rsp+32],  ymm7
    vmovdqa [rsp+64],  ymm8
    vmovdqa [rsp+96],  ymm9
    
    mov  r12, rcx       ; r12 = a
    mov  r13, rdx       ; r13 = b
    mov  rcx, r8        ; rcx = n
    
    xor  rax, rax       ; scalar_acc = 0
    
    ; Zero out vector accumulators and scratch register ymm5
    vpxor  ymm5, ymm5, ymm5 ; ymm5 will hold ZERO for compare
    vpxor  ymm6, ymm6, ymm6 ; acc0
    vpxor  ymm7, ymm7, ymm7 ; acc1
    vpxor  ymm8, ymm8, ymm8 ; acc2
    vpxor  ymm9, ymm9, ymm9 ; acc3

.sad_loop16:
    cmp  rcx, 16
    jb   .sad_tail

    ; Process 4 vectors (16 elements) per iteration
    
    ; --- Vector 0 ---
    vmovdqu ymm0, [r12]          ; ymm0 = a[i..i+3]
    vpsubq  ymm0, ymm0, [r13]    ; ymm0 = diff = a - b
    vpcmpgtq ymm4, ymm5, ymm0    ; ymm4 = mask = (0 > diff) ? -1 : 0
    vpxor   ymm0, ymm0, ymm4     ; XOR with mask (flips if negative)
    vpsubq  ymm0, ymm0, ymm4     ; SUB mask (adds 1 if negative) -> abs
    vpaddq  ymm6, ymm6, ymm0     ; acc0 += abs(diff)
    
    ; --- Vector 1 ---
    vmovdqu ymm1, [r12+32]       ; ymm1 = a[i+4..i+7]
    vpsubq  ymm1, ymm1, [r13+32] ; ymm1 = diff
    vpcmpgtq ymm4, ymm5, ymm1    ; ymm4 = mask
    vpxor   ymm1, ymm1, ymm4
    vpsubq  ymm1, ymm1, ymm4
    vpaddq  ymm7, ymm7, ymm1     ; acc1 += abs(diff)
    
    ; --- Vector 2 ---
    vmovdqu ymm2, [r12+64]       ; ymm2 = a[i+8..i+11]
    vpsubq  ymm2, ymm2, [r13+64] ; ymm2 = diff
    vpcmpgtq ymm4, ymm5, ymm2    ; ymm4 = mask
    vpxor   ymm2, ymm2, ymm4
    vpsubq  ymm2, ymm2, ymm4
    vpaddq  ymm8, ymm8, ymm2     ; acc2 += abs(diff)
    
    ; --- Vector 3 ---
    vmovdqu ymm3, [r12+96]       ; ymm3 = a[i+12..i+15]
    vpsubq  ymm3, ymm3, [r13+96] ; ymm3 = diff
    vpcmpgtq ymm4, ymm5, ymm3    ; ymm4 = mask
    vpxor   ymm3, ymm3, ymm4
    vpsubq  ymm3, ymm3, ymm4
    vpaddq  ymm9, ymm9, ymm3     ; acc3 += abs(diff)

    add  r12, 128
    add  r13, 128
    sub  rcx, 16
    jmp  .sad_loop16
    
.sad_tail:
    test rcx, rcx
    jz   .sad_accum
.sad_tail_loop:
    mov  r8, [r12]
    sub  r8, [r13]
    mov  r9, r8
    sar  r9, 63     ; mask = (diff >> 63)
    xor  r8, r9     ; XOR with mask
    sub  r8, r9     ; SUB mask -> abs
    add  rax, r8
    add  r12, 8
    add  r13, 8
    dec  rcx
    jnz  .sad_tail_loop
    
.sad_accum:
    vpaddq ymm6, ymm6, ymm7
    vpaddq ymm8, ymm8, ymm9
    vpaddq ymm6, ymm6, ymm8
    vextractf128 xmm1, ymm6, 1
    vpaddq       xmm0, xmm6, xmm1
    vpermilpd    xmm1, xmm0, 1
    vpaddq       xmm0, xmm0, xmm1
    vmovq r10, xmm0
    add   rax, r10
    
.sad_done:
    vmovdqa ymm6, [rsp]
    vmovdqa ymm7, [rsp+32]
    vmovdqa ymm8, [rsp+64]
    vmovdqa ymm9, [rsp+96]
    mov  rsp, r11
    vzeroupper
    pop  r13
    pop  r12
    pop  r11
    ret

