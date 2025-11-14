bits 64
default rel

section .rdata
    align 32
VEC3_MASK:
    dd 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000
    dd 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000

ONE_VEC:
    dd 0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000

section .text
    global fp_map_transform_vec3_f32
    global fp_zipWith_vec3_add_f32
    global fp_map_quat_rotate_vec3_f32
    global fp_fold_vec3_dot_f32

; ============================================================================
; fp_map_transform_vec3_f32
; Windows x64 ABI: RCX = in_vecs, RDX = out_vecs, R8 = n, R9 = matrix
; ============================================================================
fp_map_transform_vec3_f32:
    push rbp
    mov rbp, rsp
    sub rsp, 32                     ; shadow space

    vxorps xmm15, xmm15, xmm15

    vmovups xmm8, [r9]
    vblendps xmm8, xmm8, xmm15, 0x08

    vmovups xmm9, [r9 + 16]
    vblendps xmm9, xmm9, xmm15, 0x08

    vmovups xmm10, [r9 + 32]
    vblendps xmm10, xmm10, xmm15, 0x08

    vmovups xmm11, [r9 + 48]
    vblendps xmm11, xmm11, xmm15, 0x08

    mov r10, r8
    test r10, r10
    jz .done_map

.loop_map:
    vmovups xmm0, [rcx]

    vpermilps xmm1, xmm0, 0x00      ; xxxx
    vpermilps xmm2, xmm0, 0x55      ; yyyy
    vpermilps xmm3, xmm0, 0xAA      ; zzzz

    vmovaps xmm4, xmm11
    vfmadd231ps xmm4, xmm8, xmm1
    vfmadd231ps xmm4, xmm9, xmm2
    vfmadd231ps xmm4, xmm10, xmm3

    vmovups [rdx], xmm4

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

    mov r11, rcx
    or r11, rdx
    or r11, r8
    test r11, 31
    jnz .unaligned_zip

    cmp r10, 32
    jb .aligned_no_stream

.aligned_stream:
    vmovaps ymm0, [rcx]
    vmovaps ymm1, [rcx + 32]
    vmovaps ymm2, [rcx + 64]
    vmovaps ymm3, [rcx + 96]
    vaddps ymm0, ymm0, [rdx]
    vaddps ymm1, ymm1, [rdx + 32]
    vaddps ymm2, ymm2, [rdx + 64]
    vaddps ymm3, ymm3, [rdx + 96]
    vmovntps [r8], ymm0
    vmovntps [r8 + 32], ymm1
    vmovntps [r8 + 64], ymm2
    vmovntps [r8 + 96], ymm3

    add rcx, 128
    add rdx, 128
    add r8, 128
    sub r10, 8
    cmp r10, 32
    jae .aligned_stream

    sfence

.aligned_no_stream:
    cmp r10, 8
    jb .aligned_check_four

.aligned_loop8:
    vmovaps ymm0, [rcx]
    vmovaps ymm1, [rcx + 32]
    vmovaps ymm2, [rcx + 64]
    vmovaps ymm3, [rcx + 96]
    vaddps ymm0, ymm0, [rdx]
    vaddps ymm1, ymm1, [rdx + 32]
    vaddps ymm2, ymm2, [rdx + 64]
    vaddps ymm3, ymm3, [rdx + 96]
    vmovaps [r8], ymm0
    vmovaps [r8 + 32], ymm1
    vmovaps [r8 + 64], ymm2
    vmovaps [r8 + 96], ymm3

    add rcx, 128
    add rdx, 128
    add r8, 128
    sub r10, 8
    cmp r10, 8
    jae .aligned_loop8

.aligned_check_four:
    cmp r10, 4
    jb .aligned_check_two

.aligned_loop4:
    vmovaps ymm0, [rcx]
    vmovaps ymm1, [rcx + 32]
    vaddps ymm0, ymm0, [rdx]
    vaddps ymm1, ymm1, [rdx + 32]
    vmovaps [r8], ymm0
    vmovaps [r8 + 32], ymm1

    add rcx, 64
    add rdx, 64
    add r8, 64
    sub r10, 4
    cmp r10, 4
    jae .aligned_loop4

.aligned_check_two:
    cmp r10, 2
    jb .aligned_tail

.aligned_loop2:
    vmovaps ymm0, [rcx]
    vaddps ymm0, ymm0, [rdx]
    vmovaps [r8], ymm0

    add rcx, 32
    add rdx, 32
    add r8, 32
    sub r10, 2
    cmp r10, 2
    jae .aligned_loop2

.aligned_tail:
    test r10, r10
    jz .done_zip

    vmovaps xmm0, [rcx]
    vaddps xmm0, xmm0, [rdx]
    vmovaps [r8], xmm0
    jmp .done_zip

.unaligned_zip:
    cmp r10, 8
    jb .unaligned_check_four

.unaligned_loop8:
    vmovups ymm0, [rcx]
    vmovups ymm1, [rcx + 32]
    vmovups ymm2, [rcx + 64]
    vmovups ymm3, [rcx + 96]
    vaddps ymm0, ymm0, [rdx]
    vaddps ymm1, ymm1, [rdx + 32]
    vaddps ymm2, ymm2, [rdx + 64]
    vaddps ymm3, ymm3, [rdx + 96]
    vmovups [r8], ymm0
    vmovups [r8 + 32], ymm1
    vmovups [r8 + 64], ymm2
    vmovups [r8 + 96], ymm3

    add rcx, 128
    add rdx, 128
    add r8, 128
    sub r10, 8
    cmp r10, 8
    jae .unaligned_loop8

.unaligned_check_four:
    cmp r10, 4
    jb .unaligned_check_two

.unaligned_loop4:
    vmovups ymm0, [rcx]
    vmovups ymm1, [rcx + 32]
    vaddps ymm0, ymm0, [rdx]
    vaddps ymm1, ymm1, [rdx + 32]
    vmovups [r8], ymm0
    vmovups [r8 + 32], ymm1

    add rcx, 64
    add rdx, 64
    add r8, 64
    sub r10, 4
    cmp r10, 4
    jae .unaligned_loop4

.unaligned_check_two:
    cmp r10, 2
    jb .unaligned_tail

.unaligned_loop2:
    vmovups ymm0, [rcx]
    vaddps ymm0, ymm0, [rdx]
    vmovups [r8], ymm0

    add rcx, 32
    add rdx, 32
    add r8, 32
    sub r10, 2
    cmp r10, 2
    jae .unaligned_loop2

.unaligned_tail:
    test r10, r10
    jz .done_zip

    vmovups xmm0, [rcx]
    vaddps xmm0, xmm0, [rdx]
    vmovups [r8], xmm0

.done_zip:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_map_quat_rotate_vec3_f32
; ============================================================================
fp_map_quat_rotate_vec3_f32:
    push rbp
    mov rbp, rsp
    sub rsp, 32                     ; shadow space

    vmovaps xmm0, [rel ONE_VEC]
    vmovaps xmm1, [r9]              ; {x, y, z, w}
    vpermilps xmm2, xmm1, 0x55      ; y
    vpermilps xmm3, xmm1, 0xAA      ; z
    vpermilps xmm4, xmm1, 0xFF      ; w
    vpermilps xmm1, xmm1, 0x00      ; x

    vaddps xmm8, xmm1, xmm1         ; x2
    vaddps xmm9, xmm2, xmm2         ; y2
    vaddps xmm10, xmm3, xmm3        ; z2

    vmulps xmm11, xmm1, xmm8        ; xx
    vmulps xmm12, xmm1, xmm9        ; xy
    vmulps xmm13, xmm1, xmm10       ; xz
    vmulps xmm14, xmm2, xmm9        ; yy
    vmulps xmm15, xmm2, xmm10       ; yz
    vmulps xmm5, xmm3, xmm10        ; zz
    vmulps xmm6, xmm4, xmm8         ; wx
    vmulps xmm7, xmm4, xmm9         ; wy
    vmulps xmm8, xmm4, xmm10        ; wz (x2 reused)

    vaddps xmm1, xmm14, xmm5        ; yy + zz
    vsubps xmm1, xmm0, xmm1         ; row0.x
    vaddps xmm2, xmm12, xmm8        ; row0.y
    vsubps xmm3, xmm13, xmm7        ; row0.z

    vsubps xmm4, xmm12, xmm8        ; row1.x
    vaddps xmm9, xmm11, xmm5        ; xx + zz
    vsubps xmm5, xmm0, xmm9         ; row1.y
    vaddps xmm9, xmm15, xmm6        ; row1.z

    vaddps xmm10, xmm13, xmm7       ; row2.x
    vsubps xmm12, xmm15, xmm6       ; row2.y
    vaddps xmm13, xmm11, xmm14      ; xx + yy
    vsubps xmm13, xmm0, xmm13       ; row2.z

    vxorps xmm0, xmm0, xmm0         ; row0 vector
    vmovss xmm0, xmm0, xmm1
    vinsertps xmm0, xmm0, xmm2, 0x10
    vinsertps xmm0, xmm0, xmm3, 0x20

    vxorps xmm1, xmm1, xmm1         ; row1 vector
    vmovss xmm1, xmm1, xmm4
    vinsertps xmm1, xmm1, xmm5, 0x10
    vinsertps xmm1, xmm1, xmm9, 0x20

    vxorps xmm2, xmm2, xmm2         ; row2 vector
    vmovss xmm2, xmm2, xmm10
    vinsertps xmm2, xmm2, xmm12, 0x10
    vinsertps xmm2, xmm2, xmm13, 0x20

    vmovaps xmm4, xmm0
    vmovaps xmm5, xmm1
    vmovaps xmm6, xmm2
    vxorps xmm7, xmm7, xmm7         ; translation = 0

    vxorps ymm12, ymm12, ymm12
    vinsertf128 ymm12, ymm12, xmm4, 0
    vinsertf128 ymm12, ymm12, xmm4, 1

    vxorps ymm13, ymm13, ymm13
    vinsertf128 ymm13, ymm13, xmm5, 0
    vinsertf128 ymm13, ymm13, xmm5, 1

    vxorps ymm14, ymm14, ymm14
    vinsertf128 ymm14, ymm14, xmm6, 0
    vinsertf128 ymm14, ymm14, xmm6, 1

    vxorps ymm15, ymm15, ymm15

    mov r10, r8
    cmp r10, 4
    jb .check_quat_two

.loop_quat4:
    vmovups ymm0, [rcx]
    vpermilps ymm1, ymm0, 0x00      ; xxxx
    vpermilps ymm2, ymm0, 0x55      ; yyyy
    vpermilps ymm3, ymm0, 0xAA      ; zzzz

    vmovaps ymm8, ymm15
    vfmadd231ps ymm8, ymm12, ymm1
    vfmadd231ps ymm8, ymm13, ymm2
    vfmadd231ps ymm8, ymm14, ymm3
    vmovups [rdx], ymm8

    vmovups ymm0, [rcx + 32]
    vpermilps ymm1, ymm0, 0x00
    vpermilps ymm2, ymm0, 0x55
    vpermilps ymm3, ymm0, 0xAA

    vmovaps ymm8, ymm15
    vfmadd231ps ymm8, ymm12, ymm1
    vfmadd231ps ymm8, ymm13, ymm2
    vfmadd231ps ymm8, ymm14, ymm3
    vmovups [rdx + 32], ymm8

    add rcx, 64
    add rdx, 64
    sub r10, 4
    cmp r10, 4
    jae .loop_quat4

.check_quat_two:
    cmp r10, 2
    jb .tail_quat

.loop_quat2:
    vmovups ymm0, [rcx]
    vpermilps ymm1, ymm0, 0x00      ; xxxx
    vpermilps ymm2, ymm0, 0x55      ; yyyy
    vpermilps ymm3, ymm0, 0xAA      ; zzzz

    vmovaps ymm0, ymm15
    vfmadd231ps ymm0, ymm12, ymm1
    vfmadd231ps ymm0, ymm13, ymm2
    vfmadd231ps ymm0, ymm14, ymm3
    vmovups [rdx], ymm0

    add rcx, 32
    add rdx, 32
    sub r10, 2
    cmp r10, 2
    jae .loop_quat2

.tail_quat:
    test r10, r10
    jz .done_quat

    vmovups xmm0, [rcx]
    vpermilps xmm1, xmm0, 0x00      ; xxxx
    vpermilps xmm2, xmm0, 0x55      ; yyyy
    vpermilps xmm3, xmm0, 0xAA      ; zzzz

    vmovaps xmm0, xmm7
    vfmadd231ps xmm0, xmm4, xmm1
    vfmadd231ps xmm0, xmm5, xmm2
    vfmadd231ps xmm0, xmm6, xmm3
    vmovups [rdx], xmm0

.done_quat:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

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
    vfmadd231ps ymm4, ymm1, ymm2

    add rcx, 32
    add rdx, 32
    sub r10, 2
    jmp .loop_vec

.reduce:
    vextractf128 xmm1, ymm4, 1
    vaddps xmm2, xmm4, xmm1
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
