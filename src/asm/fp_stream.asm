; =============================================================================
; fp_stream.asm — non-temporal (streaming) copy for GPU-mapped memory
;
;   void fp_stream_copy(void* dst, const void* src, size_t bytes)
;
; Copies `bytes` from src to dst using non-temporal stores (vmovntps), which
; bypass the CPU caches. This is the right way to fill memory returned by
; ID3D11DeviceContext::Map(WRITE_DISCARD), glMapBufferRange or vkMapMemory:
; such memory is usually write-combined (uncached), where ordinary stores
; that do not fill whole lines, and any read, are very slow.
;
; Typical use: compute a batch in a cache-resident scratch array with the
; regular kernels, then fp_stream_copy it into the mapped buffer.
;
; Contract:
;   - dst is only written, front to back; it is never read.
;   - any alignment of dst/src; dst is brought to 32-byte alignment with a
;     short byte loop, then streamed in 128- and 32-byte blocks.
;   - ends with sfence, so the data is globally visible before the caller
;     unmaps / submits.
;   - dst and src must not overlap.
;   - AVX only; uses volatile registers only (rax, rcx, rdx, r8, r9, ymm0-3),
;     so nothing needs saving on SysV or Win64.
; =============================================================================

bits 64
default rel

%include "macros.inc"

section .text

global fp_stream_copy
fp_stream_copy:
    ABI_ARGS_INT                    ; rcx = dst, rdx = src, r8 = bytes
    test    r8, r8
    jz      .done

    ; head: bytes until dst is 32-byte aligned (at most 31, at most n)
    mov     rax, rcx
    neg     rax
    and     rax, 31
    cmp     rax, r8
    cmova   rax, r8
    sub     r8, rax
.head:
    test    rax, rax
    jz      .body
    mov     r9b, [rdx]
    mov     [rcx], r9b
    inc     rcx
    inc     rdx
    dec     rax
    jmp     .head

.body:
    mov     rax, r8
    shr     rax, 7                  ; 128-byte blocks
    jz      .v32
.l128:
    vmovups ymm0, [rdx]
    vmovups ymm1, [rdx + 32]
    vmovups ymm2, [rdx + 64]
    vmovups ymm3, [rdx + 96]
    vmovntps [rcx],      ymm0
    vmovntps [rcx + 32], ymm1
    vmovntps [rcx + 64], ymm2
    vmovntps [rcx + 96], ymm3
    add     rdx, 128
    add     rcx, 128
    dec     rax
    jnz     .l128
    and     r8, 127
.v32:
    cmp     r8, 32
    jb      .tail
    vmovups ymm0, [rdx]
    vmovntps [rcx], ymm0
    add     rdx, 32
    add     rcx, 32
    sub     r8, 32
    jmp     .v32

.tail:
    test    r8, r8
    jz      .fence
.tail_loop:
    mov     r9b, [rdx]
    mov     [rcx], r9b
    inc     rcx
    inc     rdx
    dec     r8
    jnz     .tail_loop
.fence:
    sfence
    vzeroupper
.done:
    ret
