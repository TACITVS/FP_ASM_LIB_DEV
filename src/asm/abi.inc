; =============================================================================
; abi.inc — Cross-platform x64 calling-convention abstraction
;
; The function bodies in this library are written for the **Windows x64 ABI**
; (integer args in rcx, rdx, r8, r9; float args positional in xmm0..xmm3;
; 5th+ args on the stack). Upstream was Windows-only.
;
; To support the System V AMD64 ABI (Linux, macOS, BSD) WITHOUT rewriting every
; body, each function invokes one of the ABI_* shim macros at entry. On Windows
; the shims expand to NOTHING, so the original, verified Win64 behavior is byte-
; for-byte preserved. On System V the shims shuffle the incoming argument
; registers into the Win64 positions the body expects.
;
; System V integer args: rdi, rsi, rdx, rcx, r8, r9
; System V float   args: xmm0, xmm1, ... (sequential, float-only order)
; Windows   integer args: rcx, rdx, r8, r9
; Windows   float   args: xmm0..xmm3 (by argument position)
; =============================================================================

%ifndef FP_ABI_INC
%define FP_ABI_INC

; --- Platform detection (NASM predefines the output format) ---
%ifidn __?OUTPUT_FORMAT?__, win64
    %define FP_WIN64 1
%elifidn __?OUTPUT_FORMAT?__, win32
    %define FP_WIN64 1
%else
    %define FP_SYSV 1
%endif

; -----------------------------------------------------------------------------
; ABI_ARGS_INT — up to 4 integer/pointer args.
; Covers the majority of functions (reductions, folds, maps, zips, set-ops).
; Win64:  args already in rcx, rdx, r8, r9  -> no-op.
; SysV:   rearrange rdi, rsi, rdx, rcx -> rcx, rdx, r8, r9.
;         Order preserves sources before they are overwritten:
;           rcx is SysV a4 AND Win a1;  rdx is SysV a3 AND Win a2.
; NOTE: only valid for functions with <= 4 integer args and no register float
;       args. Functions with a 5th (stack) arg must capture it BEFORE this shim
;       (it clobbers r8/r9, which hold SysV args 5/6).
; -----------------------------------------------------------------------------
%macro ABI_ARGS_INT 0
    %ifdef FP_SYSV
        mov r9,  rcx        ; Win a4  <- SysV a4
        mov r8,  rdx        ; Win a3  <- SysV a3
        mov rdx, rsi        ; Win a2  <- SysV a2
        mov rcx, rdi        ; Win a1  <- SysV a1
    %endif
%endmacro

; -----------------------------------------------------------------------------
; ABI_ARGS_INT3_F1 — 3 integer args + 1 float arg in the 4th position.
; e.g. fp_map_scale_f64(in, out, n, double c) / fp_map_offset_f32(...).
; Win64:  ints rcx, rdx, r8 ; float in xmm3  -> no-op.
; SysV:   ints rdi, rsi, rdx ; float in xmm0 -> ints to rcx, rdx, r8; xmm0->xmm3.
; -----------------------------------------------------------------------------
%macro ABI_ARGS_INT3_F1 0
    %ifdef FP_SYSV
        mov  r8,  rdx        ; Win a3 (n)   <- SysV a3
        mov  rdx, rsi        ; Win a2 (out) <- SysV a2
        mov  rcx, rdi        ; Win a1 (in)  <- SysV a1
        vmovaps xmm3, xmm0   ; Win float a4 <- SysV float a1 (128-bit copy: ss/sd safe)
    %endif
%endmacro

; -----------------------------------------------------------------------------
; ABI_ARGS_PTR_F3 — 1 pointer arg + 3 float args.
; e.g. fp_euler_to_quat(Quaternion* out, float pitch, float yaw, float roll).
; Win64:  ptr rcx ; floats xmm1, xmm2, xmm3 (positional)      -> no-op.
; SysV:   ptr rdi ; floats xmm0, xmm1, xmm2 -> ptr rcx; floats xmm1,xmm2,xmm3.
;         Shift xmm2->xmm3, xmm1->xmm2, xmm0->xmm1 (high to low, no clobber).
; -----------------------------------------------------------------------------
%macro ABI_ARGS_PTR_F3 0
    %ifdef FP_SYSV
        mov  rcx, rdi
        vmovaps xmm3, xmm2
        vmovaps xmm2, xmm1
        vmovaps xmm1, xmm0
    %endif
%endmacro

; -----------------------------------------------------------------------------
; ABI_SCALAR5_INT reg — capture an integer 5th arg into `reg`.
; MUST be invoked as the very first instruction of the function (before any
; prologue push and before ABI_ARGS_INT), because on SysV the 5th int arg lives
; in r8, which ABI_ARGS_INT overwrites. On Win64 it is on the stack; we read it
; relative to rsp at entry (return addr + 4 home slots = [rsp+40]).
;   Win64 entry stack: [rsp]=ret, [rsp+8..32]=shadow home, [rsp+40]=arg5.
; -----------------------------------------------------------------------------
%macro ABI_SCALAR5_INT 1
    %ifdef FP_WIN64
        mov %1, [rsp + 40]
    %else
        mov %1, r8
    %endif
%endmacro

; -----------------------------------------------------------------------------
; ABI_SCALAR5_F reg — capture a float/double 5th arg into xmm `reg`.
; e.g. fp_map_axpy_f64(x, y, out, n, double c): c is arg5.
; Win64: on stack at entry [rsp+40]. SysV: first float arg -> xmm0.
; Invoke as the first instruction (before prologue push changes rsp).
; -----------------------------------------------------------------------------
%macro ABI_SCALAR5_F 1
    %ifdef FP_WIN64
        vmovsd %1, [rsp + 40]
    %else
        vmovaps %1, xmm0
    %endif
%endmacro

%endif ; FP_ABI_INC
