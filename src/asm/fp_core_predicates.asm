; =============================================================================
; FP-ASM Core Library: Predicates Module (Module 6)
;
; Implements boolean predicates (all/any) operations on arrays.
; Returns 1 (true) or 0 (false) based on comparison results.
;
; Strategy: AVX2 compare instructions + mask extraction
; =============================================================================

bits 64
default rel

section .text
    global fp_pred_all_eq_const_i64
    global fp_pred_any_gt_const_i64
    global fp_pred_all_gt_zip_i64

; ===========================================================================
; fp_pred_all_eq_const_i64: Check if all elements equal a constant
;
; Signature: bool fp_pred_all_eq_const_i64(const int64_t* arr, size_t n, int64_t value)
;
; Arguments:
;   RCX = arr   (const int64_t*)
;   RDX = n     (size_t)
;   R8  = value (int64_t constant to compare against)
;
; Returns: 1 if all elements equal value, 0 otherwise
; ===========================================================================
fp_pred_all_eq_const_i64:
    ; Early exit for empty array
    test rdx, rdx
    jz   .return_true        ; Empty array = vacuously true

    ; Broadcast constant to YMM register
    vmovq xmm5, r8
    vpbroadcastq ymm5, xmm5  ; ymm5 = [value, value, value, value]

    ; Process main loop (4 elements per iteration)
.loop4:
    cmp rdx, 4
    jb  .tail

    ; Load 4 elements and compare
    vmovdqu ymm0, [rcx]
    vpcmpeqq ymm0, ymm0, ymm5    ; ymm0 = (arr[i] == value) ? -1 : 0

    ; Extract comparison mask
    vmovmskpd eax, ymm0          ; Convert to bitmask (4 bits)
    cmp eax, 0xF                 ; All 4 equal? (0xF = 1111b)
    jne .return_false            ; If not all equal, return false

    add rcx, 32                  ; Advance pointer (4 × 8 bytes)
    sub rdx, 4
    jmp .loop4

.tail:
    test rdx, rdx
    jz   .return_true

    ; Scalar tail loop
.tail_loop:
    mov rax, [rcx]
    cmp rax, r8
    jne .return_false            ; Found mismatch

    add rcx, 8
    dec rdx
    jnz .tail_loop

.return_true:
    mov rax, 1                   ; Return true
    ret

.return_false:
    xor rax, rax                 ; Return false
    ret

; ===========================================================================
; fp_pred_any_gt_const_i64: Check if any element > constant
;
; Signature: bool fp_pred_any_gt_const_i64(const int64_t* arr, size_t n, int64_t value)
;
; Arguments:
;   RCX = arr   (const int64_t*)
;   RDX = n     (size_t)
;   R8  = value (int64_t constant to compare against)
;
; Returns: 1 if any element > value, 0 otherwise
; ===========================================================================
fp_pred_any_gt_const_i64:
    ; Early exit for empty array
    test rdx, rdx
    jz   .return_false       ; Empty array = no elements > value

    ; Broadcast constant to YMM register
    vmovq xmm5, r8
    vpbroadcastq ymm5, xmm5  ; ymm5 = [value, value, value, value]

    ; Process main loop (4 elements per iteration)
.loop4:
    cmp rdx, 4
    jb  .tail

    ; Load 4 elements and compare
    vmovdqu ymm0, [rcx]
    vpcmpgtq ymm0, ymm0, ymm5    ; ymm0 = (arr[i] > value) ? -1 : 0

    ; Extract comparison mask
    vmovmskpd eax, ymm0          ; Convert to bitmask
    test eax, eax                ; Any bit set?
    jnz .return_true             ; If yes, return true

    add rcx, 32
    sub rdx, 4
    jmp .loop4

.tail:
    test rdx, rdx
    jz   .return_false

    ; Scalar tail loop
.tail_loop:
    mov rax, [rcx]
    cmp rax, r8
    jg  .return_true             ; Found element > value

    add rcx, 8
    dec rdx
    jnz .tail_loop

.return_false:
    xor rax, rax                 ; Return false
    ret

.return_true:
    mov rax, 1                   ; Return true
    ret

; ===========================================================================
; fp_pred_all_gt_zip_i64: Check if all a[i] > b[i]
;
; Signature: bool fp_pred_all_gt_zip_i64(const int64_t* a, const int64_t* b, size_t n)
;
; Arguments:
;   RCX = a (const int64_t*)
;   RDX = b (const int64_t*)
;   R8  = n (size_t)
;
; Returns: 1 if all a[i] > b[i], 0 otherwise
; ===========================================================================
fp_pred_all_gt_zip_i64:
    ; Early exit for empty array
    test r8, r8
    jz   .return_true        ; Empty array = vacuously true

    mov r9, r8               ; Save n in r9 (use r8 for loop)

    ; Process main loop (4 elements per iteration)
.loop4:
    cmp r9, 4
    jb  .tail

    ; Load 4 elements from both arrays
    vmovdqu ymm0, [rcx]      ; a[i..i+3]
    vmovdqu ymm1, [rdx]      ; b[i..i+3]

    ; Compare: a > b
    vpcmpgtq ymm0, ymm0, ymm1    ; ymm0 = (a[i] > b[i]) ? -1 : 0

    ; Extract comparison mask
    vmovmskpd eax, ymm0          ; Convert to bitmask
    cmp eax, 0xF                 ; All 4 comparisons true?
    jne .return_false            ; If not, return false

    add rcx, 32
    add rdx, 32
    sub r9, 4
    jmp .loop4

.tail:
    test r9, r9
    jz   .return_true

    ; Scalar tail loop
.tail_loop:
    mov r10, [rcx]
    mov r11, [rdx]
    cmp r10, r11
    jle .return_false            ; If a[i] <= b[i], return false

    add rcx, 8
    add rdx, 8
    dec r9
    jnz .tail_loop

.return_true:
    mov rax, 1                   ; Return true
    ret

.return_false:
    xor rax, rax                 ; Return false
    ret
