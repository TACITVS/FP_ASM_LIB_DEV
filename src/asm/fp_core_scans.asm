; =============================================================================
; FP-ASM Core Library: Scans Module (Module 5)
;
; Implements inclusive prefix sum (scan) operations.
; Strategy: Scalar implementation with loop unrolling for performance.
;
; Scan is inherently sequential - parallel SIMD algorithms like Blelloch/
; Hillis-Steele are future optimizations.
; =============================================================================

bits 64
default rel

section .text
    global fp_scan_add_i64
    global fp_scan_add_f64

; ===========================================================================
; fp_scan_add_i64: Inclusive prefix sum for int64_t arrays
;
; Signature: void fp_scan_add_i64(const int64_t* in, int64_t* out, size_t n)
;
; Arguments:
;   RCX = in  (const int64_t*)
;   RDX = out (int64_t*)
;   R8  = n   (size_t)
;
; Returns: void
;
; Example: [1, 2, 3, 4] -> [1, 3, 6, 10]
; ===========================================================================
fp_scan_add_i64:
    ; Prologue - save non-volatile registers
    push r12
    push r13
    sub  rsp, 32             ; Shadow space

    ; Initialize
    mov  r12, rcx            ; r12 = in
    mov  r13, rdx            ; r13 = out
    mov  rcx, r8             ; rcx = n (loop counter)

    test rcx, rcx
    jz   .done_scan_i64

    ; Initialize accumulator to 0
    xor  rax, rax            ; rax = accumulator

    ; Main loop: Process 4 elements at a time
.loop4_scan_i64:
    cmp  rcx, 4
    jb   .tail_scan_i64

    ; Load 4 elements and compute running sum
    mov  r8, [r12]           ; r8 = in[i]
    add  rax, r8             ; acc += in[i]
    mov  [r13], rax          ; out[i] = acc

    mov  r9, [r12+8]         ; r9 = in[i+1]
    add  rax, r9             ; acc += in[i+1]
    mov  [r13+8], rax        ; out[i+1] = acc

    mov  r10, [r12+16]       ; r10 = in[i+2]
    add  rax, r10            ; acc += in[i+2]
    mov  [r13+16], rax       ; out[i+2] = acc

    mov  r11, [r12+24]       ; r11 = in[i+3]
    add  rax, r11            ; acc += in[i+3]
    mov  [r13+24], rax       ; out[i+3] = acc

    add  r12, 32             ; in += 4
    add  r13, 32             ; out += 4
    sub  rcx, 4              ; n -= 4
    jmp  .loop4_scan_i64

    ; Tail: Process remaining 1-3 elements
.tail_scan_i64:
    test rcx, rcx
    jz   .done_scan_i64

.tail_loop_scan_i64:
    mov  r8, [r12]           ; r8 = in[i]
    add  rax, r8             ; acc += in[i]
    mov  [r13], rax          ; out[i] = acc
    add  r12, 8              ; in++
    add  r13, 8              ; out++
    dec  rcx                 ; n--
    jnz  .tail_loop_scan_i64

.done_scan_i64:
    ; Epilogue
    add  rsp, 32
    pop  r13
    pop  r12
    ret

; ===========================================================================
; fp_scan_add_f64: Inclusive prefix sum for double arrays
;
; Signature: void fp_scan_add_f64(const double* in, double* out, size_t n)
;
; Arguments:
;   RCX = in  (const double*)
;   RDX = out (double*)
;   R8  = n   (size_t)
;
; Returns: void
;
; Example: [1.0, 2.0, 3.0, 4.0] -> [1.0, 3.0, 6.0, 10.0]
; ===========================================================================
fp_scan_add_f64:
    ; Prologue - save non-volatile registers
    push r12
    push r13
    sub  rsp, 32             ; Shadow space

    ; Initialize
    mov  r12, rcx            ; r12 = in
    mov  r13, rdx            ; r13 = out
    mov  rcx, r8             ; rcx = n (loop counter)

    test rcx, rcx
    jz   .done_scan_f64

    ; Initialize accumulator to 0.0
    vxorpd xmm0, xmm0, xmm0  ; xmm0 = accumulator (0.0)

    ; Main loop: Process 4 elements at a time
.loop4_scan_f64:
    cmp  rcx, 4
    jb   .tail_scan_f64

    ; Load and accumulate element 0
    vmovsd xmm1, [r12]       ; xmm1 = in[i]
    vaddsd xmm0, xmm0, xmm1  ; acc += in[i]
    vmovsd [r13], xmm0       ; out[i] = acc

    ; Load and accumulate element 1
    vmovsd xmm2, [r12+8]     ; xmm2 = in[i+1]
    vaddsd xmm0, xmm0, xmm2  ; acc += in[i+1]
    vmovsd [r13+8], xmm0     ; out[i+1] = acc

    ; Load and accumulate element 2
    vmovsd xmm3, [r12+16]    ; xmm3 = in[i+2]
    vaddsd xmm0, xmm0, xmm3  ; acc += in[i+2]
    vmovsd [r13+16], xmm0    ; out[i+2] = acc

    ; Load and accumulate element 3
    vmovsd xmm4, [r12+24]    ; xmm4 = in[i+3]
    vaddsd xmm0, xmm0, xmm4  ; acc += in[i+3]
    vmovsd [r13+24], xmm0    ; out[i+3] = acc

    add  r12, 32             ; in += 4
    add  r13, 32             ; out += 4
    sub  rcx, 4              ; n -= 4
    jmp  .loop4_scan_f64

    ; Tail: Process remaining 1-3 elements
.tail_scan_f64:
    test rcx, rcx
    jz   .done_scan_f64

.tail_loop_scan_f64:
    vmovsd xmm1, [r12]       ; xmm1 = in[i]
    vaddsd xmm0, xmm0, xmm1  ; acc += in[i]
    vmovsd [r13], xmm0       ; out[i] = acc
    add  r12, 8              ; in++
    add  r13, 8              ; out++
    dec  rcx                 ; n--
    jnz  .tail_loop_scan_f64

.done_scan_f64:
    ; Epilogue
    add  rsp, 32
    pop  r13
    pop  r12
    ret
