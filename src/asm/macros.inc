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

; --- Horizontal Reduction Macros ---

; Horizontal sum of 4x f32 in XMM register (result in xmm%1[0])
; Uses vhaddps for optimal performance
; Args: %1 = source/dest register
%macro HSUM_F32_XMM 1
    vhaddps xmm%1, xmm%1, xmm%1     ; [a+b, c+d, a+b, c+d]
    vhaddps xmm%1, xmm%1, xmm%1     ; [a+b+c+d, ...]
%endmacro

; Horizontal sum of 8x f32 in YMM register (result in xmm%1[0])
; Reduces to XMM then applies HSUM_F32_XMM
; Args: %1 = source/dest register, %2 = temp register
%macro HSUM_F32_YMM 2
    vextractf128 xmm%2, ymm%1, 1    ; Extract upper 128 bits
    vaddps xmm%1, xmm%1, xmm%2      ; Sum upper/lower halves
    HSUM_F32_XMM %1
%endmacro

; Horizontal sum of 4x i32 in XMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HSUM_I32_XMM 2
    vpshufd xmm%2, xmm%1, 0x4E      ; Shuffle [2,3,0,1]
    vpaddd xmm%1, xmm%1, xmm%2      ; Sum pairs
    vpshufd xmm%2, xmm%1, 0xB1      ; Shuffle [1,0,3,2]
    vpaddd xmm%1, xmm%1, xmm%2      ; Final sum
%endmacro

; Horizontal sum of 8x i32 in YMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HSUM_I32_YMM 2
    vextracti128 xmm%2, ymm%1, 1    ; Extract upper 128 bits
    vpaddd xmm%1, xmm%1, xmm%2      ; Sum upper/lower halves
    HSUM_I32_XMM %1, %2
%endmacro

; Horizontal product of 4x i32 in XMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HPROD_I32_XMM 2
    vpshufd xmm%2, xmm%1, 0x4E      ; Shuffle [2,3,0,1]
    vpmulld xmm%1, xmm%1, xmm%2     ; Multiply pairs
    vpshufd xmm%2, xmm%1, 0xB1      ; Shuffle [1,0,3,2]
    vpmulld xmm%1, xmm%1, xmm%2     ; Final multiply
%endmacro

; Horizontal product of 8x i32 in YMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HPROD_I32_YMM 2
    vextracti128 xmm%2, ymm%1, 1    ; Extract upper 128 bits
    vpmulld xmm%1, xmm%1, xmm%2     ; Multiply upper/lower halves
    HPROD_I32_XMM %1, %2
%endmacro

; Horizontal min of 4x i32 in XMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HMIN_I32_XMM 2
    vpshufd xmm%2, xmm%1, 0x4E      ; Shuffle [2,3,0,1]
    vpminsd xmm%1, xmm%1, xmm%2     ; Min of pairs
    vpshufd xmm%2, xmm%1, 0xB1      ; Shuffle [1,0,3,2]
    vpminsd xmm%1, xmm%1, xmm%2     ; Final min
%endmacro

; Horizontal min of 8x i32 in YMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HMIN_I32_YMM 2
    vextracti128 xmm%2, ymm%1, 1    ; Extract upper 128 bits
    vpminsd xmm%1, xmm%1, xmm%2     ; Min of upper/lower halves
    HMIN_I32_XMM %1, %2
%endmacro

; Horizontal max of 4x i32 in XMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HMAX_I32_XMM 2
    vpshufd xmm%2, xmm%1, 0x4E      ; Shuffle [2,3,0,1]
    vpmaxsd xmm%1, xmm%1, xmm%2     ; Max of pairs
    vpshufd xmm%2, xmm%1, 0xB1      ; Shuffle [1,0,3,2]
    vpmaxsd xmm%1, xmm%1, xmm%2     ; Final max
%endmacro

; Horizontal max of 8x i32 in YMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HMAX_I32_YMM 2
    vextracti128 xmm%2, ymm%1, 1    ; Extract upper 128 bits
    vpmaxsd xmm%1, xmm%1, xmm%2     ; Max of upper/lower halves
    HMAX_I32_XMM %1, %2
%endmacro

; Horizontal product of 4x f32 in XMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HPROD_F32_XMM 2
    vshufps xmm%2, xmm%1, xmm%1, 0x4E ; Shuffle [2,3,0,1]
    vmulps xmm%1, xmm%1, xmm%2         ; Multiply pairs
    vshufps xmm%2, xmm%1, xmm%1, 0xB1 ; Shuffle [1,0,3,2]
    vmulps xmm%1, xmm%1, xmm%2         ; Final multiply
%endmacro

; Horizontal product of 8x f32 in YMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HPROD_F32_YMM 2
    vextractf128 xmm%2, ymm%1, 1    ; Extract upper 128 bits
    vmulps xmm%1, xmm%1, xmm%2      ; Multiply upper/lower halves
    HPROD_F32_XMM %1, %2
%endmacro

; Horizontal min of 4x f32 in XMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HMIN_F32_XMM 2
    vshufps xmm%2, xmm%1, xmm%1, 0x4E ; Shuffle [2,3,0,1]
    vminps xmm%1, xmm%1, xmm%2         ; Min of pairs
    vshufps xmm%2, xmm%1, xmm%1, 0xB1 ; Shuffle [1,0,3,2]
    vminps xmm%1, xmm%1, xmm%2         ; Final min
%endmacro

; Horizontal min of 8x f32 in YMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HMIN_F32_YMM 2
    vextractf128 xmm%2, ymm%1, 1    ; Extract upper 128 bits
    vminps xmm%1, xmm%1, xmm%2      ; Min of upper/lower halves
    HMIN_F32_XMM %1, %2
%endmacro

; Horizontal max of 4x f32 in XMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HMAX_F32_XMM 2
    vshufps xmm%2, xmm%1, xmm%1, 0x4E ; Shuffle [2,3,0,1]
    vmaxps xmm%1, xmm%1, xmm%2         ; Max of pairs
    vshufps xmm%2, xmm%1, xmm%1, 0xB1 ; Shuffle [1,0,3,2]
    vmaxps xmm%1, xmm%1, xmm%2         ; Final max
%endmacro

; Horizontal max of 8x f32 in YMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HMAX_F32_YMM 2
    vextractf128 xmm%2, ymm%1, 1    ; Extract upper 128 bits
    vmaxps xmm%1, xmm%1, xmm%2      ; Max of upper/lower halves
    HMAX_F32_XMM %1, %2
%endmacro

; Horizontal sum of 4x u32 in XMM register (result in xmm%1[0])
; Note: Same as i32 (vpaddd works for both signed and unsigned)
; Args: %1 = source/dest register, %2 = temp register
%macro HSUM_U32_XMM 2
    vpshufd xmm%2, xmm%1, 0x4E      ; Shuffle [2,3,0,1]
    vpaddd xmm%1, xmm%1, xmm%2      ; Sum pairs
    vpshufd xmm%2, xmm%1, 0xB1      ; Shuffle [1,0,3,2]
    vpaddd xmm%1, xmm%1, xmm%2      ; Final sum
%endmacro

; Horizontal sum of 8x u32 in YMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HSUM_U32_YMM 2
    vextracti128 xmm%2, ymm%1, 1    ; Extract upper 128 bits
    vpaddd xmm%1, xmm%1, xmm%2      ; Sum upper/lower halves
    HSUM_U32_XMM %1, %2
%endmacro

; Horizontal product of 4x u32 in XMM register (result in xmm%1[0])
; Note: Same as i32 (vpmulld works for both signed and unsigned)
; Args: %1 = source/dest register, %2 = temp register
%macro HPROD_U32_XMM 2
    vpshufd xmm%2, xmm%1, 0x4E      ; Shuffle [2,3,0,1]
    vpmulld xmm%1, xmm%1, xmm%2     ; Multiply pairs
    vpshufd xmm%2, xmm%1, 0xB1      ; Shuffle [1,0,3,2]
    vpmulld xmm%1, xmm%1, xmm%2     ; Final multiply
%endmacro

; Horizontal product of 8x u32 in YMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HPROD_U32_YMM 2
    vextracti128 xmm%2, ymm%1, 1    ; Extract upper 128 bits
    vpmulld xmm%1, xmm%1, xmm%2     ; Multiply upper/lower halves
    HPROD_U32_XMM %1, %2
%endmacro

; Horizontal min of 4x u32 in XMM register (result in xmm%1[0])
; Note: Uses vpminud for UNSIGNED comparison (different from i32)
; Args: %1 = source/dest register, %2 = temp register
%macro HMIN_U32_XMM 2
    vpshufd xmm%2, xmm%1, 0x4E      ; Shuffle [2,3,0,1]
    vpminud xmm%1, xmm%1, xmm%2     ; Min of pairs (unsigned)
    vpshufd xmm%2, xmm%1, 0xB1      ; Shuffle [1,0,3,2]
    vpminud xmm%1, xmm%1, xmm%2     ; Final min
%endmacro

; Horizontal min of 8x u32 in YMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HMIN_U32_YMM 2
    vextracti128 xmm%2, ymm%1, 1    ; Extract upper 128 bits
    vpminud xmm%1, xmm%1, xmm%2     ; Min of upper/lower halves (unsigned)
    HMIN_U32_XMM %1, %2
%endmacro

; Horizontal max of 4x u32 in XMM register (result in xmm%1[0])
; Note: Uses vpmaxud for UNSIGNED comparison (different from i32)
; Args: %1 = source/dest register, %2 = temp register
%macro HMAX_U32_XMM 2
    vpshufd xmm%2, xmm%1, 0x4E      ; Shuffle [2,3,0,1]
    vpmaxud xmm%1, xmm%1, xmm%2     ; Max of pairs (unsigned)
    vpshufd xmm%2, xmm%1, 0xB1      ; Shuffle [1,0,3,2]
    vpmaxud xmm%1, xmm%1, xmm%2     ; Final max
%endmacro

; Horizontal max of 8x u32 in YMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HMAX_U32_YMM 2
    vextracti128 xmm%2, ymm%1, 1    ; Extract upper 128 bits
    vpmaxud xmm%1, xmm%1, xmm%2     ; Max of upper/lower halves (unsigned)
    HMAX_U32_XMM %1, %2
%endmacro

; ============================================================================
; i16 Horizontal Reduction Macros
; ============================================================================

; Horizontal sum of 8x i16 in XMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HSUM_I16_XMM 2
    vphaddw xmm%1, xmm%1, xmm%1     ; Horizontal add pairs
    vphaddw xmm%1, xmm%1, xmm%1     ; Horizontal add pairs again
    vphaddw xmm%1, xmm%1, xmm%1     ; Final horizontal add
%endmacro

; Horizontal sum of 16x i16 in YMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HSUM_I16_YMM 2
    vextracti128 xmm%2, ymm%1, 1    ; Extract upper 128 bits
    vpaddw xmm%1, xmm%1, xmm%2      ; Sum upper/lower halves
    HSUM_I16_XMM %1, %2
%endmacro

; Horizontal product of 8x i16 in XMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HPROD_I16_XMM 2
    vpshufd xmm%2, xmm%1, 0x4E      ; Shuffle [2,3,0,1]
    vpmullw xmm%1, xmm%1, xmm%2     ; Multiply pairs
    vpshufd xmm%2, xmm%1, 0xB1      ; Shuffle [1,0,3,2]
    vpmullw xmm%1, xmm%1, xmm%2
    vpsrldq xmm%2, xmm%1, 2         ; Shift by 1 word
    vpmullw xmm%1, xmm%1, xmm%2
%endmacro

; Horizontal product of 16x i16 in YMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HPROD_I16_YMM 2
    vextracti128 xmm%2, ymm%1, 1    ; Extract upper 128 bits
    vpmullw xmm%1, xmm%1, xmm%2     ; Multiply upper/lower halves
    HPROD_I16_XMM %1, %2
%endmacro

; Horizontal min of 8x i16 in XMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HMIN_I16_XMM 2
    vpshufd xmm%2, xmm%1, 0x4E      ; Shuffle [2,3,0,1]
    vpminsw xmm%1, xmm%1, xmm%2     ; Min of pairs (signed)
    vpshufd xmm%2, xmm%1, 0xB1      ; Shuffle [1,0,3,2]
    vpminsw xmm%1, xmm%1, xmm%2
    vpsrldq xmm%2, xmm%1, 2         ; Shift by 1 word
    vpminsw xmm%1, xmm%1, xmm%2
%endmacro

; Horizontal min of 16x i16 in YMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HMIN_I16_YMM 2
    vextracti128 xmm%2, ymm%1, 1    ; Extract upper 128 bits
    vpminsw xmm%1, xmm%1, xmm%2     ; Min of upper/lower halves (signed)
    HMIN_I16_XMM %1, %2
%endmacro

; Horizontal max of 8x i16 in XMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HMAX_I16_XMM 2
    vpshufd xmm%2, xmm%1, 0x4E      ; Shuffle [2,3,0,1]
    vpmaxsw xmm%1, xmm%1, xmm%2     ; Max of pairs (signed)
    vpshufd xmm%2, xmm%1, 0xB1      ; Shuffle [1,0,3,2]
    vpmaxsw xmm%1, xmm%1, xmm%2
    vpsrldq xmm%2, xmm%1, 2         ; Shift by 1 word
    vpmaxsw xmm%1, xmm%1, xmm%2
%endmacro

; Horizontal max of 16x i16 in YMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HMAX_I16_YMM 2
    vextracti128 xmm%2, ymm%1, 1    ; Extract upper 128 bits
    vpmaxsw xmm%1, xmm%1, xmm%2     ; Max of upper/lower halves (signed)
    HMAX_I16_XMM %1, %2
%endmacro

; ============================================================================
; u16 Horizontal Reduction Macros
; ============================================================================

; Horizontal sum of 8x u16 in XMM register (result in xmm%1[0])
; Note: Same as i16 (vpaddw works for both signed and unsigned)
; Args: %1 = source/dest register, %2 = temp register
%macro HSUM_U16_XMM 2
    vphaddw xmm%1, xmm%1, xmm%1     ; Horizontal add pairs
    vphaddw xmm%1, xmm%1, xmm%1     ; Horizontal add pairs again
    vphaddw xmm%1, xmm%1, xmm%1     ; Final horizontal add
%endmacro

; Horizontal sum of 16x u16 in YMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HSUM_U16_YMM 2
    vextracti128 xmm%2, ymm%1, 1    ; Extract upper 128 bits
    vpaddw xmm%1, xmm%1, xmm%2      ; Sum upper/lower halves
    HSUM_U16_XMM %1, %2
%endmacro

; Horizontal product of 8x u16 in XMM register (result in xmm%1[0])
; Note: Same as i16 (vpmullw works for both signed and unsigned)
; Args: %1 = source/dest register, %2 = temp register
%macro HPROD_U16_XMM 2
    vpshufd xmm%2, xmm%1, 0x4E      ; Shuffle [2,3,0,1]
    vpmullw xmm%1, xmm%1, xmm%2     ; Multiply pairs
    vpshufd xmm%2, xmm%1, 0xB1      ; Shuffle [1,0,3,2]
    vpmullw xmm%1, xmm%1, xmm%2
    vpsrldq xmm%2, xmm%1, 2         ; Shift by 1 word
    vpmullw xmm%1, xmm%1, xmm%2
%endmacro

; Horizontal product of 16x u16 in YMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HPROD_U16_YMM 2
    vextracti128 xmm%2, ymm%1, 1    ; Extract upper 128 bits
    vpmullw xmm%1, xmm%1, xmm%2     ; Multiply upper/lower halves
    HPROD_U16_XMM %1, %2
%endmacro

; Horizontal min of 8x u16 in XMM register (result in xmm%1[0])
; Note: Uses vpminuw for UNSIGNED comparison
; Args: %1 = source/dest register, %2 = temp register
%macro HMIN_U16_XMM 2
    vpshufd xmm%2, xmm%1, 0x4E      ; Shuffle [2,3,0,1]
    vpminuw xmm%1, xmm%1, xmm%2     ; Min of pairs (unsigned)
    vpshufd xmm%2, xmm%1, 0xB1      ; Shuffle [1,0,3,2]
    vpminuw xmm%1, xmm%1, xmm%2
    vpsrldq xmm%2, xmm%1, 2         ; Shift by 1 word
    vpminuw xmm%1, xmm%1, xmm%2
%endmacro

; Horizontal min of 16x u16 in YMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HMIN_U16_YMM 2
    vextracti128 xmm%2, ymm%1, 1    ; Extract upper 128 bits
    vpminuw xmm%1, xmm%1, xmm%2     ; Min of upper/lower halves (unsigned)
    HMIN_U16_XMM %1, %2
%endmacro

; Horizontal max of 8x u16 in XMM register (result in xmm%1[0])
; Note: Uses vpmaxuw for UNSIGNED comparison
; Args: %1 = source/dest register, %2 = temp register
%macro HMAX_U16_XMM 2
    vpshufd xmm%2, xmm%1, 0x4E      ; Shuffle [2,3,0,1]
    vpmaxuw xmm%1, xmm%1, xmm%2     ; Max of pairs (unsigned)
    vpshufd xmm%2, xmm%1, 0xB1      ; Shuffle [1,0,3,2]
    vpmaxuw xmm%1, xmm%1, xmm%2
    vpsrldq xmm%2, xmm%1, 2         ; Shift by 1 word
    vpmaxuw xmm%1, xmm%1, xmm%2
%endmacro

; Horizontal max of 16x u16 in YMM register (result in xmm%1[0])
; Args: %1 = source/dest register, %2 = temp register
%macro HMAX_U16_YMM 2
    vextracti128 xmm%2, ymm%1, 1    ; Extract upper 128 bits
    vpmaxuw xmm%1, xmm%1, xmm%2     ; Max of upper/lower halves (unsigned)
    HMAX_U16_XMM %1, %2
%endmacro
