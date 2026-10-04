; =============================================================================
; fp_avxvnni_folds.asm — AVX-VNNI (VEX, 256-bit) integer dot products
;
; Target: Intel Alder Lake / Raptor Lake (12th-14th gen Core), Meteor Lake,
; Arrow Lake, Sierra Forest, Zen 5 ... — CPUs with AVX-VNNI but often WITHOUT
; AVX-512. Selected at runtime by src/runtime/fp_dispatch.c only when CPUID
; reports AVX-VNNI; the AVX2 versions remain the fallback.
;
; Functions (same contracts as the AVX2 versions in fp_core_fused_folds_*):
;   int8_t   fp_fold_dotp_i8_avxvnni (const int8_t*  a, const int8_t*  b, size_t n)
;   uint8_t  fp_fold_dotp_u8_avxvnni (const uint8_t* a, const uint8_t* b, size_t n)
;   int16_t  fp_fold_dotp_i16_avxvnni(const int16_t* a, const int16_t* b, size_t n)
;   uint16_t fp_fold_dotp_u16_avxvnni(const uint16_t*a, const uint16_t*b, size_t n)
;
; The results are the exact sums modulo 2^8 / 2^16 (wrapping, like the AVX2
; versions). That lets one instruction serve both signednesses:
;   vpdpbusd multiplies u8 x s8 and accumulates into i32. Modulo 256,
;   a_u * b_s == a * b for any reinterpretation of the bytes, so the low byte
;   of the i32 total is the wrapped dot product for both i8 and u8.
;   vpdpwssd (s16 x s16 -> i32) does the same for i16/u16 modulo 2^16.
; The i32 accumulators wrap (non-saturating forms), which keeps the low bits
; exact.
;
; Only volatile registers are used (rax, rcx, rdx, r8-r11, ymm0-ymm5), so no
; saves are needed on either ABI.
; =============================================================================

bits 64
default rel

%include "macros.inc"

; NASM < 2.16.03 cannot encode the VEX forms of VPDPBUSD/VPDPWSSD (it only
; knows the EVEX/AVX512-VNNI ones), so emit them by hand.
;   VEX.256.66.0F38.W0 50 /r  VPDPBUSD ymm1, ymm2, ymm3
;   VEX.256.66.0F38.W0 52 /r  VPDPWSSD ymm1, ymm2, ymm3
; VEX256_0F38_66 opcode, dst, src1, src2   (register numbers 0..15)
%macro VEX256_0F38_66 4
    db 0xC4
    db ((((%2) >> 3) ^ 1) << 7) | 0x40 | ((((%4) >> 3) ^ 1) << 5) | 0x02
    db ((15 - (%3)) << 3) | 0x05
    db %1
    db 0xC0 | (((%2) & 7) << 3) | ((%4) & 7)
%endmacro

section .text

; -----------------------------------------------------------------------------
; VNNI_DOT name, opcode(0x50|0x52), esize(1|2), size(byte|word), ext, reg
;   Main loop: 4 independent accumulators x 32 bytes; then 32-byte steps;
;   then a scalar tail (elements loaded with `ext`, movsx/movzx); the result
;   is narrowed with `ext eax, reg` (reg = al | ax).
; -----------------------------------------------------------------------------
%macro VNNI_DOT 6
global %1
%1:
    ABI_ARGS_INT                    ; rcx = a, rdx = b, r8 = n (elements)
    vpxor   ymm0, ymm0, ymm0
    vpxor   ymm1, ymm1, ymm1
    vpxor   ymm2, ymm2, ymm2
    vpxor   ymm3, ymm3, ymm3
    mov     r9, r8
    shr     r9, 7 - (%3 - 1)        ; blocks of 128 bytes
    jz      .x32
.l128:
    vmovdqu ymm4, [rcx]
    vmovdqu ymm5, [rdx]
    VEX256_0F38_66 %2, 0, 4, 5
    vmovdqu ymm4, [rcx + 32]
    vmovdqu ymm5, [rdx + 32]
    VEX256_0F38_66 %2, 1, 4, 5
    vmovdqu ymm4, [rcx + 64]
    vmovdqu ymm5, [rdx + 64]
    VEX256_0F38_66 %2, 2, 4, 5
    vmovdqu ymm4, [rcx + 96]
    vmovdqu ymm5, [rdx + 96]
    VEX256_0F38_66 %2, 3, 4, 5
    add     rcx, 128
    add     rdx, 128
    dec     r9
    jnz     .l128
    and     r8, (128 / %3) - 1
.x32:
    cmp     r8, 32 / %3
    jb      .reduce
    vmovdqu ymm4, [rcx]
    vmovdqu ymm5, [rdx]
    VEX256_0F38_66 %2, 0, 4, 5
    add     rcx, 32
    add     rdx, 32
    sub     r8, 32 / %3
    jmp     .x32
.reduce:
    vpaddd  ymm0, ymm0, ymm1
    vpaddd  ymm2, ymm2, ymm3
    vpaddd  ymm0, ymm0, ymm2
    HSUM_I32_YMM 0, 1
    vmovd   eax, xmm0
    test    r8, r8
    jz      .done
.tail:
    %5      r10d, %4 [rcx]
    %5      r11d, %4 [rdx]
    imul    r10d, r11d
    add     eax, r10d
    add     rcx, %3
    add     rdx, %3
    dec     r8
    jnz     .tail
.done:
    %5      eax, %6
    vzeroupper
    ret
%endmacro

VNNI_DOT fp_fold_dotp_i8_avxvnni,  0x50, 1, byte, movsx, al
VNNI_DOT fp_fold_dotp_u8_avxvnni,  0x50, 1, byte, movzx, al
VNNI_DOT fp_fold_dotp_i16_avxvnni, 0x52, 2, word, movsx, ax
VNNI_DOT fp_fold_dotp_u16_avxvnni, 0x52, 2, word, movzx, ax
