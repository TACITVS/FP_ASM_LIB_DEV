; fp_core_moving_averages.asm
;
; Moving Averages - Algorithm #6 (Financial Computing)
; Compute SMA, EMA, and WMA for time series data
; Critical for technical analysis and trend detection
;
; Windows x64 calling convention

section .text

; ============================================================================
; fp_sma_f64
;
; Simple Moving Average - unweighted sliding window
; SMA[i] = (data[i] + data[i-1] + ... + data[i-window+1]) / window
;
; Arguments:
;   RCX = const double* data (time series)
;   RDX = size_t n (number of data points)
;   R8  = size_t window (window size, must be > 0 and <= n)
;   R9  = double* output (result array, size = n - window + 1)
;
; Returns: void (results in output array)
; ============================================================================
global fp_sma_f64
fp_sma_f64:
    push rbp
    mov rbp, rsp
    sub rsp, 64

    ; Validate inputs
    test rdx, rdx
    jz .empty_array

    test r8, r8
    jz .empty_array

    cmp r8, rdx
    ja .empty_array

    ; Save non-volatile registers
    push rbx
    push r12
    push r13
    push r14

    ; Save arguments
    mov [rbp - 8], rcx          ; data
    mov [rbp - 16], rdx         ; n
    mov [rbp - 24], r8          ; window
    mov [rbp - 32], r9          ; output

    ; Compute initial window sum
    vxorpd xmm0, xmm0, xmm0     ; sum
    xor rbx, rbx                ; index

.initial_sum:
    cmp rbx, r8
    jge .sliding_window

    vmovsd xmm1, [rcx + rbx*8]
    vaddsd xmm0, xmm0, xmm1
    inc rbx
    jmp .initial_sum

.sliding_window:
    ; xmm0 contains sum of first window
    ; Compute first SMA
    vcvtsi2sd xmm2, xmm2, r8    ; window as double
    vdivsd xmm3, xmm0, xmm2     ; first SMA
    vmovsd [r9], xmm3           ; store first result

    ; Sliding window: subtract oldest, add newest
    mov r12, rcx                ; data pointer
    mov r13, r9                 ; output pointer
    mov r14, 1                  ; output index
    mov rbx, r8                 ; data index (next new value)

    ; Number of additional windows
    mov rax, rdx
    sub rax, r8                 ; n - window
    test rax, rax
    jz .done

.slide_loop:
    ; Subtract data[i-window]
    mov r10, rbx
    sub r10, r8
    vmovsd xmm4, [r12 + r10*8]
    vsubsd xmm0, xmm0, xmm4

    ; Add data[i]
    vmovsd xmm5, [r12 + rbx*8]
    vaddsd xmm0, xmm0, xmm5

    ; Compute SMA
    vdivsd xmm3, xmm0, xmm2
    vmovsd [r13 + r14*8], xmm3

    inc rbx
    inc r14
    cmp r14, rax
    jbe .slide_loop

.done:
    pop r14
    pop r13
    pop r12
    pop rbx
    mov rsp, rbp
    pop rbp
    ret

.empty_array:
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_ema_f64
;
; Exponential Moving Average - exponentially weighted
; EMA[0] = data[0]
; EMA[i] = alpha * data[i] + (1-alpha) * EMA[i-1]
; where alpha = 2 / (window + 1)
;
; Arguments:
;   RCX = const double* data
;   RDX = size_t n
;   R8  = size_t window (used to calculate alpha)
;   R9  = double* output (size = n)
;
; Returns: void
; ============================================================================
global fp_ema_f64
fp_ema_f64:
    push rbp
    mov rbp, rsp
    sub rsp, 32

    ; Validate
    test rdx, rdx
    jz .empty_ema

    test r8, r8
    jz .empty_ema

    push rbx
    push r12

    ; Calculate alpha = 2 / (window + 1)
    inc r8                      ; window + 1
    vcvtsi2sd xmm0, xmm0, r8    ; (window + 1) as double
    vmovsd xmm1, [rel const_two]
    vdivsd xmm2, xmm1, xmm0     ; alpha

    ; Calculate (1 - alpha)
    vmovsd xmm3, [rel const_one]
    vsubsd xmm4, xmm3, xmm2     ; (1-alpha)

    ; Initialize: EMA[0] = data[0]
    vmovsd xmm5, [rcx]
    vmovsd [r9], xmm5           ; output[0] = data[0]

    ; Process remaining values
    mov r12, 1                  ; index
    vmovsd xmm6, xmm5           ; previous EMA

.ema_loop:
    cmp r12, rdx
    jge .ema_done

    ; EMA[i] = alpha * data[i] + (1-alpha) * EMA[i-1]
    vmovsd xmm7, [rcx + r12*8]  ; data[i]
    vmulsd xmm8, xmm2, xmm7     ; alpha * data[i]
    vmulsd xmm9, xmm4, xmm6     ; (1-alpha) * EMA[i-1]
    vaddsd xmm6, xmm8, xmm9     ; new EMA

    vmovsd [r9 + r12*8], xmm6   ; store

    inc r12
    jmp .ema_loop

.ema_done:
    pop r12
    pop rbx
    mov rsp, rbp
    pop rbp
    ret

.empty_ema:
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_wma_f64
;
; Weighted Moving Average - linearly weighted
; WMA[i] = (window*data[i] + (window-1)*data[i-1] + ... + 1*data[i-window+1]) / sum_of_weights
; where sum_of_weights = window * (window + 1) / 2
;
; Arguments:
;   RCX = const double* data
;   RDX = size_t n
;   R8  = size_t window
;   R9  = double* output (size = n - window + 1)
;
; Returns: void
; ============================================================================
global fp_wma_f64
fp_wma_f64:
    push rbp
    mov rbp, rsp
    sub rsp, 96

    ; Validate
    test rdx, rdx
    jz .empty_wma

    test r8, r8
    jz .empty_wma

    cmp r8, rdx
    ja .empty_wma

    push rbx
    push r12
    push r13
    push r14
    push r15

    ; Save arguments
    mov [rbp - 8], rcx          ; data
    mov [rbp - 16], rdx         ; n
    mov [rbp - 24], r8          ; window
    mov [rbp - 32], r9          ; output

    ; Calculate sum of weights = window * (window + 1) / 2
    mov rax, r8
    inc rax                     ; window + 1
    imul rax, r8                ; window * (window + 1)
    shr rax, 1                  ; divide by 2
    vcvtsi2sd xmm15, xmm15, rax ; sum_of_weights

    ; Number of WMA values to compute
    mov r15, rdx
    sub r15, r8
    inc r15                     ; n - window + 1

    xor r14, r14                ; output index

.wma_outer_loop:
    cmp r14, r15
    jge .wma_done

    ; Compute WMA for window starting at position r14
    vxorpd xmm0, xmm0, xmm0     ; weighted sum
    mov rbx, 0                  ; offset in window
    mov r13, r8                 ; weight (starts at window, counts down)

.wma_inner_loop:
    cmp rbx, r8
    jge .wma_store

    ; Get data[r14 + rbx]
    mov r10, r14
    add r10, rbx
    vmovsd xmm1, [rcx + r10*8]

    ; Multiply by weight
    vcvtsi2sd xmm2, xmm2, r13
    vmulsd xmm3, xmm1, xmm2

    ; Add to sum
    vaddsd xmm0, xmm0, xmm3

    inc rbx
    dec r13
    jmp .wma_inner_loop

.wma_store:
    ; Divide by sum of weights
    vdivsd xmm0, xmm0, xmm15
    vmovsd [r9 + r14*8], xmm0

    inc r14
    jmp .wma_outer_loop

.wma_done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    mov rsp, rbp
    pop rbp
    ret

.empty_wma:
    mov rsp, rbp
    pop rbp
    ret

section .data
align 8
const_one:  dq 1.0
const_two:  dq 2.0
