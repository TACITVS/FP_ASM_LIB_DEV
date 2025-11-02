; fp_core_percentiles.asm
;
; Percentile Calculations - Algorithm #2
; Calculate percentiles, quartiles, median from sorted data
; Uses linear interpolation for exact percentile values
;
; Windows x64 calling convention

section .text

; ============================================================================
; fp_percentile_sorted_f64
;
; Calculate a single percentile from sorted data
; Uses linear interpolation between adjacent values
;
; NOTE: This is an internal function. User-facing API is in fp_percentile_wrappers.c
; which handles sorting automatically.
;
; Arguments:
;   RCX = const double* sorted_data (input array, MUST BE SORTED!)
;   RDX = size_t n (number of elements)
;   XMM2 = double p (percentile, 0.0 to 1.0, e.g. 0.5 for median)
;
; Returns: XMM0 = percentile value
; ============================================================================
global fp_percentile_sorted_f64
fp_percentile_sorted_f64:
    push rbp
    mov rbp, rsp
    sub rsp, 32

    ; Handle edge cases
    test rdx, rdx
    jz .empty_array

    cmp rdx, 1
    je .single_element

    ; Calculate position: pos = p * (n - 1)
    dec rdx                     ; rdx = n - 1
    vcvtsi2sd xmm0, xmm0, rdx   ; xmm0 = (n-1) as double
    vmulsd xmm0, xmm2, xmm0     ; xmm0 = p * (n-1) = position
    inc rdx                     ; restore n

    ; Get integer and fractional parts
    vroundsd xmm1, xmm0, xmm0, 0x01  ; xmm1 = floor(pos)
    vsubsd xmm3, xmm0, xmm1          ; xmm3 = frac = pos - floor(pos)

    ; Convert floor to integer index
    vcvttsd2si rax, xmm1        ; rax = lower_index

    ; Check if we're at or beyond the last element
    ; Need to use n-1 as the maximum valid index
    mov rbx, rdx
    dec rbx                     ; rbx = n - 1
    cmp rax, rbx
    jge .at_last_element        ; if lower_index >= n-1, return last element

    ; Load adjacent values for interpolation
    vmovsd xmm4, [rcx + rax*8]       ; xmm4 = lower_value
    inc rax
    vmovsd xmm5, [rcx + rax*8]       ; xmm5 = upper_value

    ; Linear interpolation: result = lower + frac * (upper - lower)
    vsubsd xmm6, xmm5, xmm4          ; xmm6 = upper - lower
    vmulsd xmm6, xmm3, xmm6          ; xmm6 = frac * (upper - lower)
    vaddsd xmm0, xmm4, xmm6          ; xmm0 = lower + frac * (upper - lower)

    mov rsp, rbp
    pop rbp
    ret

.at_last_element:
    ; At last element, return it directly
    ; rbx = n - 1 (already calculated above)
    vmovsd xmm0, [rcx + rbx*8]
    mov rsp, rbp
    pop rbp
    ret

.single_element:
    ; Only one element, return it
    vmovsd xmm0, [rcx]
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
; fp_percentiles_sorted_f64
;
; Calculate multiple percentiles at once (batch operation)
; More efficient than calling fp_percentile_sorted_f64 repeatedly
;
; NOTE: This is an internal function. User-facing API is in fp_percentile_wrappers.c
; which handles sorting automatically.
;
; Arguments:
;   RCX = const double* sorted_data (input array, MUST BE SORTED!)
;   RDX = size_t n
;   R8  = const double* p_values (array of percentiles to calculate)
;   R9  = size_t n_percentiles
;   [RBP+48] = double* results (output array)
;
; Returns: void (results in output array)
; ============================================================================
global fp_percentiles_sorted_f64
fp_percentiles_sorted_f64:
    push rbp
    mov rbp, rsp
    sub rsp, 64

    ; Save non-volatile registers
    push rbx
    push r12
    push r13
    push r14
    push r15

    ; Save arguments
    mov [rbp - 8], rcx          ; sorted_data
    mov [rbp - 16], rdx         ; n
    mov [rbp - 24], r8          ; p_values
    mov [rbp - 32], r9          ; n_percentiles
    mov r10, [rbp + 48]         ; results pointer
    mov [rbp - 40], r10

    ; Handle empty array
    test rdx, rdx
    jz .empty_array

    ; Loop through each percentile
    xor r14, r14                ; r14 = percentile index

.percentile_loop:
    cmp r14, r9
    jge .done

    ; Load p value
    mov r8, [rbp - 24]
    vmovsd xmm2, [r8 + r14*8]

    ; Call fp_percentile_sorted_f64
    mov rcx, [rbp - 8]          ; sorted_data
    mov rdx, [rbp - 16]         ; n
    call fp_percentile_sorted_f64

    ; Store result
    mov r10, [rbp - 40]
    vmovsd [r10 + r14*8], xmm0

    inc r14
    jmp .percentile_loop

.done:
    ; Restore and return
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    mov rsp, rbp
    pop rbp
    ret

.empty_array:
    ; Fill results with NaN
    vmovsd xmm0, [rel const_nan]
    xor r14, r14

.fill_nan_loop:
    cmp r14, r9
    jge .done
    mov r10, [rbp + 48]
    vmovsd [r10 + r14*8], xmm0
    inc r14
    jmp .fill_nan_loop

; ============================================================================
; fp_quartiles_sorted_f64
;
; Calculate quartiles (Q1, median, Q3) and IQR in one call
;
; NOTE: This is an internal function. User-facing API is in fp_percentile_wrappers.c
; which handles sorting automatically.
;
; Arguments:
;   RCX = const double* sorted_data (input array, MUST BE SORTED!)
;   RDX = size_t n
;   R8  = Quartiles* output (struct with q1, median, q3, iqr)
;
; Returns: void (results in output struct)
; ============================================================================
global fp_quartiles_sorted_f64
fp_quartiles_sorted_f64:
    push rbp
    mov rbp, rsp
    sub rsp, 96

    ; Save non-volatile registers
    push rbx
    push r12
    push r13

    ; Save arguments
    mov [rbp - 8], rcx          ; sorted_data
    mov [rbp - 16], rdx         ; n
    mov [rbp - 24], r8          ; output

    ; Handle empty array
    test rdx, rdx
    jz .empty_array

    ; Calculate Q1 (p = 0.25)
    vmovsd xmm2, [rel const_025]
    call fp_percentile_sorted_f64
    vmovsd [rbp - 32], xmm0     ; Save Q1

    ; Calculate median (p = 0.50)
    mov rcx, [rbp - 8]
    mov rdx, [rbp - 16]
    vmovsd xmm2, [rel const_050]
    call fp_percentile_sorted_f64
    vmovsd [rbp - 40], xmm0     ; Save median

    ; Calculate Q3 (p = 0.75)
    mov rcx, [rbp - 8]
    mov rdx, [rbp - 16]
    vmovsd xmm2, [rel const_075]
    call fp_percentile_sorted_f64
    vmovsd [rbp - 48], xmm0     ; Save Q3

    ; Calculate IQR = Q3 - Q1
    vmovsd xmm1, [rbp - 32]     ; Q1
    vsubsd xmm2, xmm0, xmm1     ; Q3 - Q1 = IQR

    ; Store results in output struct
    mov r8, [rbp - 24]
    vmovsd xmm0, [rbp - 32]
    vmovsd [r8], xmm0           ; q1
    vmovsd xmm0, [rbp - 40]
    vmovsd [r8 + 8], xmm0       ; median
    vmovsd xmm0, [rbp - 48]
    vmovsd [r8 + 16], xmm0      ; q3
    vmovsd [r8 + 24], xmm2      ; iqr

    ; Restore and return
    pop r13
    pop r12
    pop rbx
    mov rsp, rbp
    pop rbp
    ret

.empty_array:
    ; Fill with NaN
    mov r8, [rbp - 24]
    vmovsd xmm0, [rel const_nan]
    vmovsd [r8], xmm0
    vmovsd [r8 + 8], xmm0
    vmovsd [r8 + 16], xmm0
    vmovsd [r8 + 24], xmm0

    pop r13
    pop r12
    pop rbx
    mov rsp, rbp
    pop rbp
    ret

section .data
align 8
const_025:  dq 0.25
const_050:  dq 0.50
const_075:  dq 0.75
const_nan:  dq 0x7FF8000000000000  ; NaN
