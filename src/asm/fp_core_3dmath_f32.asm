bits 64
default rel

section .rdata
    align 32
VEC3_MASK:
    dd 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000
    dd 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000

section .text
    global fp_map_transform_vec3_f32
    global fp_zipWith_vec3_add_f32
    global fp_map_quat_rotate_vec3_f32
    global fp_fold_vec3_dot_f32

    extern ref_map_quat_rotate_vec3_f32

; ============================================================================
; fp_map_transform_vec3_f32
; Windows x64 ABI: RCX = in_vecs, RDX = out_vecs, R8 = n, R9 = matrix
; ============================================================================
fp_map_transform_vec3_f32:
    push rbp
    mov rbp, rsp
    sub rsp, 96                     ; 32-byte shadow + 64 bytes for constants

    vxorps xmm4, xmm4, xmm4

    vmovaps xmm0, [r9]
    vblendps xmm0, xmm0, xmm4, 0x08
    vmovaps [rsp], xmm0

    vmovaps xmm0, [r9 + 16]
    vblendps xmm0, xmm0, xmm4, 0x08
    vmovaps [rsp + 16], xmm0

    vmovaps xmm0, [r9 + 32]
    vblendps xmm0, xmm0, xmm4, 0x08
    vmovaps [rsp + 32], xmm0

    vmovaps xmm0, [r9 + 48]
    vblendps xmm0, xmm0, xmm4, 0x08
    vmovaps [rsp + 48], xmm0

    mov r10, r8
    test r10, r10
    jz .done_map

.loop_map:
    vmovaps xmm5, [rsp + 48]

    vbroadcastss xmm4, [rcx]
    vmovaps xmm0, [rsp]
    vfmadd231ps xmm5, xmm0, xmm4

    vbroadcastss xmm4, [rcx + 4]
    vmovaps xmm0, [rsp + 16]
    vfmadd231ps xmm5, xmm0, xmm4

    vbroadcastss xmm4, [rcx + 8]
    vmovaps xmm0, [rsp + 32]
    vfmadd231ps xmm5, xmm0, xmm4

    vmovaps [rdx], xmm5

    add rcx, 16
    add rdx, 16
    dec r10
    jnz .loop_map

.done_map:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_zipWith_vec3_add_f32
; Windows x64 ABI: RCX = in_a, RDX = in_b, R8 = out, R9 = n
; ============================================================================
fp_zipWith_vec3_add_f32:
    push rbp
    mov rbp, rsp
    sub rsp, 32                     ; shadow space

    mov r10, r9
    test r10, r10
    jz .done_zip

.loop_zip:
    vmovups xmm0, [rcx]
    vaddps xmm0, xmm0, [rdx]
    vmovups [r8], xmm0

    add rcx, 16
    add rdx, 16
    add r8, 16
    dec r10
    jnz .loop_zip

.done_zip:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_map_quat_rotate_vec3_f32 (stub - delegates to reference C)
; ============================================================================
fp_map_quat_rotate_vec3_f32:
    jmp ref_map_quat_rotate_vec3_f32

; ============================================================================
; fp_fold_vec3_dot_f32
; Windows x64 ABI: RCX = in_a, RDX = in_b, R8 = n
; Return: XMM0 = float
; ============================================================================
fp_fold_vec3_dot_f32:
    push rbp
    mov rbp, rsp
    sub rsp, 32                     ; shadow space

    vxorps xmm0, xmm0, xmm0         ; scalar accumulator
    vxorps ymm4, ymm4, ymm4         ; vector accumulator
    vmovaps ymm5, [rel VEC3_MASK]

    mov r10, r8

.loop_vec:
    cmp r10, 2
    jb .reduce

    vmovups ymm1, [rcx]
    vmovups ymm2, [rdx]
    vandps ymm1, ymm1, ymm5
    vandps ymm2, ymm2, ymm5
    vmulps ymm1, ymm1, ymm2
    vaddps ymm4, ymm4, ymm1

    add rcx, 32
    add rdx, 32
    sub r10, 2
    jmp .loop_vec

.reduce:
    vextractf128 xmm1, ymm4, 1
    vmovaps xmm2, xmm4
    vaddps xmm2, xmm2, xmm1
    vhaddps xmm2, xmm2, xmm2
    vhaddps xmm2, xmm2, xmm2
    vaddss xmm0, xmm0, xmm2

    vmovaps xmm3, xmm5

.tail_check:
    test r10, r10
    jz .done_fold

.tail_loop:
    vmovups xmm1, [rcx]
    vmovups xmm2, [rdx]
    vandps xmm1, xmm1, xmm3
    vandps xmm2, xmm2, xmm3
    vmulps xmm1, xmm1, xmm2
    vhaddps xmm1, xmm1, xmm1
    vhaddps xmm1, xmm1, xmm1
    vaddss xmm0, xmm0, xmm1

    add rcx, 16
    add rdx, 16
    dec r10
    jnz .tail_loop

.done_fold:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret
