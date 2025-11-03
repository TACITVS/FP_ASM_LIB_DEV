; fp_core_fused_maps_u16.asm
;
; Hand-optimized x64 AVX2 SIMD implementations for u16 fused maps
; Windows x64 calling convention
;
; Performance target: 4x faster than u64 (16x u16 per YMM register!)
;
; Key advantage: vpmullw provides true SIMD 16-bit multiply!
; Memory advantage: 16-bit data = better cache utilization
;
; Functions:
;   - fp_map_axpy_u16:    out = c*x + y (SIMD multiply + add!)
;   - fp_map_scale_u16:   out = c*x (SIMD multiply!)
;   - fp_map_offset_u16:  out = x + c (SIMD add)
;   - fp_zip_add_u16:     out = x + y (SIMD add)

bits 64
default rel

section .text

; ============================================================================
; fp_map_axpy_u16: AXPY operation (out = c*x + y)
; ============================================================================
; Signature: void fp_map_axpy_u16(const uint16_t* x, const uint16_t* y, uint16_t* out, size_t n, uint16_t c);
;
; HUGE advantage over u64: vpmullw gives us true SIMD multiply!

global fp_map_axpy_u16
fp_map_axpy_u16:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Load c from stack (5th arg) and broadcast
    movzx eax, word [rbp + 48]
    vpinsrw xmm0, xmm0, eax, 0
    vpbroadcastw ymm15, xmm0

    mov r12, rcx        ; x
    mov r13, rdx        ; y
    mov r14, r8         ; out
    mov rcx, r9         ; n

.loop64:
    cmp rcx, 64
    jb .loop16

    vmovdqu ymm0, [r12]
    vmovdqu ymm1, [r12 + 32]
    vmovdqu ymm2, [r12 + 64]
    vmovdqu ymm3, [r12 + 96]

    vmovdqu ymm4, [r13]
    vmovdqu ymm5, [r13 + 32]
    vmovdqu ymm6, [r13 + 64]
    vmovdqu ymm7, [r13 + 96]

    ; c*x (SIMD multiply!)
    vpmullw ymm0, ymm0, ymm15
    vpmullw ymm1, ymm1, ymm15
    vpmullw ymm2, ymm2, ymm15
    vpmullw ymm3, ymm3, ymm15

    ; + y
    vpaddw ymm0, ymm0, ymm4
    vpaddw ymm1, ymm1, ymm5
    vpaddw ymm2, ymm2, ymm6
    vpaddw ymm3, ymm3, ymm7

    vmovdqu [r14], ymm0
    vmovdqu [r14 + 32], ymm1
    vmovdqu [r14 + 64], ymm2
    vmovdqu [r14 + 96], ymm3

    add r12, 128
    add r13, 128
    add r14, 128
    sub rcx, 64
    jmp .loop64

.loop16:
    cmp rcx, 16
    jb .tail

    vmovdqu ymm0, [r12]
    vmovdqu ymm4, [r13]
    vpmullw ymm0, ymm0, ymm15
    vpaddw ymm0, ymm0, ymm4
    vmovdqu [r14], ymm0

    add r12, 32
    add r13, 32
    add r14, 32
    sub rcx, 16
    jmp .loop16

.tail:
    test rcx, rcx
    jz .cleanup

    vpextrw r10d, xmm15, 0

.tail_loop:
    movzx eax, word [r12]
    imul eax, r10d
    movzx r9d, word [r13]
    add eax, r9d
    mov [r14], ax

    add r12, 2
    add r13, 2
    add r14, 2
    dec rcx
    jnz .tail_loop

.cleanup:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_map_scale_u16: Scale operation (out = c*x)
; ============================================================================
; Signature: void fp_map_scale_u16(const uint16_t* x, uint16_t* out, size_t n, uint16_t c);
;
; Pure SIMD multiply - this is beautiful!

global fp_map_scale_u16
fp_map_scale_u16:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; c is 4th arg - in R9 (still in register!)
    vpinsrw xmm0, xmm0, r9d, 0
    vpbroadcastw ymm15, xmm0

    mov r12, rcx        ; x
    mov r13, rdx        ; out
    mov rcx, r8         ; n

.loop64:
    cmp rcx, 64
    jb .loop16

    vmovdqu ymm0, [r12]
    vmovdqu ymm1, [r12 + 32]
    vmovdqu ymm2, [r12 + 64]
    vmovdqu ymm3, [r12 + 96]

    vpmullw ymm0, ymm0, ymm15
    vpmullw ymm1, ymm1, ymm15
    vpmullw ymm2, ymm2, ymm15
    vpmullw ymm3, ymm3, ymm15

    vmovdqu [r13], ymm0
    vmovdqu [r13 + 32], ymm1
    vmovdqu [r13 + 64], ymm2
    vmovdqu [r13 + 96], ymm3

    add r12, 128
    add r13, 128
    sub rcx, 64
    jmp .loop64

.loop16:
    cmp rcx, 16
    jb .tail

    vmovdqu ymm0, [r12]
    vpmullw ymm0, ymm0, ymm15
    vmovdqu [r13], ymm0

    add r12, 32
    add r13, 32
    sub rcx, 16
    jmp .loop16

.tail:
    test rcx, rcx
    jz .cleanup

    vpextrw r10d, xmm15, 0

.tail_loop:
    movzx eax, word [r12]
    imul eax, r10d
    mov [r13], ax

    add r12, 2
    add r13, 2
    dec rcx
    jnz .tail_loop

.cleanup:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_map_offset_u16: Offset operation (out = x + c)
; ============================================================================
; Signature: void fp_map_offset_u16(const uint16_t* x, uint16_t* out, size_t n, uint16_t c);

global fp_map_offset_u16
fp_map_offset_u16:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; c is 4th arg - in R9
    vpinsrw xmm0, xmm0, r9d, 0
    vpbroadcastw ymm15, xmm0

    mov r12, rcx        ; x
    mov r13, rdx        ; out
    mov rcx, r8         ; n

.loop64:
    cmp rcx, 64
    jb .loop16

    vmovdqu ymm0, [r12]
    vmovdqu ymm1, [r12 + 32]
    vmovdqu ymm2, [r12 + 64]
    vmovdqu ymm3, [r12 + 96]

    vpaddw ymm0, ymm0, ymm15
    vpaddw ymm1, ymm1, ymm15
    vpaddw ymm2, ymm2, ymm15
    vpaddw ymm3, ymm3, ymm15

    vmovdqu [r13], ymm0
    vmovdqu [r13 + 32], ymm1
    vmovdqu [r13 + 64], ymm2
    vmovdqu [r13 + 96], ymm3

    add r12, 128
    add r13, 128
    sub rcx, 64
    jmp .loop64

.loop16:
    cmp rcx, 16
    jb .tail

    vmovdqu ymm0, [r12]
    vpaddw ymm0, ymm0, ymm15
    vmovdqu [r13], ymm0

    add r12, 32
    add r13, 32
    sub rcx, 16
    jmp .loop16

.tail:
    test rcx, rcx
    jz .cleanup

    vpextrw r10d, xmm15, 0

.tail_loop:
    movzx eax, word [r12]
    add eax, r10d
    mov [r13], ax

    add r12, 2
    add r13, 2
    dec rcx
    jnz .tail_loop

.cleanup:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_zip_add_u16: Zip add operation (out = x + y)
; ============================================================================
; Signature: void fp_zip_add_u16(const uint16_t* x, const uint16_t* y, uint16_t* out, size_t n);

global fp_zip_add_u16
fp_zip_add_u16:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    mov r12, rcx        ; x
    mov r13, rdx        ; y
    mov r14, r8         ; out
    mov rcx, r9         ; n

.loop64:
    cmp rcx, 64
    jb .loop16

    vmovdqu ymm0, [r12]
    vmovdqu ymm1, [r12 + 32]
    vmovdqu ymm2, [r12 + 64]
    vmovdqu ymm3, [r12 + 96]

    vmovdqu ymm4, [r13]
    vmovdqu ymm5, [r13 + 32]
    vmovdqu ymm6, [r13 + 64]
    vmovdqu ymm7, [r13 + 96]

    vpaddw ymm0, ymm0, ymm4
    vpaddw ymm1, ymm1, ymm5
    vpaddw ymm2, ymm2, ymm6
    vpaddw ymm3, ymm3, ymm7

    vmovdqu [r14], ymm0
    vmovdqu [r14 + 32], ymm1
    vmovdqu [r14 + 64], ymm2
    vmovdqu [r14 + 96], ymm3

    add r12, 128
    add r13, 128
    add r14, 128
    sub rcx, 64
    jmp .loop64

.loop16:
    cmp rcx, 16
    jb .tail

    vmovdqu ymm0, [r12]
    vmovdqu ymm4, [r13]
    vpaddw ymm0, ymm0, ymm4
    vmovdqu [r14], ymm0

    add r12, 32
    add r13, 32
    add r14, 32
    sub rcx, 16
    jmp .loop16

.tail:
    test rcx, rcx
    jz .cleanup

.tail_loop:
    movzx eax, word [r12]
    movzx r9d, word [r13]
    add eax, r9d
    mov [r14], ax

    add r12, 2
    add r13, 2
    add r14, 2
    dec rcx
    jnz .tail_loop

.cleanup:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret
