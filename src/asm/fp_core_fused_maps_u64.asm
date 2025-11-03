; fp_core_fused_maps_u64.asm
;
; Hand-optimized x64 AVX2 SIMD implementations for u64 fused maps
; Windows x64 calling convention
;
; Performance target: Memory-bound operations with guaranteed SIMD saturation
;
; Functions:
;   - fp_map_axpy_u64:    out = c*x + y (scalar multiply, SIMD add)
;   - fp_map_scale_u64:   out = c*x (scalar multiply)
;   - fp_map_offset_u64:  out = x + c (SIMD add)
;   - fp_zip_add_u64:     out = x + y (SIMD add)

bits 64
default rel

section .text

; ============================================================================
; fp_map_axpy_u64: AXPY operation (out = c*x + y)
; ============================================================================
; Signature: void fp_map_axpy_u64(const uint64_t* x, const uint64_t* y, uint64_t* out, size_t n, uint64_t c);
;
; No SIMD i64 multiply, use scalar multiply with SIMD add

global fp_map_axpy_u64
fp_map_axpy_u64:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; c is 5th arg - on stack at [rbp + 48]
    mov r15, [rbp + 48]

    mov r12, rcx        ; x
    mov r13, rdx        ; y
    mov r14, r8         ; out
    mov rcx, r9         ; n

.loop4:
    cmp rcx, 4
    jb .loop1

    ; Scalar multiply
    mov rax, [r12]
    imul rax, r15
    mov r10, [r12 + 8]
    imul r10, r15
    mov r11, [r12 + 16]
    imul r11, r15
    mov rbx, [r12 + 24]
    imul rbx, r15

    ; Load into YMM
    vpxor ymm0, ymm0, ymm0
    vpinsrq xmm0, xmm0, rax, 0
    vpinsrq xmm0, xmm0, r10, 1
    vinserti128 ymm0, ymm0, xmm0, 0
    vpxor ymm1, ymm1, ymm1
    vpinsrq xmm1, xmm1, r11, 0
    vpinsrq xmm1, xmm1, rbx, 1
    vinserti128 ymm0, ymm0, xmm1, 1

    ; SIMD add with y
    vmovdqu ymm1, [r13]
    vpaddq ymm0, ymm0, ymm1

    vmovdqu [r14], ymm0

    add r12, 32
    add r13, 32
    add r14, 32
    sub rcx, 4
    jmp .loop4

.loop1:
    test rcx, rcx
    jz .cleanup

.loop1_iter:
    mov rax, [r12]
    imul rax, r15
    add rax, [r13]
    mov [r14], rax

    add r12, 8
    add r13, 8
    add r14, 8
    dec rcx
    jnz .loop1_iter

.cleanup:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_map_scale_u64: Scale operation (out = c*x)
; ============================================================================
; Signature: void fp_map_scale_u64(const uint64_t* x, uint64_t* out, size_t n, uint64_t c);
;
; No SIMD i64 multiply, use scalar

global fp_map_scale_u64
fp_map_scale_u64:
    push rbp
    mov rbp, rsp

    ; c is 4th arg - in R9
    mov r10, r9

    mov r12, rcx        ; x
    mov r13, rdx        ; out
    mov rcx, r8         ; n

.loop4:
    cmp rcx, 4
    jb .loop1

    mov rax, [r12]
    imul rax, r10
    mov [r13], rax

    mov rax, [r12 + 8]
    imul rax, r10
    mov [r13 + 8], rax

    mov rax, [r12 + 16]
    imul rax, r10
    mov [r13 + 16], rax

    mov rax, [r12 + 24]
    imul rax, r10
    mov [r13 + 24], rax

    add r12, 32
    add r13, 32
    sub rcx, 4
    jmp .loop4

.loop1:
    test rcx, rcx
    jz .cleanup

.loop1_iter:
    mov rax, [r12]
    imul rax, r10
    mov [r13], rax

    add r12, 8
    add r13, 8
    dec rcx
    jnz .loop1_iter

.cleanup:
    pop rbp
    ret

; ============================================================================
; fp_map_offset_u64: Offset operation (out = x + c)
; ============================================================================
; Signature: void fp_map_offset_u64(const uint64_t* x, uint64_t* out, size_t n, uint64_t c);

global fp_map_offset_u64
fp_map_offset_u64:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; c is 4th arg - in R9
    vpxor xmm0, xmm0, xmm0
    vpinsrq xmm0, xmm0, r9, 0
    vpbroadcastq ymm15, xmm0

    mov r12, rcx        ; x
    mov r13, rdx        ; out
    mov rcx, r8         ; n

.loop16:
    cmp rcx, 16
    jb .loop4

    vmovdqu ymm0, [r12]
    vmovdqu ymm1, [r12 + 32]
    vmovdqu ymm2, [r12 + 64]
    vmovdqu ymm3, [r12 + 96]

    vpaddq ymm0, ymm0, ymm15
    vpaddq ymm1, ymm1, ymm15
    vpaddq ymm2, ymm2, ymm15
    vpaddq ymm3, ymm3, ymm15

    vmovdqu [r13], ymm0
    vmovdqu [r13 + 32], ymm1
    vmovdqu [r13 + 64], ymm2
    vmovdqu [r13 + 96], ymm3

    add r12, 128
    add r13, 128
    sub rcx, 16
    jmp .loop16

.loop4:
    cmp rcx, 4
    jb .tail

    vmovdqu ymm0, [r12]
    vpaddq ymm0, ymm0, ymm15
    vmovdqu [r13], ymm0

    add r12, 32
    add r13, 32
    sub rcx, 4
    jmp .loop4

.tail:
    test rcx, rcx
    jz .cleanup

    vmovq r10, xmm15

.tail_loop:
    mov rax, [r12]
    add rax, r10
    mov [r13], rax

    add r12, 8
    add r13, 8
    dec rcx
    jnz .tail_loop

.cleanup:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_zip_add_u64: Zip add operation (out = x + y)
; ============================================================================
; Signature: void fp_zip_add_u64(const uint64_t* x, const uint64_t* y, uint64_t* out, size_t n);

global fp_zip_add_u64
fp_zip_add_u64:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    mov r12, rcx        ; x
    mov r13, rdx        ; y
    mov r14, r8         ; out
    mov rcx, r9         ; n

.loop16:
    cmp rcx, 16
    jb .loop4

    vmovdqu ymm0, [r12]
    vmovdqu ymm1, [r12 + 32]
    vmovdqu ymm2, [r12 + 64]
    vmovdqu ymm3, [r12 + 96]

    vmovdqu ymm4, [r13]
    vmovdqu ymm5, [r13 + 32]
    vmovdqu ymm6, [r13 + 64]
    vmovdqu ymm7, [r13 + 96]

    vpaddq ymm0, ymm0, ymm4
    vpaddq ymm1, ymm1, ymm5
    vpaddq ymm2, ymm2, ymm6
    vpaddq ymm3, ymm3, ymm7

    vmovdqu [r14], ymm0
    vmovdqu [r14 + 32], ymm1
    vmovdqu [r14 + 64], ymm2
    vmovdqu [r14 + 96], ymm3

    add r12, 128
    add r13, 128
    add r14, 128
    sub rcx, 16
    jmp .loop16

.loop4:
    cmp rcx, 4
    jb .tail

    vmovdqu ymm0, [r12]
    vmovdqu ymm4, [r13]
    vpaddq ymm0, ymm0, ymm4
    vmovdqu [r14], ymm0

    add r12, 32
    add r13, 32
    add r14, 32
    sub rcx, 4
    jmp .loop4

.tail:
    test rcx, rcx
    jz .cleanup

.tail_loop:
    mov rax, [r12]
    add rax, [r13]
    mov [r14], rax

    add r12, 8
    add r13, 8
    add r14, 8
    dec rcx
    jnz .tail_loop

.cleanup:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret
