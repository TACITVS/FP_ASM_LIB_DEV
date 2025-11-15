; =============================================================================
; FP-ASM Library: Standardized Prologue/Epilogue Macros
; =============================================================================

; --- Helper Macros ---
%macro PROLOGUE 0
    push    rbp
    mov     rbp, rsp
    push    rbx                     ; Save additional non-volatile register used by callers
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
    pop     rbx
    pop     rbp
    vzeroupper                      ; Clear upper YMM lanes
    ret
%endmacro
