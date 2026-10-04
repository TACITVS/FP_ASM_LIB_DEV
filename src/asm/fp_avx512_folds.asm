; =============================================================================
; fp_avx512_folds.asm — AVX-512 (512-bit, EVEX) reductions and dot products
;
; Target: CPUs with AVX-512 (x86-64-v4): Skylake-SP/X, Ice/Tiger/Rocket Lake,
; Sapphire/Emerald/Granite Rapids, AMD Zen 4/Zen 5.
; NOTE: Alder Lake / Raptor Lake (12th-14th gen Core) do NOT have AVX-512 —
; it is fused off on the hybrid parts. They use the AVX2 / AVX-VNNI paths.
;
; Selected at runtime by src/runtime/fp_dispatch.c only when CPUID reports the
; required features AND the OS has enabled ZMM/opmask state (XCR0). Calling
; these directly on other CPUs raises SIGILL.
;
;   float    fp_reduce_add_f32_avx512 (const float*  in, size_t n)        AVX512F
;   double   fp_reduce_add_f64_avx512 (const double* in, size_t n)        AVX512F
;   float    fp_fold_sumsq_f32_avx512 (const float*  in, size_t n)        AVX512F
;   float    fp_fold_dotp_f32_avx512  (const float*  a, const float*  b, size_t n)
;   double   fp_fold_dotp_f64_avx512  (const double* a, const double* b, size_t n)
;   int8_t   fp_fold_dotp_i8_avx512   (...)   AVX512BW + AVX512_VNNI
;   uint8_t  fp_fold_dotp_u8_avx512   (...)   AVX512BW + AVX512_VNNI
;   int16_t  fp_fold_dotp_i16_avx512  (...)   AVX512BW + AVX512_VNNI
;   uint16_t fp_fold_dotp_u16_avx512  (...)   AVX512BW + AVX512_VNNI
; All of them also use BMI2 (bzhi) for the tail mask.
;
; Design:
;   * 8 independent accumulators (FP add/FMA latency 4 x 2 ports) for floats,
;     4 for the integer VNNI folds.
;   * No scalar tail: the last partial vector is a zero-masked load ({k}{z}),
;     which also suppresses faults past the end of the buffer.
;   * Float results differ from the AVX2 versions only by summation order
;     (rounding), never by more than normal reassociation error.
;   * Only volatile state is touched on both ABIs: rax, rcx, rdx, r8, r9,
;     k1, zmm0-zmm5 and zmm16-zmm31 (Win64's callee-saved xmm6-15 are unused).
; =============================================================================

bits 64
default rel

%include "macros.inc"

section .text

; The 8 float accumulators
%define ACC0 zmm0
%define ACC1 zmm1
%define ACC2 zmm2
%define ACC3 zmm3
%define ACC4 zmm16
%define ACC5 zmm17
%define ACC6 zmm18
%define ACC7 zmm19

; FSTEP kind, sfx, acc, off — one 64-byte step into `acc`
%macro FSTEP 4
  %ifidn %1, ADD
    vadd%2      %3, %3, [rcx + %4]
  %elifidn %1, SQ
    vmovu%2     zmm20, [rcx + %4]
    vfmadd231%2 %3, zmm20, zmm20
  %else ; DOT
    vmovu%2     zmm20, [rcx + %4]
    vfmadd231%2 %3, zmm20, [rdx + %4]
  %endif
%endmacro

; -----------------------------------------------------------------------------
; FOLD512 name, kind(ADD|SQ|DOT), sfx(ps|pd), lanes(16|8), log2(8*lanes)
; -----------------------------------------------------------------------------
%macro FOLD512 5
global %1
%1:
    ABI_ARGS_INT                    ; rcx = a, rdx = b | n, r8 = n
  %ifnidn %2, DOT
    mov     r8, rdx                 ; unary: n is the 2nd argument
  %endif
    vpxord  ACC0, ACC0, ACC0
    vpxord  ACC1, ACC1, ACC1
    vpxord  ACC2, ACC2, ACC2
    vpxord  ACC3, ACC3, ACC3
    vpxord  ACC4, ACC4, ACC4
    vpxord  ACC5, ACC5, ACC5
    vpxord  ACC6, ACC6, ACC6
    vpxord  ACC7, ACC7, ACC7

    mov     r9, r8
    shr     r9, %5                  ; blocks of 8 vectors (512 B)
    jz      .vec
.block:
    FSTEP %2, %3, ACC0, 0
    FSTEP %2, %3, ACC1, 64
    FSTEP %2, %3, ACC2, 128
    FSTEP %2, %3, ACC3, 192
    FSTEP %2, %3, ACC4, 256
    FSTEP %2, %3, ACC5, 320
    FSTEP %2, %3, ACC6, 384
    FSTEP %2, %3, ACC7, 448
    add     rcx, 512
  %ifidn %2, DOT
    add     rdx, 512
  %endif
    dec     r9
    jnz     .block
    and     r8, (8 * %4) - 1
.vec:                               ; whole vectors
    cmp     r8, %4
    jb      .tail
    FSTEP %2, %3, ACC0, 0
    add     rcx, 64
  %ifidn %2, DOT
    add     rdx, 64
  %endif
    sub     r8, %4
    jmp     .vec
.tail:                              ; 0 < r8 < lanes: masked, fault-free
    test    r8, r8
    jz      .reduce
    mov     eax, -1
    bzhi    eax, eax, r8d
    kmovw   k1, eax
  %ifidn %2, ADD
    vmovu%3 zmm20{k1}{z}, [rcx]
    vadd%3  ACC1, ACC1, zmm20
  %elifidn %2, SQ
    vmovu%3 zmm20{k1}{z}, [rcx]
    vfmadd231%3 ACC1, zmm20, zmm20
  %else
    vmovu%3 zmm20{k1}{z}, [rcx]
    vmovu%3 zmm21{k1}{z}, [rdx]
    vfmadd231%3 ACC1, zmm20, zmm21
  %endif
.reduce:
    vadd%3  ACC0, ACC0, ACC1
    vadd%3  ACC2, ACC2, ACC3
    vadd%3  ACC4, ACC4, ACC5
    vadd%3  ACC6, ACC6, ACC7
    vadd%3  ACC0, ACC0, ACC2
    vadd%3  ACC4, ACC4, ACC6
    vadd%3  ACC0, ACC0, ACC4
    vextractf64x4 ymm1, zmm0, 1
    vadd%3  ymm0, ymm0, ymm1
    vextractf128  xmm1, ymm0, 1
    vadd%3  xmm0, xmm0, xmm1
  %ifidn %3, ps
    vmovhlps xmm1, xmm1, xmm0
    vaddps  xmm0, xmm0, xmm1
    vmovshdup xmm1, xmm0
    vaddss  xmm0, xmm0, xmm1
  %else
    vunpckhpd xmm1, xmm0, xmm0
    vaddsd  xmm0, xmm0, xmm1
  %endif
    vzeroupper
    ret
%endmacro

FOLD512 fp_reduce_add_f32_avx512, ADD, ps, 16, 7
FOLD512 fp_reduce_add_f64_avx512, ADD, pd, 8, 6
FOLD512 fp_fold_sumsq_f32_avx512, SQ,  ps, 16, 7
FOLD512 fp_fold_dotp_f32_avx512,  DOT, ps, 16, 7
FOLD512 fp_fold_dotp_f64_avx512,  DOT, pd, 8, 6

; -----------------------------------------------------------------------------
; VNNI512 name, insn(vpdpbusd|vpdpwssd), esize(1|2), ext(movsx|movzx), reg(al|ax)
;   Same wrapping semantics as fp_avxvnni_folds.asm (see the note there).
; -----------------------------------------------------------------------------
%macro VNNI512 5
global %1
%1:
    ABI_ARGS_INT                    ; rcx = a, rdx = b, r8 = n (elements)
    vpxord  zmm0, zmm0, zmm0
    vpxord  zmm1, zmm1, zmm1
    vpxord  zmm2, zmm2, zmm2
    vpxord  zmm3, zmm3, zmm3
    mov     r9, r8
    shr     r9, 8 - (%3 - 1)        ; blocks of 256 bytes
    jz      .vec
.block:
    vmovdqu64 zmm4, [rcx]
    %2      zmm0, zmm4, [rdx]
    vmovdqu64 zmm5, [rcx + 64]
    %2      zmm1, zmm5, [rdx + 64]
    vmovdqu64 zmm4, [rcx + 128]
    %2      zmm2, zmm4, [rdx + 128]
    vmovdqu64 zmm5, [rcx + 192]
    %2      zmm3, zmm5, [rdx + 192]
    add     rcx, 256
    add     rdx, 256
    dec     r9
    jnz     .block
    and     r8, (256 / %3) - 1
.vec:
    cmp     r8, 64 / %3
    jb      .tail
    vmovdqu64 zmm4, [rcx]
    %2      zmm0, zmm4, [rdx]
    add     rcx, 64
    add     rdx, 64
    sub     r8, 64 / %3
    jmp     .vec
.tail:                              ; 0 < r8 < 64/esize: masked, fault-free
    test    r8, r8
    jz      .reduce
    mov     rax, -1
    bzhi    rax, rax, r8
  %if %3 == 1
    kmovq   k1, rax
    vmovdqu8  zmm4{k1}{z}, [rcx]
    vmovdqu8  zmm5{k1}{z}, [rdx]
  %else
    kmovd   k1, eax
    vmovdqu16 zmm4{k1}{z}, [rcx]
    vmovdqu16 zmm5{k1}{z}, [rdx]
  %endif
    %2      zmm1, zmm4, zmm5
.reduce:
    vpaddd  zmm0, zmm0, zmm1
    vpaddd  zmm2, zmm2, zmm3
    vpaddd  zmm0, zmm0, zmm2
    vextracti64x4 ymm1, zmm0, 1
    vpaddd  ymm0, ymm0, ymm1
    HSUM_I32_YMM 0, 1
    vmovd   eax, xmm0
    %4      eax, %5
    vzeroupper
    ret
%endmacro

VNNI512 fp_fold_dotp_i8_avx512,  vpdpbusd, 1, movsx, al
VNNI512 fp_fold_dotp_u8_avx512,  vpdpbusd, 1, movzx, al
VNNI512 fp_fold_dotp_i16_avx512, vpdpwssd, 2, movsx, ax
VNNI512 fp_fold_dotp_u16_avx512, vpdpwssd, 2, movzx, ax
