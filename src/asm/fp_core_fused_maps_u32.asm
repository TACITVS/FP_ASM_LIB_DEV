; fp_core_fused_maps_u32.asm
;
; Hand-optimized x64 AVX2 SIMD implementations for u32 fused maps
; Windows x64 calling convention
;
; Performance target: Memory-bound operations with guaranteed SIMD saturation
;
; Functions:
;   - fp_map_axpy_u32:    out = c*x + y (BLAS axpy)
;   - fp_map_scale_u32:   out = c*x
;   - fp_map_offset_u32:  out = x + c
;   - fp_zip_add_u32:     out = x + y

bits 64
default rel

section .text

; ============================================================================
; fp_map_axpy_u32: AXPY operation (out = c*x + y)
; ============================================================================
; Signature: void fp_map_axpy_u32(const uint32_t* x, const uint32_t* y, uint32_t* out, size_t n, uint32_t c);
;
; Same as i32 - arithmetic is identical

global fp_map_axpy_u32
fp_map_axpy_u32:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Load c from stack (5th arg)
    mov eax, [rbp + 48]
    vpxor xmm0, xmm0, xmm0
    vpinsrd xmm0, xmm0, eax, 0
    vpbroadcastd ymm15, xmm0

    mov r12, rcx        ; x
    mov r13, rdx        ; y
    mov r14, r8         ; out
    mov rcx, r9         ; n

.loop32:
    cmp rcx, 32
    jb .loop8

    vmovdqu ymm0, [r12]
    vmovdqu ymm1, [r12 + 32]
    vmovdqu ymm2, [r12 + 64]
    vmovdqu ymm3, [r12 + 96]

    vmovdqu ymm4, [r13]
    vmovdqu ymm5, [r13 + 32]
    vmovdqu ymm6, [r13 + 64]
    vmovdqu ymm7, [r13 + 96]

    vpmulld ymm0, ymm0, ymm15
    vpmulld ymm1, ymm1, ymm15
    vpmulld ymm2, ymm2, ymm15
    vpmulld ymm3, ymm3, ymm15

    vpaddd ymm0, ymm0, ymm4
    vpaddd ymm1, ymm1, ymm5
    vpaddd ymm2, ymm2, ymm6
    vpaddd ymm3, ymm3, ymm7

    vmovdqu [r14], ymm0
    vmovdqu [r14 + 32], ymm1
    vmovdqu [r14 + 64], ymm2
    vmovdqu [r14 + 96], ymm3

    add r12, 128
    add r13, 128
    add r14, 128
    sub rcx, 32
    jmp .loop32

.loop8:
    cmp rcx, 8
    jb .tail

    vmovdqu ymm0, [r12]
    vmovdqu ymm4, [r13]
    vpmulld ymm0, ymm0, ymm15
    vpaddd ymm0, ymm0, ymm4
    vmovdqu [r14], ymm0

    add r12, 32
    add r13, 32
    add r14, 32
    sub rcx, 8
    jmp .loop8

.tail:
    test rcx, rcx
    jz .cleanup

    vmovd r10d, xmm15

.tail_loop:
    mov eax, [r12]
    imul eax, r10d
    add eax, [r13]
    mov [r14], eax

    add r12, 4
    add r13, 4
    add r14, 4
    dec rcx
    jnz .tail_loop

.cleanup:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_map_scale_u32: Scale operation (out = c*x)
; ============================================================================
; Signature: void fp_map_scale_u32(const uint32_t* x, uint32_t* out, size_t n, uint32_t c);

global fp_map_scale_u32
fp_map_scale_u32:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Load c from R9 (4th arg - still in register!)
    vpxor xmm0, xmm0, xmm0
    vpinsrd xmm0, xmm0, r9d, 0
    vpbroadcastd ymm15, xmm0

    mov r12, rcx        ; x
    mov r13, rdx        ; out
    mov rcx, r8         ; n

.loop32:
    cmp rcx, 32
    jb .loop8

    vmovdqu ymm0, [r12]
    vmovdqu ymm1, [r12 + 32]
    vmovdqu ymm2, [r12 + 64]
    vmovdqu ymm3, [r12 + 96]

    vpmulld ymm0, ymm0, ymm15
    vpmulld ymm1, ymm1, ymm15
    vpmulld ymm2, ymm2, ymm15
    vpmulld ymm3, ymm3, ymm15

    vmovdqu [r13], ymm0
    vmovdqu [r13 + 32], ymm1
    vmovdqu [r13 + 64], ymm2
    vmovdqu [r13 + 96], ymm3

    add r12, 128
    add r13, 128
    sub rcx, 32
    jmp .loop32

.loop8:
    cmp rcx, 8
    jb .tail

    vmovdqu ymm0, [r12]
    vpmulld ymm0, ymm0, ymm15
    vmovdqu [r13], ymm0

    add r12, 32
    add r13, 32
    sub rcx, 8
    jmp .loop8

.tail:
    test rcx, rcx
    jz .cleanup

    vmovd r10d, xmm15

.tail_loop:
    mov eax, [r12]
    imul eax, r10d
    mov [r13], eax

    add r12, 4
    add r13, 4
    dec rcx
    jnz .tail_loop

.cleanup:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_map_offset_u32: Offset operation (out = x + c)
; ============================================================================
; Signature: void fp_map_offset_u32(const uint32_t* x, uint32_t* out, size_t n, uint32_t c);

global fp_map_offset_u32
fp_map_offset_u32:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Load c from R9 (4th arg - still in register!)
    vpxor xmm0, xmm0, xmm0
    vpinsrd xmm0, xmm0, r9d, 0
    vpbroadcastd ymm15, xmm0

    mov r12, rcx        ; x
    mov r13, rdx        ; out
    mov rcx, r8         ; n

.loop32:
    cmp rcx, 32
    jb .loop8

    vmovdqu ymm0, [r12]
    vmovdqu ymm1, [r12 + 32]
    vmovdqu ymm2, [r12 + 64]
    vmovdqu ymm3, [r12 + 96]

    vpaddd ymm0, ymm0, ymm15
    vpaddd ymm1, ymm1, ymm15
    vpaddd ymm2, ymm2, ymm15
    vpaddd ymm3, ymm3, ymm15

    vmovdqu [r13], ymm0
    vmovdqu [r13 + 32], ymm1
    vmovdqu [r13 + 64], ymm2
    vmovdqu [r13 + 96], ymm3

    add r12, 128
    add r13, 128
    sub rcx, 32
    jmp .loop32

.loop8:
    cmp rcx, 8
    jb .tail

    vmovdqu ymm0, [r12]
    vpaddd ymm0, ymm0, ymm15
    vmovdqu [r13], ymm0

    add r12, 32
    add r13, 32
    sub rcx, 8
    jmp .loop8

.tail:
    test rcx, rcx
    jz .cleanup

    vmovd r10d, xmm15

.tail_loop:
    mov eax, [r12]
    add eax, r10d
    mov [r13], eax

    add r12, 4
    add r13, 4
    dec rcx
    jnz .tail_loop

.cleanup:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_zip_add_u32: Zip add operation (out = x + y)
; ============================================================================
; Signature: void fp_zip_add_u32(const uint32_t* x, const uint32_t* y, uint32_t* out, size_t n);

global fp_zip_add_u32
fp_zip_add_u32:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    mov r12, rcx        ; x
    mov r13, rdx        ; y
    mov r14, r8         ; out
    mov rcx, r9         ; n

.loop32:
    cmp rcx, 32
    jb .loop8

    vmovdqu ymm0, [r12]
    vmovdqu ymm1, [r12 + 32]
    vmovdqu ymm2, [r12 + 64]
    vmovdqu ymm3, [r12 + 96]

    vmovdqu ymm4, [r13]
    vmovdqu ymm5, [r13 + 32]
    vmovdqu ymm6, [r13 + 64]
    vmovdqu ymm7, [r13 + 96]

    vpaddd ymm0, ymm0, ymm4
    vpaddd ymm1, ymm1, ymm5
    vpaddd ymm2, ymm2, ymm6
    vpaddd ymm3, ymm3, ymm7

    vmovdqu [r14], ymm0
    vmovdqu [r14 + 32], ymm1
    vmovdqu [r14 + 64], ymm2
    vmovdqu [r14 + 96], ymm3

    add r12, 128
    add r13, 128
    add r14, 128
    sub rcx, 32
    jmp .loop32

.loop8:
    cmp rcx, 8
    jb .tail

    vmovdqu ymm0, [r12]
    vmovdqu ymm4, [r13]
    vpaddd ymm0, ymm0, ymm4
    vmovdqu [r14], ymm0

    add r12, 32
    add r13, 32
    add r14, 32
    sub rcx, 8
    jmp .loop8

.tail:
    test rcx, rcx
    jz .cleanup

.tail_loop:
    mov eax, [r12]
    add eax, [r13]
    mov [r14], eax

    add r12, 4
    add r13, 4
    add r14, 4
    dec rcx
    jnz .tail_loop

.cleanup:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret
