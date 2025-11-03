; fp_core_fused_maps_i32.asm
;
; Hand-optimized x64 AVX2 SIMD implementations for i32 fused maps
; Windows x64 calling convention
;
; Performance target: Memory-bound operations (saturate bandwidth)
;
; Functions:
;   - fp_map_axpy_i32:    out[i] = c * x[i] + y[i]  (BLAS AXPY)
;   - fp_map_scale_i32:   out[i] = c * x[i]         (scalar multiply)
;   - fp_map_offset_i32:  out[i] = c + x[i]         (scalar add)
;   - fp_zip_add_i32:     out[i] = x[i] + y[i]      (element-wise add)

bits 64
default rel

section .text

; ============================================================================
; fp_map_axpy_i32: AXPY operation (A*X + Y)
; ============================================================================
; Signature: void fp_map_axpy_i32(const int32_t* x, const int32_t* y,
;                                  int32_t* out, size_t n, int32_t c);
;
; Haskell type: zipWith (+) (map (*c) x) y :: [Int32] -> [Int32] -> Int32 -> [Int32]
;
; Windows ABI: RCX = x, RDX = y, R8 = out, R9 = n, [RBP+48] = c

global fp_map_axpy_i32
fp_map_axpy_i32:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Load scalar c from stack and broadcast
    mov eax, [rbp + 48]             ; Load c from 5th argument (stack)
    vpxor xmm0, xmm0, xmm0
    vpinsrd xmm0, xmm0, eax, 0
    vpbroadcastd ymm15, xmm0        ; ymm15 = [c, c, c, c, c, c, c, c]

    mov r12, rcx                    ; r12 = x
    mov r13, rdx                    ; r13 = y
    mov r14, r8                     ; r14 = out
    mov rcx, r9                     ; rcx = count

.loop32:
    cmp rcx, 32
    jb .loop8

    ; Load x and y
    vmovdqu ymm0, [r12]
    vmovdqu ymm1, [r12 + 32]
    vmovdqu ymm2, [r12 + 64]
    vmovdqu ymm3, [r12 + 96]

    vmovdqu ymm4, [r13]
    vmovdqu ymm5, [r13 + 32]
    vmovdqu ymm6, [r13 + 64]
    vmovdqu ymm7, [r13 + 96]

    ; c * x
    vpmulld ymm0, ymm0, ymm15
    vpmulld ymm1, ymm1, ymm15
    vpmulld ymm2, ymm2, ymm15
    vpmulld ymm3, ymm3, ymm15

    ; c * x + y
    vpaddd ymm0, ymm0, ymm4
    vpaddd ymm1, ymm1, ymm5
    vpaddd ymm2, ymm2, ymm6
    vpaddd ymm3, ymm3, ymm7

    ; Store result
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
    jz .done

.tail_loop:
    vmovd r10d, xmm15               ; Extract c
    mov eax, [r12]                  ; Load x[i]
    imul eax, r10d                  ; c * x[i]
    add eax, [r13]                  ; c * x[i] + y[i]
    mov [r14], eax                  ; Store result

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
; fp_map_scale_i32: Scale by constant
; ============================================================================
; Signature: void fp_map_scale_i32(const int32_t* x, int32_t* out,
;                                   size_t n, int32_t c);
;
; Haskell type: map (*c) :: [Int32] -> Int32 -> [Int32]
;
; Windows ABI: RCX = x, RDX = out, R8 = n, R9 = c

global fp_map_scale_i32
fp_map_scale_i32:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Broadcast scalar c
    vpxor xmm0, xmm0, xmm0
    vpinsrd xmm0, xmm0, r9d, 0
    vpbroadcastd ymm15, xmm0

    mov r12, rcx                    ; r12 = x
    mov r14, rdx                    ; r14 = out
    mov rcx, r8                     ; rcx = count

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

    vmovdqu [r14], ymm0
    vmovdqu [r14 + 32], ymm1
    vmovdqu [r14 + 64], ymm2
    vmovdqu [r14 + 96], ymm3

    add r12, 128
    add r14, 128
    sub rcx, 32
    jmp .loop32

.loop8:
    cmp rcx, 8
    jb .tail

    vmovdqu ymm0, [r12]
    vpmulld ymm0, ymm0, ymm15
    vmovdqu [r14], ymm0

    add r12, 32
    add r14, 32
    sub rcx, 8
    jmp .loop8

.tail:
    test rcx, rcx
    jz .done

.tail_loop:
    vmovd r10d, xmm15
    mov eax, [r12]
    imul eax, r10d
    mov [r14], eax

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
; fp_map_offset_i32: Add constant
; ============================================================================
; Signature: void fp_map_offset_i32(const int32_t* x, int32_t* out,
;                                    size_t n, int32_t c);
;
; Haskell type: map (+c) :: [Int32] -> Int32 -> [Int32]
;
; Windows ABI: RCX = x, RDX = out, R8 = n, R9 = c

global fp_map_offset_i32
fp_map_offset_i32:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Broadcast scalar c
    vpxor xmm0, xmm0, xmm0
    vpinsrd xmm0, xmm0, r9d, 0
    vpbroadcastd ymm15, xmm0

    mov r12, rcx                    ; r12 = x
    mov r14, rdx                    ; r14 = out
    mov rcx, r8                     ; rcx = count

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

    vmovdqu [r14], ymm0
    vmovdqu [r14 + 32], ymm1
    vmovdqu [r14 + 64], ymm2
    vmovdqu [r14 + 96], ymm3

    add r12, 128
    add r14, 128
    sub rcx, 32
    jmp .loop32

.loop8:
    cmp rcx, 8
    jb .tail

    vmovdqu ymm0, [r12]
    vpaddd ymm0, ymm0, ymm15
    vmovdqu [r14], ymm0

    add r12, 32
    add r14, 32
    sub rcx, 8
    jmp .loop8

.tail:
    test rcx, rcx
    jz .done

.tail_loop:
    vmovd r10d, xmm15
    mov eax, [r12]
    add eax, r10d
    mov [r14], eax

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
; fp_zip_add_i32: Element-wise addition
; ============================================================================
; Signature: void fp_zip_add_i32(const int32_t* x, const int32_t* y,
;                                 int32_t* out, size_t n);
;
; Haskell type: zipWith (+) :: [Int32] -> [Int32] -> [Int32]
;
; Windows ABI: RCX = x, RDX = y, R8 = out, R9 = n

global fp_zip_add_i32
fp_zip_add_i32:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    mov r12, rcx                    ; r12 = x
    mov r13, rdx                    ; r13 = y
    mov r14, r8                     ; r14 = out
    mov rcx, r9                     ; rcx = count

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
    jz .done

.tail_loop:
    mov eax, [r12]
    add eax, [r13]
    mov [r14], eax

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
