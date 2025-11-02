; =============================================================================
; FP-ASM Core Library - Fused Maps (Module 3) - Windows x64 ABI Compliant
;
; *** v14-FINAL: Fixed axpy_i64 non-volatile register violation ***
;
; Critical Fix: fp_map_axpy_i64 now uses only R11 (volatile) instead of
; R12-R14 (non-volatile) to avoid corrupting caller's register state.
; All functions now use consistent frame pointer prologue/epilogue for:
; - Proper 16-byte stack alignment
; - Correct shadow space allocation
; - Windows x64 ABI compliance
; - Correct register preservation
; =============================================================================

bits 64
default rel

section .text
    global fp_map_axpy_f64
    global fp_map_axpy_i64
    global fp_map_scale_i64
    global fp_map_scale_f64
    global fp_map_offset_i64
    global fp_map_offset_f64
    global fp_zip_add_i64
    global fp_zip_add_f64

; ===========================================================================
; fp_map_axpy_f64: out[i] = c * x[i] + y[i]
; ===========================================================================
; Windows x64 ABI:
;   RCX = const double* x
;   RDX = const double* y
;   R8  = double* out
;   R9  = size_t n
;   [rbp+48] = double c (5th argument on stack)
; ===========================================================================
global fp_map_axpy_f64
fp_map_axpy_f64:
    push rbp
    mov rbp, rsp
    and rsp, -32            ; Align stack pointer down to 32-byte boundary
    sub rsp, 32             ; Allocate 32 bytes for YMM15 save
    vmovdqa [rsp], ymm15    ; Save full YMM15 to aligned location
    
    test r9, r9
    jz .done
    
    vbroadcastsd ymm0, qword [rbp+48] ; Load c using RBP relative offset
    mov rax, r9
    shr rax, 2
    jz .remainder
    
.loop4:
    vmovupd ymm1, [rcx]     ; x
    vmovupd ymm2, [rdx]     ; y
    vfmadd213pd ymm1, ymm0, ymm2 ; ymm1 = (c * x) + y
    vmovupd [r8], ymm1      ; Store to out (R8)
    add rcx, 32
    add rdx, 32
    add r8, 32              ; Advance out pointer
    dec rax
    jnz .loop4
    
.remainder:
    and r9, 3
    jz .cleanup
    
.loop1:
    vmovsd xmm1, [rcx]      ; x
    vmovsd xmm2, [rdx]      ; y
    vfmadd213sd xmm1, xmm0, xmm2 ; xmm1 = (c * x) + y
    vmovsd [r8], xmm1       ; Store to out (R8)
    add rcx, 8
    add rdx, 8
    add r8, 8               ; Advance out pointer
    dec r9
    jnz .loop1
    
.cleanup:
    vzeroupper
    
.done:
    vmovdqa ymm15, [rsp]    ; Restore YMM15
    mov rsp, rbp            ; Deallocate all stack space up to RBP
    pop rbp                 ; Restore original RBP
    ret

; ===========================================================================
; fp_map_axpy_i64: out[i] = c * x[i] + y[i]
; ===========================================================================
; Windows x64 ABI:
;   RCX = const int64_t* x
;   RDX = const int64_t* y
;   R8  = int64_t* out
;   R9  = size_t n
;   [rsp+72] = int64_t c (5th argument)
; ===========================================================================
global fp_map_axpy_i64
fp_map_axpy_i64:
    sub rsp, 32             ; Shadow space
    
    test r9, r9
    jz .done
    
    mov r10, [rsp+72]       ; Load c (32 shadow + 40 original offset)
    mov rax, r9
    shr rax, 2
    jz .remainder
    
.loop4:
    ; Use only R11 (volatile) to avoid saving non-volatile registers
    mov r11, [rcx]          ; x0
    imul r11, r10           ; c*x0
    add r11, [rdx]          ; + y0
    mov [r8], r11           ; store -> out (R8)
    
    mov r11, [rcx+8]        ; x1
    imul r11, r10           ; c*x1
    add r11, [rdx+8]        ; + y1
    mov [r8+8], r11         ; store -> out (R8)
    
    mov r11, [rcx+16]       ; x2
    imul r11, r10           ; c*x2
    add r11, [rdx+16]       ; + y2
    mov [r8+16], r11        ; store -> out (R8)
    
    mov r11, [rcx+24]       ; x3
    imul r11, r10           ; c*x3
    add r11, [rdx+24]       ; + y3
    mov [r8+24], r11        ; store -> out (R8)
    
    add rcx, 32
    add rdx, 32
    add r8, 32              ; Advance out pointer
    dec rax
    jnz .loop4
    
.remainder:
    and r9, 3
    jz .done
    
.loop1:
    mov r11, [rcx]          ; x
    imul r11, r10           ; c*x
    add r11, [rdx]          ; + y
    mov [r8], r11           ; store -> out (R8)
    add rcx, 8
    add rdx, 8
    add r8, 8               ; Advance out pointer
    dec r9
    jnz .loop1
    
.done:
    add rsp, 32
    ret

; ===========================================================================
; fp_map_scale_f64: out[i] = c * in[i]
; ===========================================================================
; Windows x64 ABI:
;   RCX = const double* in
;   RDX = double* out
;   R8  = size_t n
;   XMM3 = double c
; ===========================================================================
global fp_map_scale_f64
fp_map_scale_f64:
    push rbp
    mov rbp, rsp
    and rsp, -32            ; Align
    sub rsp, 32             ; Save space for YMM1
    vmovdqa [rsp], ymm1     ; Save YMM1
    
    test r8, r8
    jz .done
    
    vbroadcastsd ymm0, xmm3 ; c is in XMM3
    mov rax, r8
    shr rax, 2
    jz .remainder
    
.loop4:
    vmovupd ymm1, [rcx]     ; in
    vmulpd ymm1, ymm1, ymm0 ; temp = in * c
    vmovupd [rdx], ymm1     ; Store to out (RDX)
    add rcx, 32
    add rdx, 32             ; Advance out pointer
    dec rax
    jnz .loop4
    
.remainder:
    and r8, 3
    jz .cleanup
    
.loop1:
    vmovsd xmm1, [rcx]      ; in
    vmulsd xmm1, xmm1, xmm0 ; temp = in * c
    vmovsd [rdx], xmm1      ; Store to out (RDX)
    add rcx, 8
    add rdx, 8              ; Advance out pointer
    dec r8
    jnz .loop1
    
.cleanup:
    vzeroupper
    
.done:
    vmovdqa ymm1, [rsp]     ; Restore YMM1
    mov rsp, rbp
    pop rbp
    ret

; ===========================================================================
; fp_map_scale_i64: out[i] = c * in[i]
; ===========================================================================
; Windows x64 ABI:
;   RCX = const int64_t* in
;   RDX = int64_t* out
;   R8  = size_t n
;   R9  = int64_t c
; ===========================================================================
global fp_map_scale_i64
fp_map_scale_i64:
    sub rsp, 32
    
    test r8, r8
    jz .done
    
    mov rax, r8
    shr rax, 2
    jz .remainder
    
.loop4:
    mov r10, [rcx]          ; in0
    imul r10, r9            ; c (in R9) * in0
    mov [rdx], r10          ; store -> out (RDX)
    
    mov r10, [rcx+8]        ; in1
    imul r10, r9            ; c * in1
    mov [rdx+8], r10        ; store -> out (RDX)
    
    mov r10, [rcx+16]       ; in2
    imul r10, r9            ; c * in2
    mov [rdx+16], r10       ; store -> out (RDX)
    
    mov r10, [rcx+24]       ; in3
    imul r10, r9            ; c * in3
    mov [rdx+24], r10       ; store -> out (RDX)
    
    add rcx, 32
    add rdx, 32             ; Advance out pointer
    dec rax
    jnz .loop4
    
.remainder:
    and r8, 3
    jz .done
    
.loop1:
    mov r10, [rcx]          ; in
    imul r10, r9            ; c * in
    mov [rdx], r10          ; store -> out (RDX)
    add rcx, 8
    add rdx, 8              ; Advance out pointer
    dec r8
    jnz .loop1
    
.done:
    add rsp, 32
    ret

; ===========================================================================
; fp_map_offset_f64: out[i] = in[i] + c
; ===========================================================================
; Windows x64 ABI:
;   RCX = const double* in
;   RDX = double* out
;   R8  = size_t n
;   XMM3 = double c
; ===========================================================================
global fp_map_offset_f64
fp_map_offset_f64:
    push rbp                ; Save original RBP
    mov rbp, rsp            ; Establish frame pointer
    and rsp, -32            ; Align stack pointer down to 32-byte boundary
    
    test r8, r8             ; n (in R8)
    jz .done
    
    mov r10, r8             ; R10 = n (copy)
    vbroadcastsd ymm0, xmm3 ; Broadcast c to YMM0
    
    mov rax, r10            ; n -> rax (loop counter)
    shr rax, 2              ; n / 4
    jz .remainder
    
.loop4:
    vmovupd ymm1, [rcx]     ; Load 4 doubles from 'in' (RCX)
    vaddpd ymm1, ymm1, ymm0 ; temp = temp + c
    vmovupd [rdx], ymm1     ; Store 4 results to 'out' (RDX)
    add rcx, 32             ; Advance 'in'
    add rdx, 32             ; Advance 'out'
    dec rax
    jnz .loop4
    
.remainder:
    mov r9, r10             ; Use R9 for remainder count
    and r9, 3               ; n % 4
    jz .cleanup
    
.loop1:
    vmovsd xmm1, [rcx]      ; Load 1 double from 'in' (RCX)
    vaddsd xmm1, xmm1, xmm3 ; temp = temp + c (use original XMM3)
    vmovsd [rdx], xmm1      ; Store result to 'out' (RDX)
    add rcx, 8              ; Advance 'in'
    add rdx, 8              ; Advance 'out'
    dec r9                  ; Decrement remainder count
    jnz .loop1
    
.cleanup:
    vzeroupper
    
.done:
    mov rsp, rbp            ; Deallocate all stack space up to RBP
    pop rbp                 ; Restore original RBP
    ret

; ===========================================================================
; fp_map_offset_i64: out[i] = in[i] + c
; ===========================================================================
; Windows x64 ABI:
;   RCX = const int64_t* in
;   RDX = int64_t* out
;   R8  = size_t n
;   R9  = int64_t c
; ===========================================================================
global fp_map_offset_i64
fp_map_offset_i64:
    sub rsp, 32             ; Shadow space
    
    test r8, r8             ; n (in R8)
    jz .done
    
    mov r11, r8             ; R11 = n (copy)
    movq xmm0, r9           ; c (in R9) -> XMM0
    vpbroadcastq ymm0, xmm0 ; Broadcast c to YMM0
    
    mov rax, r11            ; n -> rax (loop counter)
    shr rax, 2              ; n / 4
    jz .remainder
    
.loop4:
    vmovdqu ymm1, [rcx]     ; Load 4 qwords from 'in' (RCX)
    vpaddq ymm1, ymm1, ymm0 ; temp = temp + c
    vmovdqu [rdx], ymm1     ; Store 4 results to 'out' (RDX)
    add rcx, 32             ; Advance 'in'
    add rdx, 32             ; Advance 'out'
    dec rax
    jnz .loop4
    
.remainder:
    mov rax, r11            ; Use rax for remainder count
    and rax, 3              ; n % 4
    jz .cleanup
    
.loop1:
    mov r10, [rcx]          ; Load 1 qword from 'in' (RCX)
    add r10, r9             ; Add scalar 'c' (from original R9)
    mov [rdx], r10          ; Store result to 'out' (RDX)
    add rcx, 8              ; Advance 'in'
    add rdx, 8              ; Advance 'out'
    dec rax                 ; Decrement remainder count
    jnz .loop1
    
.cleanup:
    vzeroupper
    
.done:
    add rsp, 32
    ret

; ===========================================================================
; fp_zip_add_f64: out[i] = a[i] + b[i]
; ===========================================================================
; Windows x64 ABI:
;   RCX = const double* a
;   RDX = const double* b
;   R8  = double* out
;   R9  = size_t n
; ===========================================================================
global fp_zip_add_f64
fp_zip_add_f64:
    push rbp                ; Save RBP (maintains 16-byte alignment)
    mov rbp, rsp            ; Establish frame pointer
    sub rsp, 32             ; Shadow space (maintains 16-byte alignment)
    
    test r9, r9
    jz .done
    
    mov rax, r9             ; n -> rax (loop counter)
    shr rax, 2
    jz .remainder
    
.loop4:
    vmovupd ymm0, [rcx]     ; a
    vmovupd ymm1, [rdx]     ; b
    vaddpd ymm0, ymm0, ymm1 ; a + b
    vmovupd [r8], ymm0      ; Store to out (R8)
    add rcx, 32
    add rdx, 32
    add r8, 32              ; Advance out pointer
    dec rax
    jnz .loop4
    
.remainder:
    and r9, 3               ; n % 4
    jz .cleanup
    
.loop1:
    vmovsd xmm0, [rcx]      ; a
    vmovsd xmm1, [rdx]      ; b
    vaddsd xmm0, xmm0, xmm1 ; a + b
    vmovsd [r8], xmm0       ; Store to out (R8)
    add rcx, 8
    add rdx, 8
    add r8, 8               ; Advance out pointer
    dec r9
    jnz .loop1
    
.cleanup:
    vzeroupper
    
.done:
    mov rsp, rbp            ; Restore stack
    pop rbp
    ret

; ===========================================================================
; fp_zip_add_i64: out[i] = a[i] + b[i]
; ===========================================================================
; Windows x64 ABI:
;   RCX = const int64_t* a
;   RDX = const int64_t* b
;   R8  = int64_t* out
;   R9  = size_t n
; ===========================================================================
global fp_zip_add_i64
fp_zip_add_i64:
    push rbp                ; Save RBP (maintains 16-byte alignment)
    mov rbp, rsp            ; Establish frame pointer
    sub rsp, 32             ; Shadow space (maintains 16-byte alignment)
    
    test r9, r9
    jz .done
    
    mov rax, r9             ; n -> rax (loop counter)
    shr rax, 2
    jz .remainder
    
.loop4:
    vmovdqu ymm0, [rcx]     ; a
    vmovdqu ymm1, [rdx]     ; b
    vpaddq ymm0, ymm0, ymm1 ; a + b
    vmovdqu [r8], ymm0      ; Store to out (R8)
    add rcx, 32
    add rdx, 32
    add r8, 32              ; Advance out pointer
    dec rax
    jnz .loop4
    
.remainder:
    and r9, 3               ; n % 4
    jz .cleanup
    
.loop1:
    mov r10, [rcx]          ; a
    add r10, [rdx]          ; a + b
    mov [r8], r10           ; Store to out (R8)
    add rcx, 8
    add rdx, 8
    add r8, 8               ; Advance out pointer
    dec r9
    jnz .loop1
    
.cleanup:
    vzeroupper
    
.done:
    mov rsp, rbp            ; Restore stack
    pop rbp
    ret