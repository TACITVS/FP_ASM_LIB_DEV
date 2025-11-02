; fp_core_descriptive_stats.asm
;
; Descriptive Statistics Suite - Algorithm #1
; Calculates statistical moments (mean, variance, std dev, skewness, kurtosis)
; in a single pass using fused operations
;
; Windows x64 calling convention

section .text

; ============================================================================
; fp_moments_f64
;
; Calculate first 4 statistical moments in single pass
; Moments: sum, sum_sq, sum_cube, sum_quad
;
; Arguments:
;   RCX = const double* data (input array)
;   RDX = size_t n (number of elements)
;   R8  = double* moments (output: 4 doubles - m1, m2, m3, m4)
;
; Returns: void (results in moments array)
; ============================================================================
global fp_moments_f64
fp_moments_f64:
    push rbp
    mov rbp, rsp
    sub rsp, 32

    ; Handle edge cases
    test rdx, rdx
    jz .empty_array

    ; Save non-volatile registers
    push rbx
    push r12
    push r13
    push r14

    ; Initialize accumulators
    vxorpd ymm0, ymm0, ymm0    ; sum accumulator (4 doubles)
    vxorpd ymm1, ymm1, ymm1    ; sum_sq accumulator
    vxorpd ymm2, ymm2, ymm2    ; sum_cube accumulator
    vxorpd ymm3, ymm3, ymm3    ; sum_quad accumulator

    mov r12, rcx               ; r12 = data pointer
    mov r13, rdx               ; r13 = remaining count
    xor r14, r14               ; r14 = index

    ; Process 4 elements at a time
.loop4:
    cmp r13, 4
    jb .tail

    ; Load 4 doubles
    vmovupd ymm4, [r12 + r14*8]

    ; Accumulate sum (m1)
    vaddpd ymm0, ymm0, ymm4

    ; Calculate x^2
    vmulpd ymm5, ymm4, ymm4
    vaddpd ymm1, ymm1, ymm5

    ; Calculate x^3
    vmulpd ymm6, ymm5, ymm4
    vaddpd ymm2, ymm2, ymm6

    ; Calculate x^4
    vmulpd ymm7, ymm5, ymm5
    vaddpd ymm3, ymm3, ymm7

    add r14, 4
    sub r13, 4
    jmp .loop4

.tail:
    ; Horizontal reduction of YMM registers to scalars FIRST
    ; Extract upper 128 bits and add to lower
    vextractf128 xmm4, ymm0, 1
    vaddpd xmm0, xmm0, xmm4
    vhaddpd xmm0, xmm0, xmm0    ; sum in xmm0[0]

    vextractf128 xmm4, ymm1, 1
    vaddpd xmm1, xmm1, xmm4
    vhaddpd xmm1, xmm1, xmm1    ; sum_sq in xmm1[0]

    vextractf128 xmm4, ymm2, 1
    vaddpd xmm2, xmm2, xmm4
    vhaddpd xmm2, xmm2, xmm2    ; sum_cube in xmm2[0]

    vextractf128 xmm4, ymm3, 1
    vaddpd xmm3, xmm3, xmm4
    vhaddpd xmm3, xmm3, xmm3    ; sum_quad in xmm3[0]

    ; Process remaining elements one at a time (after reduction)
    test r13, r13
    jz .store_results

.tail_loop:
    vmovsd xmm4, [r12 + r14*8]

    ; sum
    vaddsd xmm0, xmm0, xmm4

    ; x^2
    vmulsd xmm5, xmm4, xmm4
    vaddsd xmm1, xmm1, xmm5

    ; x^3
    vmulsd xmm6, xmm5, xmm4
    vaddsd xmm2, xmm2, xmm6

    ; x^4
    vmulsd xmm7, xmm5, xmm5
    vaddsd xmm3, xmm3, xmm7

    inc r14
    dec r13
    jnz .tail_loop

.store_results:

    ; Store results
    vmovsd [r8], xmm0
    vmovsd [r8 + 8], xmm1
    vmovsd [r8 + 16], xmm2
    vmovsd [r8 + 24], xmm3

    ; Restore and return
    vzeroupper
    pop r14
    pop r13
    pop r12
    pop rbx
    mov rsp, rbp
    pop rbp
    ret

.empty_array:
    ; Return zeros for empty array
    vxorpd xmm0, xmm0, xmm0
    vmovsd [r8], xmm0
    vmovsd [r8 + 8], xmm0
    vmovsd [r8 + 16], xmm0
    vmovsd [r8 + 24], xmm0
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; fp_descriptive_stats_f64
;
; Calculate descriptive statistics from moments
; This is a C-callable wrapper that computes mean, variance, stddev,
; skewness, and kurtosis from the raw moments
;
; Arguments:
;   RCX = const double* data
;   RDX = size_t n
;   R8  = DescriptiveStats* stats (output struct)
;
; Returns: void
; ============================================================================
global fp_descriptive_stats_f64
fp_descriptive_stats_f64:
    push rbp
    mov rbp, rsp
    sub rsp, 96                 ; Space for moments + local vars
    and rsp, 0xFFFFFFFFFFFFFFE0

    ; Save arguments
    mov [rbp - 8], rcx         ; data
    mov [rbp - 16], rdx        ; n
    mov [rbp - 24], r8         ; stats

    ; Handle empty array
    test rdx, rdx
    jz .empty_case

    ; Calculate moments
    lea r8, [rsp + 32]         ; moments output
    call fp_moments_f64

    ; Load n as double
    mov rax, [rbp - 16]
    vcvtsi2sd xmm15, xmm15, rax  ; xmm15 = n (as double)

    ; Load moments
    vmovsd xmm0, [rsp + 32]    ; m1 = sum
    vmovsd xmm1, [rsp + 40]    ; m2 = sum_sq
    vmovsd xmm2, [rsp + 48]    ; m3 = sum_cube
    vmovsd xmm3, [rsp + 56]    ; m4 = sum_quad

    ; Calculate mean = m1 / n
    vdivsd xmm4, xmm0, xmm15   ; xmm4 = mean

    ; Calculate variance = (m2 / n) - mean^2
    vdivsd xmm5, xmm1, xmm15   ; xmm5 = m2/n
    vmulsd xmm6, xmm4, xmm4    ; xmm6 = mean^2
    vsubsd xmm5, xmm5, xmm6    ; xmm5 = variance

    ; Calculate std dev = sqrt(variance)
    vsqrtsd xmm6, xmm5, xmm5   ; xmm6 = std_dev

    ; Calculate skewness = [(m3/n) - 3*mean*(m2/n) + 2*mean^3] / stddev^3
    vdivsd xmm7, xmm2, xmm15   ; xmm7 = m3/n
    vdivsd xmm8, xmm1, xmm15   ; xmm8 = m2/n

    ; 3*mean*(m2/n)
    vmovsd xmm9, [rel const_3]
    vmulsd xmm9, xmm9, xmm4    ; 3*mean
    vmulsd xmm9, xmm9, xmm8    ; 3*mean*(m2/n)

    ; 2*mean^3
    vmulsd xmm10, xmm4, xmm4   ; mean^2
    vmulsd xmm10, xmm10, xmm4  ; mean^3
    vmovsd xmm11, [rel const_2]
    vmulsd xmm10, xmm10, xmm11 ; 2*mean^3

    ; Numerator: m3/n - 3*mean*(m2/n) + 2*mean^3
    vsubsd xmm7, xmm7, xmm9
    vaddsd xmm7, xmm7, xmm10

    ; Denominator: stddev^3
    vmulsd xmm11, xmm6, xmm6   ; stddev^2
    vmulsd xmm11, xmm11, xmm6  ; stddev^3

    ; Skewness
    vdivsd xmm7, xmm7, xmm11   ; xmm7 = skewness

    ; Calculate kurtosis = [(m4/n) - 4*mean*(m3/n) + 6*mean^2*(m2/n) - 3*mean^4] / variance^2
    vdivsd xmm8, xmm3, xmm15   ; xmm8 = m4/n
    vdivsd xmm9, xmm2, xmm15   ; xmm9 = m3/n
    vdivsd xmm10, xmm1, xmm15  ; xmm10 = m2/n

    ; 4*mean*(m3/n)
    vmovsd xmm11, [rel const_4]
    vmulsd xmm11, xmm11, xmm4  ; 4*mean
    vmulsd xmm11, xmm11, xmm9  ; 4*mean*(m3/n)

    ; 6*mean^2*(m2/n)
    vmulsd xmm12, xmm4, xmm4   ; mean^2
    vmovsd xmm13, [rel const_6]
    vmulsd xmm12, xmm12, xmm13 ; 6*mean^2
    vmulsd xmm12, xmm12, xmm10 ; 6*mean^2*(m2/n)

    ; 3*mean^4
    vmulsd xmm13, xmm4, xmm4   ; mean^2
    vmulsd xmm13, xmm13, xmm13 ; mean^4
    vmovsd xmm14, [rel const_3]
    vmulsd xmm13, xmm13, xmm14 ; 3*mean^4

    ; Numerator: m4/n - 4*mean*(m3/n) + 6*mean^2*(m2/n) - 3*mean^4
    vsubsd xmm8, xmm8, xmm11
    vaddsd xmm8, xmm8, xmm12
    vsubsd xmm8, xmm8, xmm13

    ; Denominator: variance^2
    vmulsd xmm11, xmm5, xmm5   ; variance^2

    ; Kurtosis
    vdivsd xmm8, xmm8, xmm11   ; xmm8 = kurtosis

    ; Store results in stats struct
    mov r8, [rbp - 24]
    vmovsd [r8], xmm4          ; mean
    vmovsd [r8 + 8], xmm5      ; variance
    vmovsd [r8 + 16], xmm6     ; std_dev
    vmovsd [r8 + 24], xmm7     ; skewness
    vmovsd [r8 + 32], xmm8     ; kurtosis

    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

.empty_case:
    ; Return NaN for empty array
    vmovsd xmm0, [rel const_nan]
    mov r8, [rbp - 24]
    vmovsd [r8], xmm0
    vmovsd [r8 + 8], xmm0
    vmovsd [r8 + 16], xmm0
    vmovsd [r8 + 24], xmm0
    vmovsd [r8 + 32], xmm0
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

section .data
align 8
const_2:    dq 2.0
const_3:    dq 3.0
const_4:    dq 4.0
const_6:    dq 6.0
const_nan:  dq 0x7FF8000000000000  ; NaN
