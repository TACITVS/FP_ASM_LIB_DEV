; fp_core_correlation.asm
;
; Correlation & Covariance - Algorithm #3
; Calculate Pearson correlation, covariance between arrays
; Uses fused operations for maximum performance
;
; Windows x64 calling convention

section .text

; ============================================================================
; fp_covariance_f64
;
; Calculate covariance between two arrays
; Cov(X,Y) = E[(X - E[X])(Y - E[Y])] = E[XY] - E[X]E[Y]
;
; Arguments:
;   RCX = const double* x (first array)
;   RDX = const double* y (second array)
;   R8  = size_t n (number of elements)
;
; Returns: XMM0 = covariance value
; ============================================================================
global fp_covariance_f64
fp_covariance_f64:
    push rbp
    mov rbp, rsp
    sub rsp, 32

    ; Handle edge cases
    test r8, r8
    jz .empty_array

    cmp r8, 1
    je .single_element

    ; Save non-volatile registers
    push rbx
    push r12
    push r13

    ; Initialize accumulators
    vxorpd ymm0, ymm0, ymm0    ; sum_x
    vxorpd ymm1, ymm1, ymm1    ; sum_y
    vxorpd ymm2, ymm2, ymm2    ; sum_xy

    mov r12, rcx               ; r12 = x pointer
    mov r13, rdx               ; r13 = y pointer
    xor rbx, rbx               ; rbx = index
    mov r10, r8                ; r10 = remaining count

    ; Process 4 elements at a time with AVX2
.loop4:
    cmp r10, 4
    jb .tail

    ; Load 4 x values and 4 y values
    vmovupd ymm4, [r12 + rbx*8]  ; x[i..i+3]
    vmovupd ymm5, [r13 + rbx*8]  ; y[i..i+3]

    ; Accumulate sums
    vaddpd ymm0, ymm0, ymm4      ; sum_x += x
    vaddpd ymm1, ymm1, ymm5      ; sum_y += y

    ; Compute and accumulate x*y
    vmulpd ymm6, ymm4, ymm5      ; x * y
    vaddpd ymm2, ymm2, ymm6      ; sum_xy += x*y

    add rbx, 4
    sub r10, 4
    jmp .loop4

.tail:
    ; Horizontal reduction to scalars
    vextractf128 xmm4, ymm0, 1
    vaddpd xmm0, xmm0, xmm4
    vhaddpd xmm0, xmm0, xmm0     ; xmm0 = sum_x

    vextractf128 xmm4, ymm1, 1
    vaddpd xmm1, xmm1, xmm4
    vhaddpd xmm1, xmm1, xmm1     ; xmm1 = sum_y

    vextractf128 xmm4, ymm2, 1
    vaddpd xmm2, xmm2, xmm4
    vhaddpd xmm2, xmm2, xmm2     ; xmm2 = sum_xy

    ; Process remaining elements
    test r10, r10
    jz .compute_covariance

.tail_loop:
    vmovsd xmm4, [r12 + rbx*8]
    vmovsd xmm5, [r13 + rbx*8]

    vaddsd xmm0, xmm0, xmm4      ; sum_x += x
    vaddsd xmm1, xmm1, xmm5      ; sum_y += y

    vmulsd xmm6, xmm4, xmm5      ; x * y
    vaddsd xmm2, xmm2, xmm6      ; sum_xy += x*y

    inc rbx
    dec r10
    jnz .tail_loop

.compute_covariance:
    ; Convert n to double
    vcvtsi2sd xmm3, xmm3, r8     ; xmm3 = n

    ; mean_x = sum_x / n
    vdivsd xmm4, xmm0, xmm3      ; xmm4 = mean_x

    ; mean_y = sum_y / n
    vdivsd xmm5, xmm1, xmm3      ; xmm5 = mean_y

    ; mean_xy = sum_xy / n
    vdivsd xmm6, xmm2, xmm3      ; xmm6 = mean_xy

    ; cov = mean_xy - mean_x * mean_y
    vmulsd xmm7, xmm4, xmm5      ; mean_x * mean_y
    vsubsd xmm0, xmm6, xmm7      ; cov = mean_xy - mean_x*mean_y

    vzeroupper
    pop r13
    pop r12
    pop rbx
    mov rsp, rbp
    pop rbp
    ret

.single_element:
    ; Covariance of single point is 0
    vxorpd xmm0, xmm0, xmm0
    mov rsp, rbp
    pop rbp
    ret

.empty_array:
    ; Return NaN for empty array
    vmovsd xmm0, [rel const_nan]
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_correlation_f64
;
; Calculate Pearson correlation coefficient
; r = Cov(X,Y) / (StdDev(X) * StdDev(Y))
;
; Arguments:
;   RCX = const double* x
;   RDX = const double* y
;   R8  = size_t n
;
; Returns: XMM0 = correlation coefficient (-1.0 to 1.0)
; ============================================================================
global fp_correlation_f64
fp_correlation_f64:
    push rbp
    mov rbp, rsp
    sub rsp, 96

    ; Handle edge cases
    test r8, r8
    jz .empty_array

    cmp r8, 1
    je .single_element

    ; Save non-volatile registers
    push rbx
    push r12
    push r13
    push r14

    ; Save arguments
    mov [rbp - 8], rcx          ; x
    mov [rbp - 16], rdx         ; y
    mov [rbp - 24], r8          ; n

    ; Initialize accumulators for a single-pass algorithm
    vxorpd ymm0, ymm0, ymm0     ; sum_x
    vxorpd ymm1, ymm1, ymm1     ; sum_y
    vxorpd ymm2, ymm2, ymm2     ; sum_x2
    vxorpd ymm3, ymm3, ymm3     ; sum_y2
    vxorpd ymm8, ymm8, ymm8     ; sum_xy

    mov r12, rcx                ; x pointer
    mov r13, rdx                ; y pointer
    xor rbx, rbx                ; index
    mov r10, r8                 ; remaining count

    ; Single pass: compute all sums at once
.loop4:
    cmp r10, 4
    jb .tail

    vmovupd ymm4, [r12 + rbx*8]  ; x[i..i+3]
    vmovupd ymm5, [r13 + rbx*8]  ; y[i..i+3]

    vaddpd ymm0, ymm0, ymm4      ; sum_x
    vaddpd ymm1, ymm1, ymm5      ; sum_y

    vmulpd ymm6, ymm4, ymm4      ; x^2
    vaddpd ymm2, ymm2, ymm6      ; sum_x2

    vmulpd ymm7, ymm5, ymm5      ; y^2
    vaddpd ymm3, ymm3, ymm7      ; sum_y2

    vmulpd ymm9, ymm4, ymm5      ; x*y
    vaddpd ymm8, ymm8, ymm9      ; sum_xy

    add rbx, 4
    sub r10, 4
    jmp .loop4

.tail:
    ; Horizontal reductions
    vextractf128 xmm4, ymm0, 1
    vaddpd xmm0, xmm0, xmm4
    vhaddpd xmm0, xmm0, xmm0     ; sum_x

    vextractf128 xmm4, ymm1, 1
    vaddpd xmm1, xmm1, xmm4
    vhaddpd xmm1, xmm1, xmm1     ; sum_y

    vextractf128 xmm4, ymm2, 1
    vaddpd xmm2, xmm2, xmm4
    vhaddpd xmm2, xmm2, xmm2     ; sum_x2

    vextractf128 xmm4, ymm3, 1
    vaddpd xmm3, xmm3, xmm4
    vhaddpd xmm3, xmm3, xmm3     ; sum_y2

    vextractf128 xmm4, ymm8, 1
    vaddpd xmm8, xmm8, xmm4
    vhaddpd xmm8, xmm8, xmm8     ; sum_xy

    ; Process remaining elements
    test r10, r10
    jz .compute_correlation

.tail_loop:
    vmovsd xmm4, [r12 + rbx*8]   ; x
    vmovsd xmm5, [r13 + rbx*8]   ; y

    vaddsd xmm0, xmm0, xmm4      ; sum_x
    vaddsd xmm1, xmm1, xmm5      ; sum_y

    vmulsd xmm6, xmm4, xmm4
    vaddsd xmm2, xmm2, xmm6      ; sum_x2

    vmulsd xmm7, xmm5, xmm5
    vaddsd xmm3, xmm3, xmm7      ; sum_y2

    vmulsd xmm9, xmm4, xmm5
    vaddsd xmm8, xmm8, xmm9      ; sum_xy

    inc rbx
    dec r10
    jnz .tail_loop

.compute_correlation:
    ; Convert n to double
    mov r8, [rbp - 24]
    vcvtsi2sd xmm15, xmm15, r8   ; xmm15 = n

    ; mean_x = sum_x / n
    vdivsd xmm10, xmm0, xmm15    ; xmm10 = mean_x

    ; mean_y = sum_y / n
    vdivsd xmm11, xmm1, xmm15    ; xmm11 = mean_y

    ; var_x = (sum_x2 / n) - mean_x^2
    vdivsd xmm12, xmm2, xmm15    ; sum_x2 / n
    vmulsd xmm13, xmm10, xmm10   ; mean_x^2
    vsubsd xmm12, xmm12, xmm13   ; var_x

    ; var_y = (sum_y2 / n) - mean_y^2
    vdivsd xmm13, xmm3, xmm15    ; sum_y2 / n
    vmulsd xmm14, xmm11, xmm11   ; mean_y^2
    vsubsd xmm13, xmm13, xmm14   ; var_y

    ; cov = (sum_xy / n) - mean_x * mean_y
    vdivsd xmm14, xmm8, xmm15    ; sum_xy / n
    vmulsd xmm9, xmm10, xmm11    ; mean_x * mean_y
    vsubsd xmm14, xmm14, xmm9    ; cov

    ; Check for zero variance (would cause division by zero)
    vxorpd xmm9, xmm9, xmm9
    vcomisd xmm12, xmm9
    je .zero_variance
    vcomisd xmm13, xmm9
    je .zero_variance

    ; stddev_x = sqrt(var_x)
    vsqrtsd xmm12, xmm12, xmm12

    ; stddev_y = sqrt(var_y)
    vsqrtsd xmm13, xmm13, xmm13

    ; correlation = cov / (stddev_x * stddev_y)
    vmulsd xmm15, xmm12, xmm13   ; stddev_x * stddev_y
    vdivsd xmm0, xmm14, xmm15    ; r = cov / (std_x * std_y)

    vzeroupper
    pop r14
    pop r13
    pop r12
    pop rbx
    mov rsp, rbp
    pop rbp
    ret

.zero_variance:
    ; If either variable has zero variance, correlation is undefined (return NaN)
    vmovsd xmm0, [rel const_nan]
    vzeroupper
    pop r14
    pop r13
    pop r12
    pop rbx
    mov rsp, rbp
    pop rbp
    ret

.single_element:
    ; Correlation undefined for single point
    vmovsd xmm0, [rel const_nan]
    mov rsp, rbp
    pop rbp
    ret

.empty_array:
    vmovsd xmm0, [rel const_nan]
    mov rsp, rbp
    pop rbp
    ret

section .data
align 8
const_nan:  dq 0x7FF8000000000000  ; NaN
