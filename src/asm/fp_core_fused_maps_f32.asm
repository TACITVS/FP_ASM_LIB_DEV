; fp_core_fused_maps_f32.asm
;
; Hand-optimized x64 AVX2 SIMD implementations for f32 fused maps
; Windows x64 calling convention
;
; Performance target: Memory-bound operations (saturate bandwidth)
;
; Functions:
;   - fp_map_axpy_f32:    out[i] = c * x[i] + y[i]  (BLAS AXPY with FMA)
;   - fp_map_scale_f32:   out[i] = c * x[i]         (scalar multiply)
;   - fp_map_offset_f32:  out[i] = c + x[i]         (scalar add)
;   - fp_zip_add_f32:     out[i] = x[i] + y[i]      (element-wise add)

bits 64
default rel

section .text

; ============================================================================
; fp_map_axpy_f32: AXPY operation with FMA (A*X + Y)
; ============================================================================
; Signature: void fp_map_axpy_f32(const float* x, const float* y,
;                                  float* out, size_t n, float c);
;
; Haskell type: zipWith (+) (map (*c) x) y :: [Float] -> [Float] -> Float -> [Float]
;
; Windows ABI: RCX = x, RDX = y, R8 = out, R9 = n, [RBP+48] = c

global fp_map_axpy_f32
fp_map_axpy_f32:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Load scalar c from stack and broadcast
    vmovss xmm15, [rbp + 48]        ; Load c from 5th argument (stack)
    vbroadcastss ymm15, xmm15       ; ymm15 = [c, c, c, c, c, c, c, c]

    mov r12, rcx                    ; r12 = x
    mov r13, rdx                    ; r13 = y
    mov r14, r8                     ; r14 = out
    mov rcx, r9                     ; rcx = count

.loop32:
    cmp rcx, 32
    jb .loop8

    ; Load x and y
    vmovups ymm0, [r12]
    vmovups ymm1, [r12 + 32]
    vmovups ymm2, [r12 + 64]
    vmovups ymm3, [r12 + 96]

    vmovups ymm4, [r13]
    vmovups ymm5, [r13 + 32]
    vmovups ymm6, [r13 + 64]
    vmovups ymm7, [r13 + 96]

    ; FMA: y + (c * x)
    vfmadd213ps ymm0, ymm15, ymm4   ; ymm0 = (ymm0 * ymm15) + ymm4
    vfmadd213ps ymm1, ymm15, ymm5
    vfmadd213ps ymm2, ymm15, ymm6
    vfmadd213ps ymm3, ymm15, ymm7

    ; Store result
    vmovups [r14], ymm0
    vmovups [r14 + 32], ymm1
    vmovups [r14 + 64], ymm2
    vmovups [r14 + 96], ymm3

    add r12, 128
    add r13, 128
    add r14, 128
    sub rcx, 32
    jmp .loop32

.loop8:
    cmp rcx, 8
    jb .tail

    vmovups ymm0, [r12]
    vmovups ymm4, [r13]
    vfmadd213ps ymm0, ymm15, ymm4
    vmovups [r14], ymm0

    add r12, 32
    add r13, 32
    add r14, 32
    sub rcx, 8
    jmp .loop8

.tail:
    test rcx, rcx
    jz .done

.tail_loop:
    vmovss xmm0, [r12]              ; Load x[i]
    vmovss xmm4, [r13]              ; Load y[i]
    vfmadd213ss xmm0, xmm15, xmm4   ; xmm0 = (x * c) + y
    vmovss [r14], xmm0              ; Store result

    add r12, 4
    add r13, 4
    add r14, 4
    dec rcx
    jnz .tail_loop

.done:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_map_scale_f32: Scale by constant
; ============================================================================
; Signature: void fp_map_scale_f32(const float* x, float* out,
;                                   size_t n, float c);
;
; Haskell type: map (*c) :: [Float] -> Float -> [Float]
;
; Windows ABI: RCX = x, RDX = out, R8 = n, R9 = c (passed in XMM3)

global fp_map_scale_f32
fp_map_scale_f32:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Broadcast scalar c (already in xmm3 per Windows ABI)
    vbroadcastss ymm15, xmm3

    mov r12, rcx                    ; r12 = x
    mov r14, rdx                    ; r14 = out
    mov rcx, r8                     ; rcx = count

.loop32:
    cmp rcx, 32
    jb .loop8

    vmovups ymm0, [r12]
    vmovups ymm1, [r12 + 32]
    vmovups ymm2, [r12 + 64]
    vmovups ymm3, [r12 + 96]

    vmulps ymm0, ymm0, ymm15
    vmulps ymm1, ymm1, ymm15
    vmulps ymm2, ymm2, ymm15
    vmulps ymm3, ymm3, ymm15

    vmovups [r14], ymm0
    vmovups [r14 + 32], ymm1
    vmovups [r14 + 64], ymm2
    vmovups [r14 + 96], ymm3

    add r12, 128
    add r14, 128
    sub rcx, 32
    jmp .loop32

.loop8:
    cmp rcx, 8
    jb .tail

    vmovups ymm0, [r12]
    vmulps ymm0, ymm0, ymm15
    vmovups [r14], ymm0

    add r12, 32
    add r14, 32
    sub rcx, 8
    jmp .loop8

.tail:
    test rcx, rcx
    jz .done

.tail_loop:
    vmovss xmm0, [r12]
    vmulss xmm0, xmm0, xmm15
    vmovss [r14], xmm0

    add r12, 4
    add r14, 4
    dec rcx
    jnz .tail_loop

.done:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_map_offset_f32: Add constant
; ============================================================================
; Signature: void fp_map_offset_f32(const float* x, float* out,
;                                    size_t n, float c);
;
; Haskell type: map (+c) :: [Float] -> Float -> [Float]
;
; Windows ABI: RCX = x, RDX = out, R8 = n, R9 = c (passed in XMM3)

global fp_map_offset_f32
fp_map_offset_f32:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Broadcast scalar c
    vbroadcastss ymm15, xmm3

    mov r12, rcx
    mov r14, rdx
    mov rcx, r8

.loop32:
    cmp rcx, 32
    jb .loop8

    vmovups ymm0, [r12]
    vmovups ymm1, [r12 + 32]
    vmovups ymm2, [r12 + 64]
    vmovups ymm3, [r12 + 96]

    vaddps ymm0, ymm0, ymm15
    vaddps ymm1, ymm1, ymm15
    vaddps ymm2, ymm2, ymm15
    vaddps ymm3, ymm3, ymm15

    vmovups [r14], ymm0
    vmovups [r14 + 32], ymm1
    vmovups [r14 + 64], ymm2
    vmovups [r14 + 96], ymm3

    add r12, 128
    add r14, 128
    sub rcx, 32
    jmp .loop32

.loop8:
    cmp rcx, 8
    jb .tail

    vmovups ymm0, [r12]
    vaddps ymm0, ymm0, ymm15
    vmovups [r14], ymm0

    add r12, 32
    add r14, 32
    sub rcx, 8
    jmp .loop8

.tail:
    test rcx, rcx
    jz .done

.tail_loop:
    vmovss xmm0, [r12]
    vaddss xmm0, xmm0, xmm15
    vmovss [r14], xmm0

    add r12, 4
    add r14, 4
    dec rcx
    jnz .tail_loop

.done:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_zip_add_f32: Element-wise addition
; ============================================================================
; Signature: void fp_zip_add_f32(const float* x, const float* y,
;                                 float* out, size_t n);
;
; Haskell type: zipWith (+) :: [Float] -> [Float] -> [Float]
;
; Windows ABI: RCX = x, RDX = y, R8 = out, R9 = n

global fp_zip_add_f32
fp_zip_add_f32:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    mov r12, rcx
    mov r13, rdx
    mov r14, r8
    mov rcx, r9

.loop32:
    cmp rcx, 32
    jb .loop8

    vmovups ymm0, [r12]
    vmovups ymm1, [r12 + 32]
    vmovups ymm2, [r12 + 64]
    vmovups ymm3, [r12 + 96]

    vmovups ymm4, [r13]
    vmovups ymm5, [r13 + 32]
    vmovups ymm6, [r13 + 64]
    vmovups ymm7, [r13 + 96]

    vaddps ymm0, ymm0, ymm4
    vaddps ymm1, ymm1, ymm5
    vaddps ymm2, ymm2, ymm6
    vaddps ymm3, ymm3, ymm7

    vmovups [r14], ymm0
    vmovups [r14 + 32], ymm1
    vmovups [r14 + 64], ymm2
    vmovups [r14 + 96], ymm3

    add r12, 128
    add r13, 128
    add r14, 128
    sub rcx, 32
    jmp .loop32

.loop8:
    cmp rcx, 8
    jb .tail

    vmovups ymm0, [r12]
    vmovups ymm4, [r13]
    vaddps ymm0, ymm0, ymm4
    vmovups [r14], ymm0

    add r12, 32
    add r13, 32
    add r14, 32
    sub rcx, 8
    jmp .loop8

.tail:
    test rcx, rcx
    jz .done

.tail_loop:
    vmovss xmm0, [r12]
    vmovss xmm4, [r13]
    vaddss xmm0, xmm0, xmm4
    vmovss [r14], xmm0

    add r12, 4
    add r13, 4
    add r14, 4
    dec rcx
    jnz .tail_loop

.done:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret
