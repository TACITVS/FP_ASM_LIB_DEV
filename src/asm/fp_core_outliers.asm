; fp_core_outliers.asm
;
; Outlier Detection - Algorithm #5
; Detect anomalous values using Z-score and IQR methods
; Uses SIMD for statistics computation
;
; Windows x64 calling convention

section .text

; ============================================================================
; fp_detect_outliers_zscore_f64
;
; Detect outliers using Z-score method
; A point is an outlier if |z| > threshold, where z = (x - mean) / stddev
;
; Arguments:
;   RCX = const double* data (input array)
;   RDX = size_t n (number of elements)
;   XMM2 = double threshold (typically 3.0 for 3 standard deviations)
;   R9  = uint8_t* is_outlier (output: 1 if outlier, 0 otherwise)
;
; Returns: RAX = number of outliers detected
; ============================================================================
global fp_detect_outliers_zscore_f64
fp_detect_outliers_zscore_f64:
    push rbp
    mov rbp, rsp
    sub rsp, 96

    ; Handle edge cases
    test rdx, rdx
    jz .empty_array

    cmp rdx, 1
    je .single_element

    ; Save non-volatile registers
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; Save arguments
    mov [rbp - 8], rcx          ; data
    mov [rbp - 16], rdx         ; n
    vmovsd [rbp - 24], xmm2     ; threshold
    mov [rbp - 32], r9          ; is_outlier

    ; PASS 1: Compute mean and variance using SIMD
    vxorpd ymm0, ymm0, ymm0     ; sum
    vxorpd ymm1, ymm1, ymm1     ; sum_sq

    mov r12, rcx                ; data pointer
    xor rbx, rbx                ; index
    mov r10, rdx                ; remaining count

.loop4_stats:
    cmp r10, 4
    jb .tail_stats

    vmovupd ymm4, [r12 + rbx*8]  ; data[i..i+3]
    vaddpd ymm0, ymm0, ymm4      ; sum
    vmulpd ymm5, ymm4, ymm4      ; data^2
    vaddpd ymm1, ymm1, ymm5      ; sum_sq

    add rbx, 4
    sub r10, 4
    jmp .loop4_stats

.tail_stats:
    ; Horizontal reduction
    vextractf128 xmm4, ymm0, 1
    vaddpd xmm0, xmm0, xmm4
    vhaddpd xmm0, xmm0, xmm0     ; sum

    vextractf128 xmm4, ymm1, 1
    vaddpd xmm1, xmm1, xmm4
    vhaddpd xmm1, xmm1, xmm1     ; sum_sq

    ; Process remaining elements
    test r10, r10
    jz .compute_mean_std

.tail_loop_stats:
    vmovsd xmm4, [r12 + rbx*8]
    vaddsd xmm0, xmm0, xmm4      ; sum
    vmulsd xmm5, xmm4, xmm4
    vaddsd xmm1, xmm1, xmm5      ; sum_sq

    inc rbx
    dec r10
    jnz .tail_loop_stats

.compute_mean_std:
    ; mean = sum / n
    mov rdx, [rbp - 16]
    vcvtsi2sd xmm15, xmm15, rdx  ; n as double
    vdivsd xmm10, xmm0, xmm15    ; xmm10 = mean

    ; variance = (sum_sq / n) - mean^2
    vdivsd xmm11, xmm1, xmm15    ; sum_sq / n
    vmulsd xmm12, xmm10, xmm10   ; mean^2
    vsubsd xmm11, xmm11, xmm12   ; variance

    ; stddev = sqrt(variance)
    vsqrtsd xmm11, xmm11, xmm11  ; xmm11 = stddev

    ; Check for zero stddev
    vxorpd xmm12, xmm12, xmm12
    vcomisd xmm11, xmm12
    je .zero_stddev

    ; PASS 2: Mark outliers
    vmovsd xmm13, [rbp - 24]     ; threshold
    mov r12, [rbp - 8]           ; data pointer
    mov r13, [rbp - 32]          ; is_outlier pointer
    xor r14, r14                 ; outlier count
    xor rbx, rbx                 ; index

.outlier_loop:
    cmp rbx, rdx
    jge .done

    ; z = (data[i] - mean) / stddev
    vmovsd xmm0, [r12 + rbx*8]
    vsubsd xmm0, xmm0, xmm10     ; data - mean
    vdivsd xmm0, xmm0, xmm11     ; z-score

    ; |z|
    vandpd xmm0, xmm0, [rel const_abs_mask]

    ; Compare |z| > threshold
    vcomisd xmm0, xmm13
    ja .is_outlier

    ; Not an outlier
    mov byte [r13 + rbx], 0
    jmp .next_point

.is_outlier:
    mov byte [r13 + rbx], 1
    inc r14

.next_point:
    inc rbx
    jmp .outlier_loop

.done:
    mov rax, r14                 ; return outlier count
    vzeroupper
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    mov rsp, rbp
    pop rbp
    ret

.zero_stddev:
    ; All values identical - no outliers
    mov r13, [rbp - 32]
    xor rbx, rbx
.zero_loop:
    cmp rbx, rdx
    jge .zero_done
    mov byte [r13 + rbx], 0
    inc rbx
    jmp .zero_loop
.zero_done:
    xor rax, rax                 ; 0 outliers
    vzeroupper
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    mov rsp, rbp
    pop rbp
    ret

.single_element:
    ; Single element is not an outlier
    mov byte [r9], 0
    xor rax, rax
    mov rsp, rbp
    pop rbp
    ret

.empty_array:
    xor rax, rax
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_detect_outliers_iqr_sorted_f64
;
; Detect outliers using IQR (Interquartile Range) method
; Requires sorted data
;
; NOTE: This is an internal function. User-facing API is in fp_percentile_wrappers.c
; which handles sorting automatically.
;
; A point is an outlier if x < Q1 - k*IQR or x > Q3 + k*IQR
; where k is typically 1.5
;
; Arguments:
;   RCX = const double* sorted_data (input array, MUST BE SORTED!)
;   RDX = size_t n
;   XMM2 = double k_factor (typically 1.5)
;   R9  = uint8_t* is_outlier (output array)
;
; Returns: RAX = number of outliers detected
; ============================================================================
global fp_detect_outliers_iqr_sorted_f64
fp_detect_outliers_iqr_sorted_f64:
    push rbp
    mov rbp, rsp
    sub rsp, 96

    ; Handle edge cases
    test rdx, rdx
    jz .empty_array_iqr

    cmp rdx, 3
    jb .too_few_iqr

    ; Save non-volatile registers
    push rbx
    push r12
    push r13
    push r14

    ; Save arguments
    mov [rbp - 8], rcx          ; sorted_data
    mov [rbp - 16], rdx         ; n
    vmovsd [rbp - 24], xmm2     ; k_factor
    mov [rbp - 32], r9          ; is_outlier

    ; Calculate Q1 (25th percentile)
    vmovsd xmm2, [rel const_025]
    call fp_percentile_sorted_f64
    vmovsd [rbp - 40], xmm0     ; Q1

    ; Calculate Q3 (75th percentile)
    mov rcx, [rbp - 8]
    mov rdx, [rbp - 16]
    vmovsd xmm2, [rel const_075]
    call fp_percentile_sorted_f64
    vmovsd [rbp - 48], xmm0     ; Q3

    ; Calculate IQR = Q3 - Q1
    vmovsd xmm1, [rbp - 40]     ; Q1
    vsubsd xmm2, xmm0, xmm1     ; IQR

    ; Calculate bounds
    vmovsd xmm3, [rbp - 24]     ; k_factor
    vmulsd xmm4, xmm2, xmm3     ; k * IQR

    ; lower_bound = Q1 - k*IQR
    vsubsd xmm5, xmm1, xmm4     ; xmm5 = lower_bound

    ; upper_bound = Q3 + k*IQR
    vmovsd xmm6, [rbp - 48]     ; Q3
    vaddsd xmm6, xmm6, xmm4     ; xmm6 = upper_bound

    ; Mark outliers
    mov r12, [rbp - 8]          ; data pointer
    mov r13, [rbp - 32]         ; is_outlier pointer
    mov rdx, [rbp - 16]         ; n
    xor r14, r14                ; outlier count
    xor rbx, rbx                ; index

.iqr_loop:
    cmp rbx, rdx
    jge .iqr_done

    vmovsd xmm0, [r12 + rbx*8]

    ; Check if x < lower_bound
    vcomisd xmm0, xmm5
    jb .is_outlier_iqr

    ; Check if x > upper_bound
    vcomisd xmm0, xmm6
    ja .is_outlier_iqr

    ; Not an outlier
    mov byte [r13 + rbx], 0
    jmp .next_iqr

.is_outlier_iqr:
    mov byte [r13 + rbx], 1
    inc r14

.next_iqr:
    inc rbx
    jmp .iqr_loop

.iqr_done:
    mov rax, r14
    pop r14
    pop r13
    pop r12
    pop rbx
    mov rsp, rbp
    pop rbp
    ret

.too_few_iqr:
    ; Too few points for IQR method - mark all as non-outliers
    xor rbx, rbx
.too_few_loop:
    cmp rbx, rdx
    jge .too_few_done
    mov byte [r9 + rbx], 0
    inc rbx
    jmp .too_few_loop
.too_few_done:
    xor rax, rax
    mov rsp, rbp
    pop rbp
    ret

.empty_array_iqr:
    xor rax, rax
    mov rsp, rbp
    pop rbp
    ret

; External function declaration
extern fp_percentile_sorted_f64

section .data
align 8
const_abs_mask:  dq 0x7FFFFFFFFFFFFFFF  ; Mask to clear sign bit
const_025:       dq 0.25
const_075:       dq 0.75
