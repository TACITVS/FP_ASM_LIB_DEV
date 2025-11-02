; fp_core_linear_regression.asm
;
; Linear Regression - Algorithm #4
; Fit y = mx + b to data, compute R², standard error, make predictions
; Uses single-pass algorithm for coefficient calculation
;
; Windows x64 calling convention

section .text

; ============================================================================
; fp_linear_regression_f64
;
; Compute linear regression: y = mx + b
; Also calculates R² (coefficient of determination) and standard error
;
; Arguments:
;   RCX = const double* x (independent variable)
;   RDX = const double* y (dependent variable)
;   R8  = size_t n (number of data points)
;   R9  = LinearRegression* result (output struct)
;
; Returns: void (results stored in output struct)
;
; LinearRegression struct layout:
;   +0:  double slope
;   +8:  double intercept
;   +16: double r_squared
;   +24: double std_error
; ============================================================================
global fp_linear_regression_f64
fp_linear_regression_f64:
    push rbp
    mov rbp, rsp
    sub rsp, 128

    ; Handle edge cases
    test r8, r8
    jz .empty_array

    cmp r8, 1
    je .single_point

    cmp r8, 2
    je .two_points

    ; Save non-volatile registers
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; Save arguments
    mov [rbp - 8], rcx          ; x pointer
    mov [rbp - 16], rdx         ; y pointer
    mov [rbp - 24], r8          ; n
    mov [rbp - 32], r9          ; result pointer

    ; PASS 1: Compute sum_x, sum_y, sum_x2, sum_y2, sum_xy
    vxorpd ymm0, ymm0, ymm0     ; sum_x
    vxorpd ymm1, ymm1, ymm1     ; sum_y
    vxorpd ymm2, ymm2, ymm2     ; sum_x2
    vxorpd ymm3, ymm3, ymm3     ; sum_y2
    vxorpd ymm8, ymm8, ymm8     ; sum_xy

    mov r12, rcx                ; x pointer
    mov r13, rdx                ; y pointer
    xor rbx, rbx                ; index
    mov r10, r8                 ; remaining count

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
    vhaddpd xmm0, xmm0, xmm0     ; sum_x in xmm0

    vextractf128 xmm4, ymm1, 1
    vaddpd xmm1, xmm1, xmm4
    vhaddpd xmm1, xmm1, xmm1     ; sum_y in xmm1

    vextractf128 xmm4, ymm2, 1
    vaddpd xmm2, xmm2, xmm4
    vhaddpd xmm2, xmm2, xmm2     ; sum_x2 in xmm2

    vextractf128 xmm4, ymm3, 1
    vaddpd xmm3, xmm3, xmm4
    vhaddpd xmm3, xmm3, xmm3     ; sum_y2 in xmm3

    vextractf128 xmm4, ymm8, 1
    vaddpd xmm8, xmm8, xmm4
    vhaddpd xmm8, xmm8, xmm8     ; sum_xy in xmm8

    ; Process remaining elements
    test r10, r10
    jz .compute_coefficients

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

.compute_coefficients:
    ; Save sums for later use
    vmovsd [rbp - 40], xmm0      ; sum_x
    vmovsd [rbp - 48], xmm1      ; sum_y
    vmovsd [rbp - 56], xmm2      ; sum_x2
    vmovsd [rbp - 64], xmm3      ; sum_y2
    vmovsd [rbp - 72], xmm8      ; sum_xy

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
    vsubsd xmm12, xmm12, xmm13   ; xmm12 = var_x

    ; var_y = (sum_y2 / n) - mean_y^2
    vdivsd xmm13, xmm3, xmm15    ; sum_y2 / n
    vmulsd xmm14, xmm11, xmm11   ; mean_y^2
    vsubsd xmm13, xmm13, xmm14   ; xmm13 = var_y

    ; cov_xy = (sum_xy / n) - mean_x * mean_y
    vdivsd xmm14, xmm8, xmm15    ; sum_xy / n
    vmulsd xmm9, xmm10, xmm11    ; mean_x * mean_y
    vsubsd xmm14, xmm14, xmm9    ; xmm14 = cov_xy

    ; Check for zero variance in x (would cause division by zero)
    vxorpd xmm9, xmm9, xmm9
    vcomisd xmm12, xmm9
    je .zero_variance_x

    ; slope = cov_xy / var_x
    vdivsd xmm6, xmm14, xmm12    ; xmm6 = slope (m)

    ; intercept = mean_y - slope * mean_x
    vmulsd xmm7, xmm6, xmm10     ; slope * mean_x
    vsubsd xmm7, xmm11, xmm7     ; xmm7 = intercept (b)

    ; Save slope and intercept for later
    vmovsd [rbp - 80], xmm6      ; slope
    vmovsd [rbp - 88], xmm7      ; intercept
    vmovsd [rbp - 96], xmm10     ; mean_x (for pass 2)
    vmovsd [rbp - 104], xmm11    ; mean_y (for pass 2)

    ; Calculate R² = (correlation)^2 = (cov_xy / (stddev_x * stddev_y))^2
    vsqrtsd xmm12, xmm12, xmm12  ; stddev_x
    vsqrtsd xmm13, xmm13, xmm13  ; stddev_y
    vmulsd xmm15, xmm12, xmm13   ; stddev_x * stddev_y
    vdivsd xmm4, xmm14, xmm15    ; correlation
    vmulsd xmm4, xmm4, xmm4      ; xmm4 = R²

    vmovsd [rbp - 112], xmm4     ; Save R²

    ; PASS 2: Compute sum of squared residuals for standard error
    vxorpd xmm0, xmm0, xmm0      ; sum_squared_residuals

    mov r12, [rbp - 8]           ; x pointer
    mov r13, [rbp - 16]          ; y pointer
    xor rbx, rbx                 ; index
    mov r10, [rbp - 24]          ; n

    vmovsd xmm6, [rbp - 80]      ; slope
    vmovsd xmm7, [rbp - 88]      ; intercept

.residuals_loop:
    vmovsd xmm4, [r12 + rbx*8]   ; x_i
    vmovsd xmm5, [r13 + rbx*8]   ; y_i

    ; y_predicted = slope * x_i + intercept
    vmulsd xmm8, xmm6, xmm4      ; slope * x_i
    vaddsd xmm8, xmm8, xmm7      ; y_pred = slope * x_i + intercept

    ; residual = y_i - y_predicted
    vsubsd xmm9, xmm5, xmm8      ; residual

    ; squared_residual = residual^2
    vmulsd xmm9, xmm9, xmm9

    ; sum_squared_residuals += squared_residual
    vaddsd xmm0, xmm0, xmm9

    inc rbx
    cmp rbx, r10
    jb .residuals_loop

    ; std_error = sqrt(sum_squared_residuals / (n - 2))
    mov r8, [rbp - 24]
    sub r8, 2                    ; n - 2 (degrees of freedom)
    vcvtsi2sd xmm1, xmm1, r8
    vdivsd xmm0, xmm0, xmm1      ; variance of residuals
    vsqrtsd xmm0, xmm0, xmm0     ; xmm0 = std_error

    ; Store results in output struct
    mov r9, [rbp - 32]           ; result pointer
    vmovsd xmm6, [rbp - 80]      ; slope
    vmovsd xmm7, [rbp - 88]      ; intercept
    vmovsd xmm4, [rbp - 112]     ; R²

    vmovsd [r9], xmm6            ; result->slope
    vmovsd [r9 + 8], xmm7        ; result->intercept
    vmovsd [r9 + 16], xmm4       ; result->r_squared
    vmovsd [r9 + 24], xmm0       ; result->std_error

    vzeroupper
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    mov rsp, rbp
    pop rbp
    ret

.zero_variance_x:
    ; If x has zero variance, cannot compute regression
    mov r9, [rbp - 32]
    vmovsd xmm0, [rel const_nan]
    vmovsd [r9], xmm0            ; slope = NaN
    vmovsd [r9 + 8], xmm0        ; intercept = NaN
    vmovsd [r9 + 16], xmm0       ; r_squared = NaN
    vmovsd [r9 + 24], xmm0       ; std_error = NaN

    vzeroupper
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    mov rsp, rbp
    pop rbp
    ret

.two_points:
    ; With 2 points, perfect fit through both points
    ; R9 still contains the result pointer from function arguments
    mov r12, rcx                 ; x pointer
    mov r13, rdx                 ; y pointer

    vmovsd xmm0, [r12]           ; x0
    vmovsd xmm1, [r12 + 8]       ; x1
    vmovsd xmm2, [r13]           ; y0
    vmovsd xmm3, [r13 + 8]       ; y1

    ; slope = (y1 - y0) / (x1 - x0)
    vsubsd xmm4, xmm3, xmm2      ; y1 - y0
    vsubsd xmm5, xmm1, xmm0      ; x1 - x0

    ; Check for vertical line (x1 == x0)
    vxorpd xmm6, xmm6, xmm6
    vcomisd xmm5, xmm6
    je .zero_variance_x_early

    vdivsd xmm6, xmm4, xmm5      ; slope

    ; intercept = y0 - slope * x0
    vmulsd xmm7, xmm6, xmm0
    vsubsd xmm7, xmm2, xmm7      ; intercept

    ; For 2 points, R² = 1.0 (perfect fit), std_error = 0.0
    vmovsd xmm4, [rel const_one]
    vxorpd xmm5, xmm5, xmm5

    ; R9 still contains result pointer
    vmovsd [r9], xmm6            ; slope
    vmovsd [r9 + 8], xmm7        ; intercept
    vmovsd [r9 + 16], xmm4       ; r_squared = 1.0
    vmovsd [r9 + 24], xmm5       ; std_error = 0.0

    mov rsp, rbp
    pop rbp
    ret

.zero_variance_x_early:
    ; Early exit for zero variance (before saving registers)
    vmovsd xmm0, [rel const_nan]
    vmovsd [r9], xmm0            ; slope = NaN
    vmovsd [r9 + 8], xmm0        ; intercept = NaN
    vmovsd [r9 + 16], xmm0       ; r_squared = NaN
    vmovsd [r9 + 24], xmm0       ; std_error = NaN

    mov rsp, rbp
    pop rbp
    ret

.single_point:
    ; Single point - undefined regression
    ; R9 still contains result pointer
    vmovsd xmm0, [rel const_nan]
    vmovsd [r9], xmm0
    vmovsd [r9 + 8], xmm0
    vmovsd [r9 + 16], xmm0
    vmovsd [r9 + 24], xmm0

    mov rsp, rbp
    pop rbp
    ret

.empty_array:
    ; Empty array - undefined
    ; R9 still contains result pointer
    vmovsd xmm0, [rel const_nan]
    vmovsd [r9], xmm0
    vmovsd [r9 + 8], xmm0
    vmovsd [r9 + 16], xmm0
    vmovsd [r9 + 24], xmm0

    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_predict_f64
;
; Make prediction using linear regression model: y = mx + b
;
; Arguments:
;   XMM0 = double x_value (input value, position 0)
;   RDX  = const LinearRegression* model (position 1 - pointer goes to RDX!)
;
; NOTE: Mixed float/pointer args - position 0 (float) uses XMM0,
;       position 1 (pointer) uses RDX (NOT RCX!)
;
; Returns: XMM0 = predicted y value
; ============================================================================
global fp_predict_f64
fp_predict_f64:
    push rbp
    mov rbp, rsp

    ; Load model parameters from RDX (not RCX!)
    vmovsd xmm1, [rdx]           ; slope
    vmovsd xmm2, [rdx + 8]       ; intercept

    ; y = slope * x + intercept
    vmulsd xmm0, xmm0, xmm1      ; x * slope
    vaddsd xmm0, xmm0, xmm2      ; + intercept

    pop rbp
    ret

section .data
align 8
const_nan:  dq 0x7FF8000000000000  ; NaN
const_one:  dq 1.0
