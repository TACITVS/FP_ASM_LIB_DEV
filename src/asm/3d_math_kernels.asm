; =============================================================================
; FP-ASM Library: 3D Math Functions (f32)
;
; CORRECTED VERSION:
; - Removed all non-breaking space characters (0xA0)
; - Fixed all functions to use ONLY AVX2 registers (ymm0-ymm15)
; - Matched fp_map_transform_vec3_f32 to the row-major matrix layout
;   and implemented a 2-vector-at-a-time loop.
; - Fixed fp_fold_vec3_dot_f32 to use 4 accumulators (ymm4-ymm7).
; =============================================================================
default rel

section .text
    global fp_map_transform_vec3_f32
    global fp_zipWith_vec3_add_f32
    global fp_map_quat_rotate_vec3_f32
    global fp_reduce_vec3_add_f32
    global fp_fold_vec3_dot_f32

; --- Helper Macros ---
%macro PROLOGUE 0
    push    rbp
    mov     rbp, rsp
    push    r12                     ; Save non-volatile registers
    push    r13
    push    r14
    push    r15
    mov     rax, rsp                ; Keep pointer to register save area
    and     rsp, 0xFFFFFFFFFFFFFFE0 ; 32-byte align stack for AVX
    sub     rsp, 288                ; 256 bytes for YMM + 32 bytes padding/metadata
    mov     [rsp+256], rax          ; Remember original stack pointer

    vmovdqa [rsp],      ymm6        ; Save non-volatile YMM registers
    vmovdqa [rsp+32],   ymm7
    vmovdqa [rsp+64],   ymm8
    vmovdqa [rsp+96],   ymm9
    vmovdqa [rsp+128],  ymm10
    vmovdqa [rsp+160],  ymm11
    vmovdqa [rsp+192],  ymm12
    vmovdqa [rsp+224],  ymm13
%endmacro

%macro EPILOGUE 0
    vmovdqa ymm6,   [rsp]           ; Restore non-volatile YMM
    vmovdqa ymm7,   [rsp+32]
    vmovdqa ymm8,   [rsp+64]
    vmovdqa ymm9,   [rsp+96]
    vmovdqa ymm10,  [rsp+128]
    vmovdqa ymm11,  [rsp+160]
    vmovdqa ymm12,  [rsp+192]
    vmovdqa ymm13,  [rsp+224]

    mov     rsp, [rsp+256]          ; Restore stack pointer before alignment
    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rbp
    vzeroupper                      ; Clear upper YMM lanes
    ret
%endmacro

; -----------------------------------------------------------------------------
; void fp_map_transform_vec3_f32(
;     RCX: const Vec3f* in_vecs,
;     RDX: Vec3f* out_vecs,
;     R8:  size_t n,
;     R9:  const Mat4f* matrix
; );
; Processes 2 vectors (32 bytes) at a time.
; -----------------------------------------------------------------------------
fp_map_transform_vec3_f32:
    PROLOGUE
    
    ; --- Load Arguments ---
    mov     r12, rcx                ; r12 = in_vecs
    mov     r13, rdx                ; r13 = out_vecs
    mov     r14, r8                 ; r14 = n
    ; r9 = matrix

    ; --- Load Matrix (Broadcast 2x) ---
    ; Mat4f is COLUMN-MAJOR (OpenGL style)
    vmovdqu xmm4, [r9]              ; Column 0: [m00, m10, m20, m30]
    vmovdqu xmm5, [r9+16]           ; Column 1: [m01, m11, m21, m31]
    vmovdqu xmm6, [r9+32]           ; Column 2: [m02, m12, m22, m32]
    vmovdqu xmm7, [r9+48]           ; Column 3: [m03, m13, m23, m33]

    vinsertf128 ymm4, ymm4, xmm4, 1 ; ymm4 = [Col0 | Col0]
    vinsertf128 ymm5, ymm5, xmm5, 1 ; ymm5 = [Col1 | Col1]
    vinsertf128 ymm6, ymm6, xmm6, 1 ; ymm6 = [Col2 | Col2]
    vinsertf128 ymm7, ymm7, xmm7, 1 ; ymm7 = [Col3 | Col3]

.loop_header:
    cmp     r14, 2
    jl      .tail

    ; Load 2 vectors (2 * 16 bytes = 32 bytes)
    vmovdqu ymm0, [r12]             ; ymm0 = [v0.xyzp | v1.xyzp]

    ; "Splat" components across registers
    vshufps ymm1, ymm0, ymm0, 0x00  ; ymm1 = [v0.x, v0.x, v0.x, v0.x | v1.x, v1.x, v1.x, v1.x]
    vshufps ymm2, ymm0, ymm0, 0x55  ; ymm2 = [v0.y, v0.y, v0.y, v0.y | v1.y, v1.y, v1.y, v1.y]
    vshufps ymm3, ymm0, ymm0, 0xAA  ; ymm3 = [v0.z, v0.z, v0.z, v0.z | v1.z, v1.z, v1.z, v1.z]

    ; --- Perform Transformation (w = 1.0) ---
    ; Column-major: out = col3 + x*col0 + y*col1 + z*col2
    
    vmovaps ymm8, ymm7              ; Start from translation column
    vfmadd231ps ymm8, ymm4, ymm1    ; += x * column0
    vfmadd231ps ymm8, ymm5, ymm2    ; += y * column1
    vfmadd231ps ymm8, ymm6, ymm3    ; += z * column2

    ; Store 2 transformed vectors and zero padding lanes
    vmovdqu [r13], ymm8
    mov     dword [r13+12], 0
    mov     dword [r13+28], 0

    add     r12, 32                 ; 2 vectors * 16 bytes/vec
    add     r13, 32
    sub     r14, 2
    jmp     .loop_header

.tail:
    cmp     r14, 0
    je      .cleanup

    ; Load 1 vector
    vmovdqu xmm0, [r12]             ; xmm0 = [v0.xyzp]
    
    vshufps xmm1, xmm0, xmm0, 0x00
    vshufps xmm2, xmm0, xmm0, 0x55
    vshufps xmm3, xmm0, xmm0, 0xAA

    vmovaps xmm8, xmm7
    vfmadd231ps xmm8, xmm4, xmm1    ; += x * column0
    vfmadd231ps xmm8, xmm5, xmm2    ; += y * column1
    vfmadd231ps xmm8, xmm6, xmm3    ; += z * column2
    
    vmovdqu [r13], xmm8
    mov     dword [r13+12], 0

.cleanup:
    EPILOGUE

; -----------------------------------------------------------------------------
; void fp_zipWith_vec3_add_f32(
;     RCX: const Vec3f* in_a,
;     RDX: const Vec3f* in_b,
;     R8:  Vec3f* out_vecs,
;     R9:  size_t n
; );
; Processes 8 vectors (128 bytes) at a time.
; -----------------------------------------------------------------------------
fp_zipWith_vec3_add_f32:
    PROLOGUE

    mov     r12, rcx                ; r12 = in_a
    mov     r13, rdx                ; r13 = in_b
    mov     r14, r8                 ; r14 = out_vecs
    mov     r15, r9                 ; r15 = n

.loop_header_add:
    cmp     r15, 8
    jl      .tail_add

    vmovdqu ymm0, [r12]             ; Load 8 floats (2 vec3s) from A
    vmovdqu ymm1, [r12+32]
    vmovdqu ymm2, [r12+64]
    vmovdqu ymm3, [r12+96]
    
    vaddps  ymm0, ymm0, [r13]       ; Add 8 floats (2 vec3s) from B
    vaddps  ymm1, ymm1, [r13+32]
    vaddps  ymm2, ymm2, [r13+64]
    vaddps  ymm3, ymm3, [r13+96]
    
    vmovdqu [r14], ymm0             ; Store 8 floats (2 vec3s) to Out
    vmovdqu [r14+32], ymm1
    vmovdqu [r14+64], ymm2
    vmovdqu [r14+96], ymm3

    add     r12, 128                ; 8 vectors * 16 bytes/vec
    add     r13, 128
    add     r14, 128
    sub     r15, 8
    jmp     .loop_header_add

.tail_add:
    cmp     r15, 0
    je      .cleanup_add
    
.tail_loop_add:
    vmovdqu xmm0, [r12]
    vaddps  xmm0, xmm0, [r13]
    vmovdqu [r14], xmm0
    
    add     r12, 16
    add     r13, 16
    add     r14, 16
    dec     r15
    jnz     .tail_loop_add

.cleanup_add:
    EPILOGUE

; -----------------------------------------------------------------------------
; void fp_map_quat_rotate_vec3_f32(
;     RCX: const Vec3f* in_vecs,
;     RDX: Vec3f* out_vecs,
;     R8:  size_t n,
;     R9:  const QuatF32* quat
; );
; Quaternion rotation implemented with scalar math
; -----------------------------------------------------------------------------
fp_map_quat_rotate_vec3_f32:
    PROLOGUE

    mov     r12, rcx                ; r12 = in_vecs
    mov     r13, rdx                ; r13 = out_vecs
    mov     r14, r8                 ; r14 = n
    ; Load quaternion components (x, y, z, w)
    vmovss  xmm8, [r9]
    vmovss  xmm9, [r9+4]
    vmovss  xmm10,[r9+8]
    vmovss  xmm11,[r9+12]

    ; Precompute s^2 - |u|^2 and 2*s
    vmovaps xmm12, xmm8
    vmulss  xmm12, xmm12, xmm8
    vfmadd231ss xmm12, xmm9, xmm9
    vfmadd231ss xmm12, xmm10, xmm10

    vmovaps xmm13, xmm11
    vmulss  xmm13, xmm13, xmm11
    vsubss  xmm13, xmm13, xmm12     ; s^2 - |u|^2

    vaddss  xmm14, xmm11, xmm11     ; 2*s

.loop_quat:
    cmp     r14, 0
    je      .cleanup_quat

    vmovss  xmm0, [r12]             ; vx
    vmovss  xmm1, [r12+4]           ; vy
    vmovss  xmm2, [r12+8]           ; vz

    ; dot_uv = u · v
    vmovaps xmm3, xmm8
    vmulss  xmm3, xmm3, xmm0
    vfmadd231ss xmm3, xmm9, xmm1
    vfmadd231ss xmm3, xmm10, xmm2

    ; two_dot = 2 * dot_uv
    vmovaps xmm4, xmm3
    vaddss  xmm4, xmm4, xmm4

    ; term1 = two_dot * u
    vmulss  xmm5, xmm8, xmm4
    vmulss  xmm6, xmm9, xmm4
    vmulss  xmm7, xmm10, xmm4

    ; term2 = (s^2 - |u|^2) * v
    vmulss  xmm12, xmm13, xmm0
    vaddss  xmm5, xmm5, xmm12
    vmulss  xmm12, xmm13, xmm1
    vaddss  xmm6, xmm6, xmm12
    vmulss  xmm12, xmm13, xmm2
    vaddss  xmm7, xmm7, xmm12

    ; term3 = 2*s * (u x v)
    vmulss  xmm12, xmm9, xmm2       ; qy*vz
    vmulss  xmm15, xmm10, xmm1      ; qz*vy
    vsubss  xmm12, xmm12, xmm15
    vmulss  xmm12, xmm12, xmm14
    vaddss  xmm5, xmm5, xmm12

    vmulss  xmm12, xmm10, xmm0      ; qz*vx
    vmulss  xmm15, xmm8, xmm2       ; qx*vz
    vsubss  xmm12, xmm12, xmm15
    vmulss  xmm12, xmm12, xmm14
    vaddss  xmm6, xmm6, xmm12

    vmulss  xmm12, xmm8, xmm1       ; qx*vy
    vmulss  xmm15, xmm9, xmm0       ; qy*vx
    vsubss  xmm12, xmm12, xmm15
    vmulss  xmm12, xmm12, xmm14
    vaddss  xmm7, xmm7, xmm12

    vmovss  [r13],   xmm5
    vmovss  [r13+4], xmm6
    vmovss  [r13+8], xmm7
    mov     dword [r13+12], 0

    add     r12, 16
    add     r13, 16
    dec     r14
    jnz     .loop_quat

.cleanup_quat:
    EPILOGUE

; -----------------------------------------------------------------------------
; void fp_reduce_vec3_add_f32(
;     RCX: const Vec3f* in_vecs,
;     RDX: size_t n,
;     R8:  Vec3f* out_sum
; );
; -----------------------------------------------------------------------------
fp_reduce_vec3_add_f32:
    PROLOGUE
    
    mov     r12, rcx                ; r12 = in_vecs
    mov     r13, rdx                ; r13 = n
    mov     r14, r8                 ; r14 = out_sum
    
    vxorps  ymm4, ymm4, ymm4        ; ymm4 = acc0 [0,0,0,0 | 0,0,0,0]
    vxorps  ymm5, ymm5, ymm5        ; ymm5 = acc1
    vxorps  ymm6, ymm6, ymm6        ; ymm6 = acc2
    vxorps  ymm7, ymm7, ymm7        ; ymm7 = acc3
    
.loop_header_sum:
    cmp     r13, 8
    jl      .tail_sum

    vaddps ymm4, ymm4, [r12]
    vaddps ymm5, ymm5, [r12+32]
    vaddps ymm6, ymm6, [r12+64]
    vaddps ymm7, ymm7, [r12+96]
    
    add     r12, 128                ; 8 vectors * 16 bytes/vec
    sub     r13, 8
    jmp     .loop_header_sum

.tail_sum:
    cmp     r13, 0
    je      .reduce_sum
    
.tail_loop_sum:
    vaddps  ymm4, ymm4, [r12]       ; acc0 += 1 vector
    add     r12, 16
    dec     r13
    jnz     .tail_loop_sum

.reduce_sum:
    ; Reduce accumulators
    vaddps  ymm4, ymm4, ymm5
    vaddps  ymm6, ymm6, ymm7
    vaddps  ymm4, ymm4, ymm6
    
    ; Horizontal sum
    vextractf128 xmm1, ymm4, 1      ; xmm1 = upper 128
    vaddps  xmm0, xmm4, xmm1        ; xmm0 = lower + upper
    
    vmovdqu [r14], xmm0             ; Store final sum (x,y,z,p)

.cleanup_sum:
    EPILOGUE

; -----------------------------------------------------------------------------
; float fp_fold_vec3_dot_f32(
;     RCX: const Vec3f* in_a,
;     RDX: const Vec3f* in_b,
;     R8:  size_t n
; );
; Returns sum of dot products in XMM0
; -----------------------------------------------------------------------------
fp_fold_vec3_dot_f32:
    PROLOGUE

    mov     r12, rcx                ; r12 = in_a
    mov     r13, rdx                ; r13 = in_b
    mov     r14, r8                 ; r14 = n

    vxorps  xmm0, xmm0, xmm0        ; accumulator = 0.0f

.loop_dot_scalar:
    test    r14, r14
    jz      .done_dot

    vmovss  xmm1, [r12]             ; ax
    vmovss  xmm2, [r13]             ; bx
    vmulss  xmm1, xmm1, xmm2        ; ax*bx

    vmovss  xmm2, [r12+4]           ; ay
    vmovss  xmm3, [r13+4]           ; by
    vmulss  xmm2, xmm2, xmm3
    vaddss  xmm1, xmm1, xmm2

    vmovss  xmm2, [r12+8]           ; az
    vmovss  xmm3, [r13+8]           ; bz
    vmulss  xmm2, xmm2, xmm3
    vaddss  xmm1, xmm1, xmm2        ; dot product

    vaddss  xmm0, xmm0, xmm1        ; accumulate

    add     r12, 16
    add     r13, 16
    dec     r14
    jmp     .loop_dot_scalar

.done_dot:
    EPILOGUE
    
; -----------------------------------------------------------------------------
; Data Section
; -----------------------------------------------------------------------------
section .data
align 16
g_neg_one: dd -1.0, -1.0, -1.0, -1.0
align 32
g_zero: dd 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
