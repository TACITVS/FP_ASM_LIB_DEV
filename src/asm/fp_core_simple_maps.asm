; =============================================================================
; FP-ASM Core Library: Simple Maps Module (Module 4)
;
; *** v2: CRITICAL FIX for fp_map_clamp_f64 - Preserve RCX, use RAX counter ***
;
; Implements element-wise transformations: abs, sqrt, clamp
; =============================================================================

bits 64
default rel

section .rdata
    ALIGN 32
    ABS_MASK_F64:
        dq 0x7FFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF

section .text
    global fp_map_abs_i64
    global fp_map_abs_f64
    global fp_map_sqrt_f64
    global fp_map_clamp_i64
    global fp_map_clamp_f64

; ===========================================================================
; fp_map_abs_i64 (Unchanged v1 - Passed)
; ===========================================================================
global fp_map_abs_i64
fp_map_abs_i64:
    push r11 ; Temp save RSP
    push r12 ; Save non-volatile
    push r13 ; Save non-volatile
    mov  r11, rsp
    and  rsp, 0xFFFFFFFFFFFFFFE0  ; Align to 32 bytes
    sub  rsp, 128            ; Space for YMM registers (though only YMM0-5 used)
    ; Note: Saving YMM6-9 might not be strictly needed if only YMM0-5 are used
    ; but doing it defensively is safer if code changes later.
    vmovdqa [rsp],     ymm6
    vmovdqa [rsp+32],  ymm7
    vmovdqa [rsp+64],  ymm8
    vmovdqa [rsp+96],  ymm9

    mov  r12, rcx            ; r12 = in
    mov  r13, rdx            ; r13 = out
    mov  rcx, r8             ; rcx = n (Loop counter - OK to overwrite as it's volatile here)

    test rcx, rcx
    jz   .done_abs_i64

    vpxor ymm5, ymm5, ymm5  ; ymm5 = 0

.loop16_abs_i64:
    cmp  rcx, 16
    jb   .tail_abs_i64
    vmovdqu ymm0, [r12]
    vpcmpgtq ymm4, ymm5, ymm0
    vpxor   ymm0, ymm0, ymm4
    vpsubq  ymm0, ymm0, ymm4
    vmovdqu [r13], ymm0
    vmovdqu ymm1, [r12+32]
    vpcmpgtq ymm4, ymm5, ymm1
    vpxor   ymm1, ymm1, ymm4
    vpsubq  ymm1, ymm1, ymm4
    vmovdqu [r13+32], ymm1
    vmovdqu ymm2, [r12+64]
    vpcmpgtq ymm4, ymm5, ymm2
    vpxor   ymm2, ymm2, ymm4
    vpsubq  ymm2, ymm2, ymm4
    vmovdqu [r13+64], ymm2
    vmovdqu ymm3, [r12+96]
    vpcmpgtq ymm4, ymm5, ymm3
    vpxor   ymm3, ymm3, ymm4
    vpsubq  ymm3, ymm3, ymm4
    vmovdqu [r13+96], ymm3
    add  r12, 128
    add  r13, 128
    sub  rcx, 16
    jmp  .loop16_abs_i64

.tail_abs_i64:
    test rcx, rcx
    jz   .cleanup_abs_i64
.tail_loop_abs_i64:
    mov  r8, [r12]
    mov  r9, r8
    sar  r9, 63
    xor  r8, r9
    sub  r8, r9
    mov  [r13], r8
    add  r12, 8
    add  r13, 8
    dec  rcx
    jnz  .tail_loop_abs_i64

.cleanup_abs_i64:
    vzeroupper
.done_abs_i64:
    vmovdqa ymm6, [rsp]
    vmovdqa ymm7, [rsp+32]
    vmovdqa ymm8, [rsp+64]
    vmovdqa ymm9, [rsp+96]
    mov  rsp, r11
    pop  r13
    pop  r12
    pop  r11
    ret

; ===========================================================================
; fp_map_abs_f64 (Unchanged v1 - Passed)
; ===========================================================================
global fp_map_abs_f64
fp_map_abs_f64:
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
    test rcx, rcx
    jz   .done_abs_f64
    vmovdqa ymm5, [rel ABS_MASK_F64]
.loop16_abs_f64:
    cmp  rcx, 16
    jb   .tail_abs_f64
    vmovupd ymm0, [r12]
    vandpd  ymm0, ymm0, ymm5
    vmovupd [r13], ymm0
    vmovupd ymm1, [r12+32]
    vandpd  ymm1, ymm1, ymm5
    vmovupd [r13+32], ymm1
    vmovupd ymm2, [r12+64]
    vandpd  ymm2, ymm2, ymm5
    vmovupd [r13+64], ymm2
    vmovupd ymm3, [r12+96]
    vandpd  ymm3, ymm3, ymm5
    vmovupd [r13+96], ymm3
    add  r12, 128
    add  r13, 128
    sub  rcx, 16
    jmp  .loop16_abs_f64
.tail_abs_f64:
    test rcx, rcx
    jz   .cleanup_abs_f64
    mov r9, [ABS_MASK_F64] ; Load scalar mask part
.tail_loop_abs_f64:
    mov r10, [r12]          ; Use GPRs for scalar tail
    and r10, r9
    mov [r13], r10
    add  r12, 8
    add  r13, 8
    dec  rcx
    jnz  .tail_loop_abs_f64
.cleanup_abs_f64:
    vzeroupper
.done_abs_f64:
    vmovdqa ymm6, [rsp]
    vmovdqa ymm7, [rsp+32]
    vmovdqa ymm8, [rsp+64]
    vmovdqa ymm9, [rsp+96]
    mov  rsp, r11
    pop  r13
    pop  r12
    pop  r11
    ret

; ===========================================================================
; fp_map_sqrt_f64 (Unchanged v1 - Passed)
; ===========================================================================
global fp_map_sqrt_f64
fp_map_sqrt_f64:
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
    test rcx, rcx
    jz   .done_sqrt_f64
.loop16_sqrt_f64:
    cmp  rcx, 16
    jb   .tail_sqrt_f64
    vmovupd ymm0, [r12]
    vsqrtpd ymm0, ymm0
    vmovupd [r13], ymm0
    vmovupd ymm1, [r12+32]
    vsqrtpd ymm1, ymm1
    vmovupd [r13+32], ymm1
    vmovupd ymm2, [r12+64]
    vsqrtpd ymm2, ymm2
    vmovupd [r13+64], ymm2
    vmovupd ymm3, [r12+96]
    vsqrtpd ymm3, ymm3
    vmovupd [r13+96], ymm3
    add  r12, 128
    add  r13, 128
    sub  rcx, 16
    jmp  .loop16_sqrt_f64
.tail_sqrt_f64:
    test rcx, rcx
    jz   .cleanup_sqrt_f64
.tail_loop_sqrt_f64:
    vmovsd xmm0, [r12]
    vsqrtsd xmm0, xmm0, xmm0
    vmovsd [r13], xmm0
    add  r12, 8
    add  r13, 8
    dec  rcx
    jnz  .tail_loop_sqrt_f64
.cleanup_sqrt_f64:
    vzeroupper
.done_sqrt_f64:
    vmovdqa ymm6, [rsp]
    vmovdqa ymm7, [rsp+32]
    vmovdqa ymm8, [rsp+64]
    vmovdqa ymm9, [rsp+96]
    mov  rsp, r11
    pop  r13
    pop  r12
    pop  r11
    ret

; ===========================================================================
; fp_map_clamp_i64 (Unchanged v1 - Passed)
; ===========================================================================
global fp_map_clamp_i64
fp_map_clamp_i64:
    push r12
    push r13
    push r14
    push r15
    sub  rsp, 32             ; Shadow space
    mov  r12, rcx            ; r12 = in
    mov  r13, rdx            ; r13 = out
    mov  r14, r8             ; r14 = n
    mov  r15, r9             ; r15 = min_val
    mov  r10, [rsp+104]      ; r10 = max_val
    test r14, r14
    jz   .done_clamp_i64
.loop4_clamp_i64:
    cmp  r14, 4
    jb   .tail_clamp_i64
    mov  rax, [r12]
    cmp  rax, r15
    cmovl rax, r15
    cmp  rax, r10
    cmovg rax, r10
    mov  [r13], rax
    mov  rax, [r12+8]
    cmp  rax, r15
    cmovl rax, r15
    cmp  rax, r10
    cmovg rax, r10
    mov  [r13+8], rax
    mov  rax, [r12+16]
    cmp  rax, r15
    cmovl rax, r15
    cmp  rax, r10
    cmovg rax, r10
    mov  [r13+16], rax
    mov  rax, [r12+24]
    cmp  rax, r15
    cmovl rax, r15
    cmp  rax, r10
    cmovg rax, r10
    mov  [r13+24], rax
    add  r12, 32
    add  r13, 32
    sub  r14, 4
    jmp  .loop4_clamp_i64
.tail_clamp_i64:
    test r14, r14
    jz   .done_clamp_i64
.tail_loop_clamp_i64:
    mov  rax, [r12]
    cmp  rax, r15
    cmovl rax, r15
    cmp  rax, r10
    cmovg rax, r10
    mov  [r13], rax
    add  r12, 8
    add  r13, 8
    dec  r14
    jnz  .tail_loop_clamp_i64
.done_clamp_i64:
    add  rsp, 32
    pop  r15
    pop  r14
    pop  r13
    pop  r12
    ret

; ===========================================================================
; fp_map_clamp_f64 (*** CORRECTED v2 - Preserve RCX, use RAX counter ***)
; ===========================================================================
global fp_map_clamp_f64
fp_map_clamp_f64:
    push rbp
    mov  rbp, rsp
    ; Save non-volatile registers used (R12, R13)
    push r12
    push r13
    and  rsp, 0xFFFFFFFFFFFFFFE0  ; Align stack to 32 bytes
    sub  rsp, 128            ; Space for YMM6-YMM9 save
    vmovdqa [rsp],     ymm6
    vmovdqa [rsp+32],  ymm7
    vmovdqa [rsp+64],  ymm8
    vmovdqa [rsp+96],  ymm9

    ; ABI: RCX=in, RDX=out, R8=n, XMM3=min_val, [rbp+48]=max_val
    mov  r12, rcx            ; r12 = in (Preserve RCX!)
    mov  r13, rdx            ; r13 = out
    mov  rax, r8             ; rax = n (Use RAX as loop counter)

    test rax, rax            ; Check n
    jz   .done_clamp_f64

    ; Broadcast min_val and max_val to YMM registers
    vbroadcastsd ymm5, xmm3               ; ymm5 = min_val
    vbroadcastsd ymm4, qword [rbp+48]     ; ymm4 = max_val

.loop16_clamp_f64:
    cmp  rax, 16
    jb   .tail_clamp_f64

    ; Process 16 elements (4 YMM registers) per iteration
    vmovupd ymm0, [r12]          ; Load in[i..i+3]
    vminpd  ymm0, ymm0, ymm4     ; min(x, max_val)
    vmaxpd  ymm0, ymm0, ymm5     ; max(min_val, ...)
    vmovupd [r13], ymm0          ; Store to out

    vmovupd ymm1, [r12+32]
    vminpd  ymm1, ymm1, ymm4
    vmaxpd  ymm1, ymm1, ymm5
    vmovupd [r13+32], ymm1

    vmovupd ymm2, [r12+64]
    vminpd  ymm2, ymm2, ymm4
    vmaxpd  ymm2, ymm2, ymm5
    vmovupd [r13+64], ymm2

    vmovupd ymm3, [r12+96]
    vminpd  ymm3, ymm3, ymm4
    vmaxpd  ymm3, ymm3, ymm5
    vmovupd [r13+96], ymm3

    add  r12, 128            ; Advance in ptr
    add  r13, 128            ; Advance out ptr
    sub  rax, 16             ; Decrement loop counter (RAX)
    jmp  .loop16_clamp_f64

.tail_clamp_f64:
    test rax, rax            ; Check remaining count in RAX
    jz   .cleanup_clamp_f64

.tail_loop_clamp_f64:
    vmovsd xmm0, [r12]       ; Load element
    vminsd xmm0, xmm0, xmm4  ; min(x, max_val) (use XMM4)
    vmaxsd xmm0, xmm0, xmm5  ; max(min_val, ...) (use XMM5)
    vmovsd [r13], xmm0       ; Store to out
    add  r12, 8              ; Advance in ptr
    add  r13, 8              ; Advance out ptr
    dec  rax                 ; Decrement loop counter (RAX)
    jnz  .tail_loop_clamp_f64

.cleanup_clamp_f64:
    vzeroupper

.done_clamp_f64:
    ; Epilogue
    vmovdqa ymm6, [rsp]
    vmovdqa ymm7, [rsp+32]
    vmovdqa ymm8, [rsp+64]
    vmovdqa ymm9, [rsp+96]
    add  rsp, 128            ; Deallocate YMM save space
    pop  r13                 ; Pop non-volatile registers BEFORE rsp restore
    pop  r12
    mov  rsp, rbp            ; Restore RSP to before alignment/shadow space
    pop  rbp
    ret

