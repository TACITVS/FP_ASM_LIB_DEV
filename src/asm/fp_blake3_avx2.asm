; =============================================================================
; FP_BLAKE3_AVX2 - AVX2-Accelerated BLAKE3 Primitives
;
; PURITY GUARANTEE: All functions follow FP_ASM_LIB conventions:
;   - Input pointers are NEVER modified (const semantics)
;   - Output written to separate destination
;   - No hidden state, no globals modified
;   - Deterministic: same input → same output
;
; Performance target: 3-5x faster than SHA-256
; =============================================================================

section .data
    align 32

; BLAKE3 IV (first 8 words of fractional part of sqrt(2..9))
BLAKE3_IV:
    dd 0x6A09E667, 0xBB67AE85, 0x3C6EF372, 0xA54FF53A
    dd 0x510E527F, 0x9B05688C, 0x1F83D9AB, 0x5BE0CD19

; Rotation constants for AVX2 (used with vpsrld/vpslld pairs)
; BLAKE3 rotations: 16, 12, 8, 7
ROT16_SHUFFLE:
    db 2, 3, 0, 1, 6, 7, 4, 5, 10, 11, 8, 9, 14, 15, 12, 13
    db 2, 3, 0, 1, 6, 7, 4, 5, 10, 11, 8, 9, 14, 15, 12, 13

ROT8_SHUFFLE:
    db 1, 2, 3, 0, 5, 6, 7, 4, 9, 10, 11, 8, 13, 14, 15, 12
    db 1, 2, 3, 0, 5, 6, 7, 4, 9, 10, 11, 8, 13, 14, 15, 12

section .text

extern fp_blake3_compress
extern fp_blake3_round
extern fp_blake3_hash


; =============================================================================
; fp_blake3_g_avx2 - Vectorized G mixing function (8 parallel)
;
; Processes 8 independent G functions simultaneously using AVX2.
; Each G function mixes 4 words with 2 message words.
;
; C signature:
;   void fp_blake3_g_avx2(
;       const uint32_t* a_in,    // [8] input a values (rcx)
;       const uint32_t* b_in,    // [8] input b values (rdx)
;       const uint32_t* c_in,    // [8] input c values (r8)
;       const uint32_t* d_in,    // [8] input d values (r9)
;       const uint32_t* mx,      // [8] message x values (stack+40)
;       const uint32_t* my,      // [8] message y values (stack+48)
;       uint32_t* a_out,         // [8] output a (stack+56)
;       uint32_t* b_out,         // [8] output b (stack+64)
;       uint32_t* c_out,         // [8] output c (stack+72)
;       uint32_t* d_out          // [8] output d (stack+80)
;   );
;
; PURITY: All inputs const, outputs to separate arrays
; =============================================================================
global fp_blake3_g_avx2
fp_blake3_g_avx2:
    ; Windows x64 ABI: rcx, rdx, r8, r9, then stack
    push rbx
    push rsi
    push rdi

    ; Load inputs into YMM registers
    vmovdqu ymm0, [rcx]         ; a
    vmovdqu ymm1, [rdx]         ; b
    vmovdqu ymm2, [r8]          ; c
    vmovdqu ymm3, [r9]          ; d

    ; Get stack parameters
    mov rax, [rsp + 32 + 24 + 40]   ; mx
    vmovdqu ymm4, [rax]
    mov rax, [rsp + 32 + 24 + 48]   ; my
    vmovdqu ymm5, [rax]

    ; Load shuffle masks for rotations
    lea rax, [rel ROT16_SHUFFLE]
    vmovdqa ymm14, [rax]
    lea rax, [rel ROT8_SHUFFLE]
    vmovdqa ymm15, [rax]

    ; === First half-round ===
    ; a = a + b + mx
    vpaddd ymm0, ymm0, ymm1
    vpaddd ymm0, ymm0, ymm4

    ; d = rotr(d ^ a, 16)
    vpxor ymm3, ymm3, ymm0
    vpshufb ymm3, ymm3, ymm14   ; 16-bit rotation via shuffle

    ; c = c + d
    vpaddd ymm2, ymm2, ymm3

    ; b = rotr(b ^ c, 12)
    vpxor ymm1, ymm1, ymm2
    vpsrld ymm6, ymm1, 12
    vpslld ymm1, ymm1, 20
    vpor ymm1, ymm1, ymm6

    ; === Second half-round ===
    ; a = a + b + my
    vpaddd ymm0, ymm0, ymm1
    vpaddd ymm0, ymm0, ymm5

    ; d = rotr(d ^ a, 8)
    vpxor ymm3, ymm3, ymm0
    vpshufb ymm3, ymm3, ymm15   ; 8-bit rotation via shuffle

    ; c = c + d
    vpaddd ymm2, ymm2, ymm3

    ; b = rotr(b ^ c, 7)
    vpxor ymm1, ymm1, ymm2
    vpsrld ymm6, ymm1, 7
    vpslld ymm1, ymm1, 25
    vpor ymm1, ymm1, ymm6

    ; === Store outputs ===
    mov rax, [rsp + 32 + 24 + 56]   ; a_out
    vmovdqu [rax], ymm0
    mov rax, [rsp + 32 + 24 + 64]   ; b_out
    vmovdqu [rax], ymm1
    mov rax, [rsp + 32 + 24 + 72]   ; c_out
    vmovdqu [rax], ymm2
    mov rax, [rsp + 32 + 24 + 80]   ; d_out
    vmovdqu [rax], ymm3

    vzeroupper
    pop rdi
    pop rsi
    pop rbx
    ret

; =============================================================================
; fp_blake3_compress_avx2 - Single block compression (optimized)
;
; Compresses one 64-byte block into 32-byte chaining value.
; Delegates to fp_blake3_compress for correctness.
;
; C signature:
;   void fp_blake3_compress_avx2(
;       const uint32_t* cv_in,    // [8] input chaining value (rcx)
;       const uint8_t* block,     // [64] input block (rdx)
;       uint8_t block_len,        // actual data length (r8)
;       uint64_t counter,         // block counter (r9)
;       uint8_t flags,            // domain flags (stack+40)
;       uint32_t* cv_out          // [8] output chaining value (stack+48)
;   );
;
; PURITY: cv_in and block are NEVER modified
; =============================================================================
global fp_blake3_compress_avx2
fp_blake3_compress_avx2:
    sub rsp, 40
    mov r11, [rsp + 88]        ; cv_out
    movzx r10d, byte [rsp + 80] ; flags
    movdqu xmm0, [rcx]
    movdqu xmm1, [rcx+16]
    movdqu [r11], xmm0
    movdqu [r11+16], xmm1
    mov rcx, r11
    mov [rsp + 32], r10
    call fp_blake3_compress
    add rsp, 40
    ret
; Wrapper around fp_blake3_round (keeps output separate)
global fp_blake3_round_avx2
fp_blake3_round_avx2:
    sub rsp, 40
    movdqu xmm0, [rcx]
    movdqu xmm1, [rcx+16]
    movdqu xmm2, [rcx+32]
    movdqu xmm3, [rcx+48]
    movdqu [r8], xmm0
    movdqu [r8+16], xmm1
    movdqu [r8+32], xmm2
    movdqu [r8+48], xmm3
    mov rcx, r8
    call fp_blake3_round
    add rsp, 40
    ret
; Wrapper around fp_blake3_hash
global fp_blake3_hash_avx2
fp_blake3_hash_avx2:
    sub rsp, 40
    call fp_blake3_hash
    add rsp, 40
    ret
global fp_rotr32_avx2
fp_rotr32_avx2:
    vmovdqu ymm0, [rcx]         ; Load input

    ; Right rotation: (x >> n) | (x << (32-n))
    mov eax, 32
    sub eax, edx                ; 32 - n

    vmovd xmm2, edx             ; n
    vmovd xmm3, eax             ; 32 - n

    vpsrld ymm1, ymm0, xmm2     ; x >> n
    vpslld ymm0, ymm0, xmm3     ; x << (32-n)
    vpor ymm0, ymm0, ymm1       ; combine

    vmovdqu [r8], ymm0          ; Store output
    vzeroupper
    ret

; =============================================================================
; fp_xor8_u32_avx2 - XOR 8 x u32 arrays
;
; C signature:
;   void fp_xor8_u32_avx2(
;       const uint32_t* a,        // [8] first input (rcx)
;       const uint32_t* b,        // [8] second input (rdx)
;       uint32_t* out             // [8] output (r8)
;   );
; =============================================================================
global fp_xor8_u32_avx2
fp_xor8_u32_avx2:
    vmovdqu ymm0, [rcx]
    vmovdqu ymm1, [rdx]
    vpxor ymm0, ymm0, ymm1
    vmovdqu [r8], ymm0
    vzeroupper
    ret

; =============================================================================
; fp_add8_u32_avx2 - Add 8 x u32 arrays
;
; C signature:
;   void fp_add8_u32_avx2(
;       const uint32_t* a,        // [8] first input (rcx)
;       const uint32_t* b,        // [8] second input (rdx)
;       uint32_t* out             // [8] output (r8)
;   );
; =============================================================================
global fp_add8_u32_avx2
fp_add8_u32_avx2:
    vmovdqu ymm0, [rcx]
    vmovdqu ymm1, [rdx]
    vpaddd ymm0, ymm0, ymm1
    vmovdqu [r8], ymm0
    vzeroupper
    ret
