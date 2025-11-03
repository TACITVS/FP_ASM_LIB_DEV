; fp_core_fused_maps_i8.asm
;
; Hand-optimized x64 AVX2 SIMD implementations for i8 fused maps
; Windows x64 calling convention
;
; Performance target: 8x faster than i64 (32x i8 per YMM register!)
;
; CRITICAL LIMITATION: AVX2 has NO vpmullb (no 8-bit multiply instruction)
; Strategy: Scalar for multiply operations, SIMD for add operations
;
; Functions:
;   - fp_map_axpy_i8:    out = c*x + y (scalar multiply, then SIMD add)
;   - fp_map_scale_i8:   out = c*x (scalar multiply only)
;   - fp_map_offset_i8:  out = x + c (SIMD add - 32-wide!)
;   - fp_zip_add_i8:     out = x + y (SIMD add - 32-wide!)

bits 64
default rel

section .text

; ============================================================================
; fp_map_axpy_i8: AXPY operation (out = c*x + y)
; ============================================================================
; Signature: void fp_map_axpy_i8(const int8_t* x, const int8_t* y, int8_t* out, size_t n, int8_t c);
;
; LIMITATION: No vpmullb, use scalar multiply then SIMD add

global fp_map_axpy_i8
fp_map_axpy_i8:
    push rbp
    mov rbp, rsp

    ; Load c from stack (5th arg)
    movsx r15d, byte [rbp + 48]

    mov r12, rcx        ; x
    mov r13, rdx        ; y
    mov r14, r8         ; out
    mov rcx, r9         ; n

    test rcx, rcx
    jz .cleanup

.loop:
    movsx eax, byte [r12]
    imul eax, r15d
    movsx r10d, byte [r13]
    add eax, r10d
    mov [r14], al

    add r12, 1
    add r13, 1
    add r14, 1
    dec rcx
    jnz .loop

.cleanup:
    pop rbp
    ret

; ============================================================================
; fp_map_scale_i8: Scale operation (out = c*x)
; ============================================================================
; Signature: void fp_map_scale_i8(const int8_t* x, int8_t* out, size_t n, int8_t c);
;
; LIMITATION: No vpmullb, must use scalar

global fp_map_scale_i8
fp_map_scale_i8:
    push rbp
    mov rbp, rsp

    ; c is 4th arg - in R9
    movsx r15d, r9b

    mov r12, rcx        ; x
    mov r13, rdx        ; out
    mov rcx, r8         ; n

    test rcx, rcx
    jz .cleanup

.loop:
    movsx eax, byte [r12]
    imul eax, r15d
    mov [r13], al

    add r12, 1
    add r13, 1
    dec rcx
    jnz .loop

.cleanup:
    pop rbp
    ret

; ============================================================================
; fp_map_offset_i8: Offset operation (out = x + c)
; ============================================================================
; Signature: void fp_map_offset_i8(const int8_t* x, int8_t* out, size_t n, int8_t c);
;
; Pure SIMD - 32-wide operations!

global fp_map_offset_i8
fp_map_offset_i8:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; c is 4th arg - in R9
    vpinsrb xmm0, xmm0, r9d, 0
    vpbroadcastb ymm15, xmm0

    mov r12, rcx        ; x
    mov r13, rdx        ; out
    mov rcx, r8         ; n

.loop128:
    cmp rcx, 128
    jb .loop32

    vmovdqu ymm0, [r12]
    vmovdqu ymm1, [r12 + 32]
    vmovdqu ymm2, [r12 + 64]
    vmovdqu ymm3, [r12 + 96]

    vpaddb ymm0, ymm0, ymm15
    vpaddb ymm1, ymm1, ymm15
    vpaddb ymm2, ymm2, ymm15
    vpaddb ymm3, ymm3, ymm15

    vmovdqu [r13], ymm0
    vmovdqu [r13 + 32], ymm1
    vmovdqu [r13 + 64], ymm2
    vmovdqu [r13 + 96], ymm3

    add r12, 128
    add r13, 128
    sub rcx, 128
    jmp .loop128

.loop32:
    cmp rcx, 32
    jb .tail

    vmovdqu ymm0, [r12]
    vpaddb ymm0, ymm0, ymm15
    vmovdqu [r13], ymm0

    add r12, 32
    add r13, 32
    sub rcx, 32
    jmp .loop32

.tail:
    test rcx, rcx
    jz .cleanup

    vpextrb r10d, xmm15, 0

.tail_loop:
    movsx eax, byte [r12]
    add eax, r10d
    mov [r13], al

    add r12, 1
    add r13, 1
    dec rcx
    jnz .tail_loop

.cleanup:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_zip_add_i8: Zip add operation (out = x + y)
; ============================================================================
; Signature: void fp_zip_add_i8(const int8_t* x, const int8_t* y, int8_t* out, size_t n);
;
; Pure SIMD - 32-wide operations!

global fp_zip_add_i8
fp_zip_add_i8:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    mov r12, rcx        ; x
    mov r13, rdx        ; y
    mov r14, r8         ; out
    mov rcx, r9         ; n

.loop128:
    cmp rcx, 128
    jb .loop32

    vmovdqu ymm0, [r12]
    vmovdqu ymm1, [r12 + 32]
    vmovdqu ymm2, [r12 + 64]
    vmovdqu ymm3, [r12 + 96]

    vmovdqu ymm4, [r13]
    vmovdqu ymm5, [r13 + 32]
    vmovdqu ymm6, [r13 + 64]
    vmovdqu ymm7, [r13 + 96]

    vpaddb ymm0, ymm0, ymm4
    vpaddb ymm1, ymm1, ymm5
    vpaddb ymm2, ymm2, ymm6
    vpaddb ymm3, ymm3, ymm7

    vmovdqu [r14], ymm0
    vmovdqu [r14 + 32], ymm1
    vmovdqu [r14 + 64], ymm2
    vmovdqu [r14 + 96], ymm3

    add r12, 128
    add r13, 128
    add r14, 128
    sub rcx, 128
    jmp .loop128

.loop32:
    cmp rcx, 32
    jb .tail

    vmovdqu ymm0, [r12]
    vmovdqu ymm4, [r13]
    vpaddb ymm0, ymm0, ymm4
    vmovdqu [r14], ymm0

    add r12, 32
    add r13, 32
    add r14, 32
    sub rcx, 32
    jmp .loop32

.tail:
    test rcx, rcx
    jz .cleanup

.tail_loop:
    movsx eax, byte [r12]
    movsx r9d, byte [r13]
    add eax, r9d
    mov [r14], al

    add r12, 1
    add r13, 1
    add r14, 1
    dec rcx
    jnz .tail_loop

.cleanup:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret
