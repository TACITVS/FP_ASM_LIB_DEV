; =============================================================================
; Win64 x86-64 Assembly Core Functions (NASM)
; =============================================================================
;
; Calling Convention:
; - 1st arg: RCX
; - 2nd arg: RDX
; - 3rd arg: R8
; - 4th arg: R9
; - Return value: RAX
;
; Callee-Save Registers (must be preserved):
; - RBX, RBP, RDI, RSI, R12, R13, R14, R15
; - XMM6 - XMM15 (and YMM6 - YMM15)
;
; Volatile Registers (can be modified):
; - RAX, RCX, RDX, R8, R9, R10, R11
; - XMM0 - XMM5 (and YMM0 - YMM5)

default rel
section .text
    ; Export functions for the C linker
    global fp_map_square_i64
    global fp_reduce_add_i64
    global fp_foldmap_sumsq_i64

; -----------------------------------------------------------------------------
; size_t fp_map_square_i64(const int64_t* in, int64_t* out, size_t n)
; (Scalar, unrolled x4 - Unchanged)
; -----------------------------------------------------------------------------
fp_map_square_i64:
    ; --- Prologue ---
    push r12
    push r13
    push r14
    sub  rsp, 32      ; Allocate 32-byte shadow space

    ; --- Initialization ---
    mov  r12, rcx     ; r12 = in
    mov  r13, rdx     ; r13 = out
    mov  r14, r8      ; r14 = n (preserved for return value)
    mov  rax, r8      ; Use RAX for the loop counter 'n'

.map_sq_loop4:
    cmp  rax, 4
    jb   .map_sq_tail
    mov  r9, [r12]
    imul r9, r9
    mov  [r13], r9
    mov  r10, [r12+8]
    imul r10, r10
    mov  [r13+8], r10
    mov  r11, [r12+16]
    imul r11, r11
    mov  [r13+16], r11
    mov  r9, [r12+24]
    imul r9, r9
    mov  [r13+24], r9
    add  r12, 32
    add  r13, 32
    sub  rax, 4
    jmp  .map_sq_loop4
.map_sq_tail:
    test rax, rax
    jz   .map_sq_done
.map_sq_tail_loop:
    mov  r9, [r12]
    imul r9, r9
    mov  [r13], r9
    add  r12, 8
    add  r13, 8
    dec  rax
    jnz  .map_sq_tail_loop
.map_sq_done:
    mov  rax, r14
    add  rsp, 32
    pop  r14
    pop  r13
    pop  r12
    ret

; -----------------------------------------------------------------------------
; int64_t fp_reduce_add_i64(const int64_t* a, size_t n, int64_t init)
; RCX=a, RDX=n, R8=init     -> RAX = sum
; AVX2 SIMD implementation.
;
; *** BUG FIX ***
; - r11 is used to save RSP.
; - The horizontal sum MUST use a different scratch register (like r10)
;   to avoid corrupting the saved RSP.
; -----------------------------------------------------------------------------
fp_reduce_add_i64:
    ; --- Prologue ---
    push r11          ; Use r11 to save pre-alignment RSP
    push r12
    push r13
    
    mov  r11, rsp     ; Save the current, non-32-byte-aligned RSP

    ; Allocate shadow space AND align RSP down to 32-byte boundary
    sub  rsp, 32      ; Allocate 32-byte shadow space
    and  rsp, 0xFFFFFFFFFFFFFFE0 ; Align RSP down to nearest 32-byte boundary
    
    ; Allocate 128 bytes for 4 YMM registers on the 32-byte aligned stack
    sub  rsp, 128
    
    ; Save non-volatile YMM registers to our aligned stack area
    vmovdqa [rsp],     ymm6
    vmovdqa [rsp+32],  ymm7
    vmovdqa [rsp+64],  ymm8
    vmovdqa [rsp+96],  ymm9

    ; --- Initialization ---
    mov  r12, rcx       ; r12 = a (pointer)
    mov  r13, rdx       ; r13 = n (count)
    mov  rax, r8        ; rax = init (scalar accumulator for tail)
    
    ; Zero out the 4 vector accumulators
    vpxor  ymm6, ymm6, ymm6  ; acc0
    vpxor  ymm7, ymm7, ymm7  ; acc1
    vpxor  ymm8, ymm8, ymm8  ; acc2
    vpxor  ymm9, ymm9, ymm9  ; acc3

.red_add_loop16:
    cmp  r13, 16
    jb   .red_add_tail
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
    jmp  .red_add_loop16

.red_add_tail:
    test r13, r13
    jz   .red_add_accum
.red_add_tail_loop:
    add  rax, [r12]
    add  r12, 8
    dec  r13
    jnz  .red_add_tail_loop

.red_add_accum:
    ; Combine the 4 vector accumulators
    vpaddq ymm6, ymm6, ymm7
    vpaddq ymm8, ymm8, ymm9
    vpaddq ymm6, ymm6, ymm8

    ; --- Horizontal Sum ---
    vextractf128 xmm1, ymm6, 1
    vpaddq       xmm0, xmm6, xmm1
    vpermilpd    xmm1, xmm0, 1
    vpaddq       xmm0, xmm0, xmm1

    ; *** THE FIX ***
    ; Use r10 (volatile) not r11 (which holds our stack pointer)
    vmovq r10, xmm0
    add   rax, r10

.red_add_done:
    ; --- Epilogue ---
    vmovdqa ymm6, [rsp]
    vmovdqa ymm7, [rsp+32]
    vmovdqa ymm8, [rsp+64]
    vmovdqa ymm9, [rsp+96]
    
    ; Restore the *original* stack pointer from r11
    mov  rsp, r11
    
    vzeroupper
    pop  r13
    pop  r12
    pop  r11
    ret

; -----------------------------------------------------------------------------
; int64_t fp_foldmap_sumsq_i64(const int64_t* in, size_t n, int64_t init)
; (Scalar, unrolled x8 - Unchanged)
; -----------------------------------------------------------------------------
fp_foldmap_sumsq_i64:
    ; --- Prologue ---
    push rbx
    push r12
    push r13
    push r14
    push r15
    push rsi
    push rdi
    sub  rsp, 32

    ; --- Initialization ---
    mov  r12, rcx
    mov  r13, rdx
    mov  rax, r8
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