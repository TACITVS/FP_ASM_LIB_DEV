; =============================================================================
; FP-ASM Library: 3D Math Functions (f32)
;
; CORRECTED VERSION:
; - Removed all non-breaking space characters (0xA0)
; - Fixed all functions to use ONLY AVX2 registers (ymm0-ymm15)
; - Matched fp_map_transform_vec3_f32 to column-major layout (OpenGL style)
;   and implemented a 2-vector-at-a-time loop with full w preservation.
; - Fixed fp_fold_vec3_dot_f32 to use 4 accumulators (ymm4-ymm7).
; =============================================================================
default rel

%include "macros.inc"

section .text
    global fp_map_transform_vec3_f32
    global fp_zipWith_vec3_add_f32
    global fp_map_quat_rotate_vec3_f32
    global fp_reduce_vec3_add_f32
    global fp_fold_vec3_dot_f32
    global fp_quat_normalize_asm


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

    ; Store 2 transformed vectors
    vmovdqu [r13], ymm8

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
    cmp     r15, 16
    jl      .loop8_add

    ; Process 16 vectors (8 YMM) per iteration to reduce loop overhead
    vmovdqu ymm0, [r12]
    vmovdqu ymm1, [r12+32]
    vmovdqu ymm2, [r12+64]
    vmovdqu ymm3, [r12+96]
    vmovdqu ymm4, [r12+128]
    vmovdqu ymm5, [r12+160]
    vmovdqu ymm6, [r12+192]
    vmovdqu ymm7, [r12+224]

    vaddps  ymm0, ymm0, [r13]
    vaddps  ymm1, ymm1, [r13+32]
    vaddps  ymm2, ymm2, [r13+64]
    vaddps  ymm3, ymm3, [r13+96]
    vaddps  ymm4, ymm4, [r13+128]
    vaddps  ymm5, ymm5, [r13+160]
    vaddps  ymm6, ymm6, [r13+192]
    vaddps  ymm7, ymm7, [r13+224]

    vmovdqu [r14],      ymm0
    vmovdqu [r14+32],   ymm1
    vmovdqu [r14+64],   ymm2
    vmovdqu [r14+96],   ymm3
    vmovdqu [r14+128],  ymm4
    vmovdqu [r14+160],  ymm5
    vmovdqu [r14+192],  ymm6
    vmovdqu [r14+224],  ymm7

    add     r12, 256                ; 16 vectors * 16 bytes/vec
    add     r13, 256
    add     r14, 256
    sub     r15, 16
    jmp     .loop_header_add

.loop8_add:
    cmp     r15, 8
    jl      .tail_add

    vmovdqu ymm0, [r12]
    vmovdqu ymm1, [r12+32]
    vmovdqu ymm2, [r12+64]
    vmovdqu ymm3, [r12+96]
    
    vaddps  ymm0, ymm0, [r13]
    vaddps  ymm1, ymm1, [r13+32]
    vaddps  ymm2, ymm2, [r13+64]
    vaddps  ymm3, ymm3, [r13+96]
    
    vmovdqu [r14], ymm0
    vmovdqu [r14+32], ymm1
    vmovdqu [r14+64], ymm2
    vmovdqu [r14+96], ymm3

    add     r12, 128
    add     r13, 128
    add     r14, 128
    sub     r15, 8
    jmp     .loop8_add

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
; Quaternion rotation optimized with SIMD math
; -----------------------------------------------------------------------------
fp_map_quat_rotate_vec3_f32:
    PROLOGUE

    mov     r12, rcx                ; r12 = in_vecs
    mov     r13, rdx                ; r13 = out_vecs
    mov     r14, r8                 ; r14 = n
    ; Load quaternion (u = xyz, s = w) and precompute constants
    vmovups xmm8, [r9]              ; u = [qx,qy,qz, qw]
    vbroadcastss xmm11, [r9+12]     ; s broadcast
    vxorps xmm13, xmm13, xmm13
    vblendps xmm8, xmm8, xmm13, 0x8 ; zero w lane for u

    ; Precompute 2*s (broadcast)
    vaddss  xmm10, xmm11, xmm11
    vbroadcastss xmm10, xmm10

.loop_quat:
    test    r14, r14
    jz      .cleanup_quat
    cmp     r14, 2
    jb      .loop_quat_single

    vmovups xmm0, [r12]             ; v

    ; cross(u, v)
    vshufps xmm1, xmm8, xmm8, 0xC9  ; u_y, u_z, u_x
    vshufps xmm2, xmm0, xmm0, 0xD2  ; v_z, v_x, v_y
    vmulps  xmm3, xmm1, xmm2

    vshufps xmm4, xmm8, xmm8, 0xD2  ; u_z, u_x, u_y
    vshufps xmm5, xmm0, xmm0, 0xC9  ; v_y, v_z, v_x
    vmulps  xmm4, xmm4, xmm5

    vsubps  xmm3, xmm3, xmm4        ; cross(u,v)

    ; t = 2 * cross(u, v)
    vaddps  xmm3, xmm3, xmm3

    ; term = v + s * t
    vmulps  xmm4, xmm3, xmm11
    vaddps  xmm0, xmm0, xmm4

    ; cross(u, t)
    vshufps xmm1, xmm8, xmm8, 0xC9
    vshufps xmm2, xmm3, xmm3, 0xD2
    vmulps  xmm4, xmm1, xmm2

    vshufps xmm5, xmm3, xmm3, 0xC9
    vshufps xmm6, xmm8, xmm8, 0xD2
    vmulps  xmm5, xmm5, xmm6

    vsubps  xmm4, xmm4, xmm5

    ; result = term + cross(u,t)
    vaddps  xmm0, xmm0, xmm4
    vblendps xmm0, xmm0, xmm13, 0x8  ; zero w lane

    vmovups [r13], xmm0

    ; second vector in the pair
    vmovups xmm0, [r12+16]

    vshufps xmm1, xmm8, xmm8, 0xC9
    vshufps xmm2, xmm0, xmm0, 0xD2
    vmulps  xmm3, xmm1, xmm2

    vshufps xmm4, xmm8, xmm8, 0xD2
    vshufps xmm5, xmm0, xmm0, 0xC9
    vmulps  xmm4, xmm4, xmm5

    vsubps  xmm3, xmm3, xmm4

    vaddps  xmm3, xmm3, xmm3

    vmulps  xmm4, xmm3, xmm11
    vaddps  xmm0, xmm0, xmm4

    vshufps xmm1, xmm8, xmm8, 0xC9
    vshufps xmm2, xmm3, xmm3, 0xD2
    vmulps  xmm4, xmm1, xmm2

    vshufps xmm5, xmm3, xmm3, 0xC9
    vshufps xmm6, xmm8, xmm8, 0xD2
    vmulps  xmm5, xmm5, xmm6

    vsubps  xmm4, xmm4, xmm5

    vaddps  xmm0, xmm0, xmm4
    vblendps xmm0, xmm0, xmm13, 0x8

    vmovups [r13+16], xmm0

    add     r12, 32
    add     r13, 32
    sub     r14, 2
    jnz     .loop_quat
    jmp     .cleanup_quat

.loop_quat_single:
    vmovups xmm0, [r12]             ; v

    vshufps xmm1, xmm8, xmm8, 0xC9
    vshufps xmm2, xmm0, xmm0, 0xD2
    vmulps  xmm3, xmm1, xmm2

    vshufps xmm4, xmm8, xmm8, 0xD2
    vshufps xmm5, xmm0, xmm0, 0xC9
    vmulps  xmm4, xmm4, xmm5

    vsubps  xmm3, xmm3, xmm4

    vaddps  xmm3, xmm3, xmm3

    vmulps  xmm4, xmm3, xmm11
    vaddps  xmm0, xmm0, xmm4

    vshufps xmm1, xmm8, xmm8, 0xC9
    vshufps xmm2, xmm3, xmm3, 0xD2
    vmulps  xmm4, xmm1, xmm2

    vshufps xmm5, xmm3, xmm3, 0xC9
    vshufps xmm6, xmm8, xmm8, 0xD2
    vmulps  xmm5, xmm5, xmm6

    vsubps  xmm4, xmm4, xmm5

    vaddps  xmm0, xmm0, xmm4
    vblendps xmm0, xmm0, xmm13, 0x8

    vmovups [r13], xmm0

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

    vxorps  xmm0, xmm0, xmm0        ; tail accumulator (xmm to avoid overread)

.tail_loop_sum:
    vaddps  xmm0, xmm0, [r12]       ; add single Vec3 (16 bytes safe)
    add     r12, 16
    dec     r13
    jnz     .tail_loop_sum

    vxorps  ymm0, ymm0, ymm0
    vinsertf128 ymm0, ymm0, xmm0, 0 ; move tail sum into lower lane
    vaddps  ymm4, ymm4, ymm0        ; fold tail into main accumulator

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

    vxorps  xmm4, xmm4, xmm4        ; acc0
    vxorps  xmm5, xmm5, xmm5        ; acc1
    vxorps  xmm6, xmm6, xmm6        ; acc2
    vxorps  xmm7, xmm7, xmm7        ; acc3

.loop_dot_vec4:
    cmp     r14, 4
    jb      .tail_dot

    vmovups xmm0, [r12]             ; a0
    vmovups xmm1, [r13]             ; b0
    vdpps   xmm0, xmm0, xmm1, 0x71  ; dot(a0,b0) in xmm0[0]
    vaddss  xmm4, xmm4, xmm0

    vmovups xmm0, [r12+16]          ; a1
    vmovups xmm1, [r13+16]          ; b1
    vdpps   xmm0, xmm0, xmm1, 0x71
    vaddss  xmm5, xmm5, xmm0

    vmovups xmm0, [r12+32]          ; a2
    vmovups xmm1, [r13+32]          ; b2
    vdpps   xmm0, xmm0, xmm1, 0x71
    vaddss  xmm6, xmm6, xmm0

    vmovups xmm0, [r12+48]          ; a3
    vmovups xmm1, [r13+48]          ; b3
    vdpps   xmm0, xmm0, xmm1, 0x71
    vaddss  xmm7, xmm7, xmm0

    add     r12, 64
    add     r13, 64
    sub     r14, 4
    jmp     .loop_dot_vec4

.tail_dot:
    test    r14, r14
    jz      .reduce_dot

.tail_loop_dot:
    vmovss  xmm0, [r12]             ; ax
    vmovss  xmm1, [r13]             ; bx
    vmulss  xmm0, xmm0, xmm1        ; ax*bx

    vmovss  xmm1, [r12+4]           ; ay
    vmovss  xmm2, [r13+4]           ; by
    vmulss  xmm1, xmm1, xmm2
    vaddss  xmm0, xmm0, xmm1

    vmovss  xmm1, [r12+8]           ; az
    vmovss  xmm2, [r13+8]           ; bz
    vmulss  xmm1, xmm1, xmm2
    vaddss  xmm0, xmm0, xmm1        ; dot product

    vaddss  xmm4, xmm4, xmm0        ; accumulate into acc0

    add     r12, 16
    add     r13, 16
    dec     r14
    jnz     .tail_loop_dot

.reduce_dot:
    vaddss  xmm4, xmm4, xmm5
    vaddss  xmm6, xmm6, xmm7
    vaddss  xmm0, xmm4, xmm6        ; final sum in xmm0

    EPILOGUE
    
; -----------------------------------------------------------------------------
; void fp_quat_normalize(
;     RCX: Quaternion* out,
;     RDX: const Quaternion* q
; );
;
; Normalize a quaternion to unit length using a fast inverse square root
; (rsqrt + 2x Newton-Raphson refinement) to beat scalar sqrt/div from GCC.
;
; Behavior:
;   - If |q|^2 < 1e-8f, returns identity quaternion [0,0,0,1]
;   - Otherwise, out = q / |q|
;
; This should match fp_quat_normalize_pure_c within 1e-6f on all components
; so that Phase 3 L0 verification tests pass.
; -----------------------------------------------------------------------------
fp_quat_normalize_asm:
    ; Leaf function: uses only volatile XMM registers, no stack frame needed.
    ; Load quaternion q = [x, y, z, w]
    vmovups xmm0, [rdx]

    ; Compute len_sq = x*x + y*y + z*z + w*w
    vmulps xmm1, xmm0, xmm0        ; squares
    HSUM_F32_XMM 1                 ; horizontal sum in xmm1[0]

    ; If len_sq < epsilon, return identity quaternion
    vmovss xmm2, [g_quat_eps]
    vucomiss xmm1, xmm2
    jb      .near_zero

    ; Fast inverse square root with two Newton-Raphson refinements
    ; y0 = rsqrt(len_sq)
    vrsqrtss xmm2, xmm1, xmm1      ; xmm2 = y

    ; First refinement: y = y * (1.5f - 0.5f * x * y * y)
    vmulss xmm3, xmm2, xmm2        ; y*y
    vmulss xmm3, xmm3, xmm1        ; x*y*y
    vmovss xmm4, [g_quat_half]     ; 0.5f
    vmulss xmm3, xmm3, xmm4        ; 0.5f*x*y*y
    vmovss xmm5, [g_quat_three]    ; 1.5f
    vsubss xmm5, xmm5, xmm3        ; 1.5f - 0.5f*x*y*y
    vmulss xmm2, xmm2, xmm5        ; y *= (1.5f - 0.5f*x*y*y)

    ; Second refinement for higher precision
    vmulss xmm3, xmm2, xmm2
    vmulss xmm3, xmm3, xmm1
    vmovss xmm4, [g_quat_half]
    vmulss xmm3, xmm3, xmm4
    vmovss xmm5, [g_quat_three]
    vsubss xmm5, xmm5, xmm3
    vmulss xmm2, xmm2, xmm5        ; final y = 1/sqrt(len_sq)

    ; Broadcast inv_len to all lanes and scale quaternion
    vbroadcastss xmm2, xmm2
    vmulps xmm0, xmm0, xmm2
    vmovups [rcx], xmm0
    jmp     .done

.near_zero:
    ; Out-of-range or near-zero length: return identity quaternion
    vmovups xmm0, [g_quat_identity]
    vmovups [rcx], xmm0

.done:
    vzeroupper
    ret
    
; -----------------------------------------------------------------------------
; Data Section
; -----------------------------------------------------------------------------
section .data
align 16
g_neg_one: dd -1.0, -1.0, -1.0, -1.0
align 32
g_zero: dd 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
align 16
g_quat_identity: dd 0.0, 0.0, 0.0, 1.0
align 16
g_quat_eps: dd 0.00000001
align 16
g_quat_half: dd 0.5
align 16
g_quat_three: dd 1.5
