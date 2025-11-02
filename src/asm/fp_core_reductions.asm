; =============================================================================
; FP-ASM Core Library: Reductions Module (Module 1)
;
; *** STABLE v10 ***
; - `fp_reduce_max_i64` v10 uses 4-way unroll with broken dependencies.
;   This matches/beats the compiler.
; =============================================================================
default rel

section .rdata
    ALIGN 32
    NEG_INF_F64:
        dq 0xFFF0000000000000, 0xFFF0000000000000, 0xFFF0000000000000, 0xFFF0000000000000
    POS_INF_F64:
        dq 0x7FF0000000000000, 0x7FF0000000000000, 0x7FF0000000000000, 0x7FF0000000000000
    MAX_I64:
        dq 0x7FFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF

section .text
    global fp_reduce_add_i64
    global fp_reduce_add_f64
    global fp_reduce_max_i64
    global fp_reduce_max_f64
    global fp_reduce_min_i64
    global fp_reduce_min_f64

; =============================================================================
; int64_t fp_reduce_add_i64(const int64_t* in, size_t n)
; (Unchanged - Performed great)
; =============================================================================
fp_reduce_add_i64:
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
    xor  rax, rax
    vpxor  ymm6, ymm6, ymm6
    vpxor  ymm7, ymm7, ymm7
    vpxor  ymm8, ymm8, ymm8
    vpxor  ymm9, ymm9, ymm9
.loop16:
    cmp  r13, 16
    jb   .tail
    vmovdqu ymm0, [r12]
    vmovdqu ymm1, [r12+32]
    vmovdqu ymm2, [r12+64]
    vmovdqu ymm3, [r12+96]
    vpaddq  ymm6, ymm6, ymm0
    vpaddq  ymm7, ymm7, ymm1
    vpaddq  ymm8, ymm8, ymm2
    vpaddq  ymm9, ymm9, ymm3
    add  r12, 128
    sub  r13, 16
    jmp  .loop16
.tail:
    test r13, r13
    jz   .accum
.tail_loop:
    add  rax, [r12]
    add  r12, 8
    dec  r13
    jnz  .tail_loop
.accum:
    vpaddq ymm6, ymm6, ymm7
    vpaddq ymm8, ymm8, ymm9
    vpaddq ymm6, ymm6, ymm8
    vextractf128 xmm1, ymm6, 1
    vpaddq       xmm0, xmm6, xmm1
    vpermilpd    xmm1, xmm0, 1
    vpaddq       xmm0, xmm0, xmm1
    vmovq r10, xmm0
    add   rax, r10
.done:
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
; double fp_reduce_add_f64(const double* in, size_t n)
; (Unchanged - Performed great)
; =============================================================================
fp_reduce_add_f64:
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
    vpxor  xmm0, xmm0, xmm0
    vpxor  ymm6, ymm6, ymm6
    vpxor  ymm7, ymm7, ymm7
    vpxor  ymm8, ymm8, ymm8
    vpxor  ymm9, ymm9, ymm9
.loop16:
    cmp  r13, 16
    jb   .tail
    vmovupd ymm1, [r12]
    vmovupd ymm2, [r12+32]
    vmovupd ymm3, [r12+64]
    vmovupd ymm4, [r12+96]
    vaddpd  ymm6, ymm6, ymm1
    vaddpd  ymm7, ymm7, ymm2
    vaddpd  ymm8, ymm8, ymm3
    vaddpd  ymm9, ymm9, ymm4
    add  r12, 128
    sub  r13, 16
    jmp  .loop16
.tail:
    test r13, r13
    jz   .accum
.tail_loop:
    vaddsd xmm0, xmm0, [r12]
    add  r12, 8
    dec  r13
    jnz  .tail_loop
.accum:
    vaddpd ymm6, ymm6, ymm7
    vaddpd ymm8, ymm8, ymm9
    vaddpd ymm6, ymm6, ymm8
    vextractf128 xmm1, ymm6, 1
    vaddpd       xmm2, xmm6, xmm1
    vpermilpd    xmm1, xmm2, 1
    vaddpd       xmm2, xmm2, xmm1
    vaddsd xmm0, xmm0, xmm2
.done:
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
; int64_t fp_reduce_max_i64(const int64_t* in, size_t n)
; *** STABLE V10 ***
; Strategy: Scalar. 4-way unroll with 4 accumulators, broken dependencies.
; This matches/beats the compiler.
; =============================================================================
fp_reduce_max_i64:
    push r12
    push r13
    push rsi          ; <-- Save rsi for use as scratch
    sub  rsp, 32
    mov  r12, rcx     ; in
    mov  r13, rdx     ; n
    
    test r13, r13     ; Check for n=0
    jz   .done_zero_scalar
    
    ; Load first element as initial max for all 4 accumulators
    mov  rax, [r12]   ; acc0
    mov  r8, rax      ; acc1
    mov  r9, rax      ; acc2
    mov  r10, rax     ; acc3
    add  r12, 8
    dec  r13
    
.loop4:
    cmp  r13, 4
    jb   .tail
    
    mov  r11, [r12]   ; scratch 1
    cmp  rax, r11
    cmovl rax, r11

    mov  rcx, [r12+8] ; scratch 2
    cmp  r8, rcx
    cmovl r8, rcx

    mov  rdx, [r12+16] ; scratch 3
    cmp  r9, rdx
    cmovl r9, rdx
    
    mov  rsi, [r12+24] ; scratch 4
    cmp  r10, rsi
    cmovl r10, rsi
    
    add  r12, 32
    sub  r13, 4
    jmp  .loop4
    
.tail:
    test r13, r13
    jz   .accum
.tail_loop:
    mov  r11, [r12]
    cmp  rax, r11
    cmovl rax, r11    ; Fold remaining 1-3 into acc0
    add  r12, 8
    dec  r13
    jnz  .tail_loop
    
.accum:
    ; Combine the 4 accumulators
    cmp  rax, r8
    cmovl rax, r8
    cmp  rax, r9
    cmovl rax, r9
    cmp  rax, r10
    cmovl rax, r10
    
.done:
    add  rsp, 32
    pop  rsi          ; <-- Restore rsi
    pop  r13
    pop  r12
    ret
.done_zero_scalar:
    xor  rax, rax
    add  rsp, 32
    pop  rsi          ; <-- Restore rsi
    pop  r13
    pop  r12
    ret


; =============================================================================
; double fp_reduce_max_f64(const double* in, size_t n)
; (Unchanged - Performed great)
; =============================================================================
fp_reduce_max_f64:
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
    test r13, r13
    jz   .done_zero_vec
    
    vmovdqa ymm6, [rel NEG_INF_F64]
    vmovdqa ymm7, [rel NEG_INF_F64]
    vmovdqa ymm8, [rel NEG_INF_F64]
    vmovdqa ymm9, [rel NEG_INF_F64]
    vmovdqa xmm0, [rel NEG_INF_F64]
.loop16:
    cmp  r13, 16
    jb   .tail
    vmovupd ymm1, [r12]
    vmovupd ymm2, [r12+32]
    vmovupd ymm3, [r12+64]
    vmovupd ymm4, [r12+96]
    vmaxpd  ymm6, ymm6, ymm1
    vmaxpd  ymm7, ymm7, ymm2
    vmaxpd  ymm8, ymm8, ymm3
    vmaxpd  ymm9, ymm9, ymm4
    add  r12, 128
    sub  r13, 16
    jmp  .loop16
.tail:
    test r13, r13
    jz   .accum
.tail_loop:
    vmaxsd xmm0, xmm0, [r12]
    add  r12, 8
    dec  r13
    jnz  .tail_loop
.accum:
    vmaxpd ymm6, ymm6, ymm7
    vmaxpd ymm8, ymm8, ymm9
    vmaxpd ymm6, ymm6, ymm8
    vextractf128 xmm1, ymm6, 1
    vmaxpd       xmm2, xmm6, xmm1
    vpermilpd    xmm1, xmm2, 1
    vmaxpd       xmm2, xmm2, xmm1
    vmaxsd xmm0, xmm0, xmm2
.done:
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
.done_zero_vec:
    vpxor  xmm0, xmm0, xmm0
    mov  rsp, r11
    vzeroupper
    pop  r13
    pop  r12
    pop  r11
    ret

; =============================================================================
; double fp_reduce_min_f64(const double* in, size_t n)
; Find minimum value in array using SIMD
; =============================================================================
fp_reduce_min_f64:
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
    test r13, r13
    jz   .done_zero_vec

    vmovdqa ymm6, [rel POS_INF_F64]
    vmovdqa ymm7, [rel POS_INF_F64]
    vmovdqa ymm8, [rel POS_INF_F64]
    vmovdqa ymm9, [rel POS_INF_F64]
    vmovdqa xmm0, [rel POS_INF_F64]
.loop16:
    cmp  r13, 16
    jb   .tail
    vmovupd ymm1, [r12]
    vmovupd ymm2, [r12+32]
    vmovupd ymm3, [r12+64]
    vmovupd ymm4, [r12+96]
    vminpd  ymm6, ymm6, ymm1
    vminpd  ymm7, ymm7, ymm2
    vminpd  ymm8, ymm8, ymm3
    vminpd  ymm9, ymm9, ymm4
    add  r12, 128
    sub  r13, 16
    jmp  .loop16
.tail:
    test r13, r13
    jz   .accum
.tail_loop:
    vminsd xmm0, xmm0, [r12]
    add  r12, 8
    dec  r13
    jnz  .tail_loop
.accum:
    vminpd ymm6, ymm6, ymm7
    vminpd ymm8, ymm8, ymm9
    vminpd ymm6, ymm6, ymm8
    vextractf128 xmm1, ymm6, 1
    vminpd       xmm2, xmm6, xmm1
    vpermilpd    xmm1, xmm2, 1
    vminpd       xmm2, xmm2, xmm1
    vminsd xmm0, xmm0, xmm2
.done:
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
.done_zero_vec:
    vpxor  xmm0, xmm0, xmm0
    mov  rsp, r11
    vzeroupper
    pop  r13
    pop  r12
    pop  r11
    ret

; =============================================================================
; int64_t fp_reduce_min_i64(const int64_t* in, size_t n)
; Find minimum value in array (scalar, AVX2 lacks vpminsq)
; =============================================================================
fp_reduce_min_i64:
    push r11
    push r12
    push r13
    mov  r11, rsp
    mov  r12, rcx
    mov  r13, rdx

    test r13, r13
    jz   .done_zero

    ; Initialize min to first element
    mov rax, [r12]
    dec r13
    jz  .done
    add r12, 8

    ; Scalar loop (AVX2 has no vpminsq)
.loop:
    mov r10, [r12]
    cmp r10, rax
    cmovl rax, r10
    add r12, 8
    dec r13
    jnz .loop

.done:
    mov rsp, r11
    pop r13
    pop r12
    pop r11
    ret

.done_zero:
    xor rax, rax
    mov rsp, r11
    pop r13
    pop r12
    pop r11
    ret

