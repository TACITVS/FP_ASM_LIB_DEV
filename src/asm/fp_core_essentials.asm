; fp_core_essentials.asm
; Module 8: Essential FP Operations
;
; TIER 1 operations that complete the FP standard library:
; - Index-based: take_n, drop_n, slice
; - Reductions: product, and, or
; - Search: find, contains, find_index
; - Manipulation: reverse, concat, replicate

bits 64
default rel

section .text

; ============================================================================
; Category 1: Index-Based Operations
; ============================================================================

; ----------------------------------------------------------------------------
; fp_take_n_i64
; Take first N elements from array
; Haskell: take 3 [1,2,3,4,5] → [1,2,3]
;
; Args: RCX = input, RDX = output, R8 = array_len, R9 = take_count
; Returns: RAX = number of elements taken (min(array_len, take_count))
; ----------------------------------------------------------------------------
global fp_take_n_i64
fp_take_n_i64:
    ; Determine actual count to take
    mov rax, r9                 ; RAX = take_count
    cmp rax, r8                 ; Compare with array_len
    cmova rax, r8               ; RAX = min(take_count, array_len)

    test rax, rax
    jz .done                    ; Nothing to take

    ; Copy RAX elements using SIMD
    mov r10, rcx                ; R10 = input ptr
    mov r11, rdx                ; R11 = output ptr
    mov rcx, rax                ; RCX = count

.loop4:
    cmp rcx, 4
    jb .tail
    vmovdqu ymm0, [r10]
    vmovdqu [r11], ymm0
    add r10, 32
    add r11, 32
    sub rcx, 4
    jmp .loop4

.tail:
    test rcx, rcx
    jz .done
.tail_loop:
    mov r8, [r10]
    mov [r11], r8
    add r10, 8
    add r11, 8
    dec rcx
    jnz .tail_loop

.done:
    vzeroupper
    ret

; ----------------------------------------------------------------------------
; fp_drop_n_i64
; Drop first N elements, return rest
; Haskell: drop 2 [1,2,3,4,5] → [3,4,5]
;
; Args: RCX = input, RDX = output, R8 = array_len, R9 = drop_count
; Returns: RAX = number of elements in output (max(0, array_len - drop_count))
; ----------------------------------------------------------------------------
global fp_drop_n_i64
fp_drop_n_i64:
    ; Calculate output count
    mov rax, r8                 ; RAX = array_len
    cmp r9, r8                  ; If drop_count >= array_len
    jae .empty                  ; Return empty

    sub rax, r9                 ; RAX = array_len - drop_count

    ; Adjust input pointer to skip first drop_count elements
    lea r10, [rcx + r9*8]       ; R10 = input + drop_count
    mov r11, rdx                ; R11 = output
    mov rcx, rax                ; RCX = count to copy

.loop4:
    cmp rcx, 4
    jb .tail
    vmovdqu ymm0, [r10]
    vmovdqu [r11], ymm0
    add r10, 32
    add r11, 32
    sub rcx, 4
    jmp .loop4

.tail:
    test rcx, rcx
    jz .done
.tail_loop:
    mov r8, [r10]
    mov [r11], r8
    add r10, 8
    add r11, 8
    dec rcx
    jnz .tail_loop
    jmp .done

.empty:
    xor rax, rax
.done:
    vzeroupper
    ret

; ----------------------------------------------------------------------------
; fp_slice_i64
; Extract elements from [start, end)
; Haskell: take (end - start) . drop start
;
; Args: RCX = input, RDX = output, R8 = array_len, R9 = start, [RBP+48] = end
; Returns: RAX = number of elements in output
; ----------------------------------------------------------------------------
global fp_slice_i64
fp_slice_i64:
    push rbp
    mov rbp, rsp

    mov r10, [rbp+48]           ; R10 = end

    ; Validate: start < array_len
    cmp r9, r8
    jae .empty

    ; Validate: end <= array_len
    cmp r10, r8
    cmova r10, r8               ; end = min(end, array_len)

    ; Validate: start < end
    cmp r9, r10
    jae .empty

    ; Calculate count and adjust pointer
    mov rax, r10
    sub rax, r9                 ; RAX = end - start
    lea r10, [rcx + r9*8]       ; R10 = input + start
    mov r11, rdx                ; R11 = output
    mov rcx, rax                ; RCX = count

.loop4:
    cmp rcx, 4
    jb .tail
    vmovdqu ymm0, [r10]
    vmovdqu [r11], ymm0
    add r10, 32
    add r11, 32
    sub rcx, 4
    jmp .loop4

.tail:
    test rcx, rcx
    jz .done
.tail_loop:
    mov r8, [r10]
    mov [r11], r8
    add r10, 8
    add r11, 8
    dec rcx
    jnz .tail_loop
    jmp .done

.empty:
    xor rax, rax
.done:
    vzeroupper
    pop rbp
    ret

; ============================================================================
; Category 2: Additional Reductions
; ============================================================================

; ----------------------------------------------------------------------------
; fp_reduce_product_i64
; Multiply all elements (scalar - no vpmulq in AVX2)
; Haskell: product [1,2,3,4] → 24
;
; Args: RCX = input, RDX = length
; Returns: RAX = product of all elements
; ----------------------------------------------------------------------------
global fp_reduce_product_i64
fp_reduce_product_i64:
    test rdx, rdx
    jz .zero

    mov r10, rcx                ; R10 = input
    mov rcx, rdx                ; RCX = count

    ; Initialize 4 accumulators for parallel multiplication
    mov rax, 1
    mov r8, 1
    mov r9, 1
    mov r11, 1

.loop4:
    cmp rcx, 4
    jb .final_mult
    imul rax, [r10]
    imul r8, [r10+8]
    imul r9, [r10+16]
    imul r11, [r10+24]
    add r10, 32
    sub rcx, 4
    jmp .loop4

.final_mult:
    ; Combine accumulators
    imul rax, r8
    imul rax, r9
    imul rax, r11

    ; Handle tail
    test rcx, rcx
    jz .done
.tail_loop:
    imul rax, [r10]
    add r10, 8
    dec rcx
    jnz .tail_loop
    jmp .done

.zero:
    mov rax, 1                  ; Empty product = 1 (identity)
.done:
    ret

; ----------------------------------------------------------------------------
; fp_reduce_product_f64
; Multiply all doubles (SIMD with vmulpd)
;
; Args: RCX = input, RDX = length
; Returns: XMM0 = product of all elements
; ----------------------------------------------------------------------------
global fp_reduce_product_f64
fp_reduce_product_f64:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    and rsp, 0xFFFFFFFFFFFFFFE0

    test rdx, rdx
    jz .zero

    mov r10, rcx                ; R10 = input
    mov rcx, rdx                ; RCX = count

    ; Initialize 4 accumulators to 1.0
    vbroadcastsd ymm0, [rel one_const]
    vmovapd ymm1, ymm0
    vmovapd ymm2, ymm0
    vmovapd ymm3, ymm0

.loop16:
    cmp rcx, 16
    jb .loop4
    vmulpd ymm0, ymm0, [r10]
    vmulpd ymm1, ymm1, [r10+32]
    vmulpd ymm2, ymm2, [r10+64]
    vmulpd ymm3, ymm3, [r10+96]
    add r10, 128
    sub rcx, 16
    jmp .loop16

.loop4:
    cmp rcx, 4
    jb .final_mult
    vmulpd ymm0, ymm0, [r10]
    add r10, 32
    sub rcx, 4
    jmp .loop4

.final_mult:
    ; Combine YMM accumulators
    vmulpd ymm0, ymm0, ymm1
    vmulpd ymm2, ymm2, ymm3
    vmulpd ymm0, ymm0, ymm2

    ; Horizontal multiply within YMM
    vextractf128 xmm1, ymm0, 1
    vmulpd xmm0, xmm0, xmm1     ; [prod01, prod23]
    vshufpd xmm1, xmm0, xmm0, 1
    vmulsd xmm0, xmm0, xmm1

    ; Handle tail
    test rcx, rcx
    jz .done
.tail_loop:
    vmulsd xmm0, xmm0, [r10]
    add r10, 8
    dec rcx
    jnz .tail_loop
    jmp .done

.zero:
    vmovsd xmm0, [rel one_const]
.done:
    vzeroupper
    mov rsp, rbp
    pop rbp
    ret

; ============================================================================
; Category 3: Search Operations
; ============================================================================

; ----------------------------------------------------------------------------
; fp_find_index_i64
; Find index of first element equal to target (SIMD)
; Haskell: findIndex (== x) list
;
; Args: RCX = input, RDX = length, R8 = target
; Returns: RAX = index (or -1 if not found)
; ----------------------------------------------------------------------------
global fp_find_index_i64
fp_find_index_i64:
    test rdx, rdx
    jz .not_found

    mov r10, rcx                ; R10 = input
    xor rax, rax                ; RAX = current index
    vpbroadcastq ymm7, r8       ; YMM7 = target (broadcast)

.loop4:
    mov rcx, rdx
    sub rcx, rax                ; RCX = remaining count
    cmp rcx, 4
    jb .tail

    vmovdqu ymm0, [r10]
    vpcmpeqq ymm1, ymm0, ymm7
    vmovmskpd r9d, ymm1
    test r9d, r9d
    jnz .found_in_chunk

    add r10, 32
    add rax, 4
    jmp .loop4

.found_in_chunk:
    ; Find which element matched (check bits 0,1,2,3)
    test r9d, 1
    jnz .done
    inc rax
    test r9d, 2
    jnz .done
    inc rax
    test r9d, 4
    jnz .done
    inc rax
    jmp .done

.tail:
    cmp rcx, 0
    jz .not_found
.tail_loop:
    mov r9, [r10]
    cmp r9, r8
    je .done
    add r10, 8
    inc rax
    dec rcx
    jnz .tail_loop

.not_found:
    mov rax, -1
.done:
    vzeroupper
    ret

; ----------------------------------------------------------------------------
; fp_contains_i64
; Check if array contains target (SIMD with early exit)
; Haskell: elem x list
;
; Args: RCX = input, RDX = length, R8 = target
; Returns: RAX = 1 if found, 0 if not found
; ----------------------------------------------------------------------------
global fp_contains_i64
fp_contains_i64:
    test rdx, rdx
    jz .not_found

    mov r10, rcx                ; R10 = input
    mov rcx, rdx                ; RCX = count
    vpbroadcastq ymm7, r8       ; YMM7 = target

.loop4:
    cmp rcx, 4
    jb .tail
    vmovdqu ymm0, [r10]
    vpcmpeqq ymm1, ymm0, ymm7
    vmovmskpd r9d, ymm1
    test r9d, r9d
    jnz .found
    add r10, 32
    sub rcx, 4
    jmp .loop4

.tail:
    test rcx, rcx
    jz .not_found
.tail_loop:
    mov r9, [r10]
    cmp r9, r8
    je .found
    add r10, 8
    dec rcx
    jnz .tail_loop

.not_found:
    xor rax, rax
    jmp .done
.found:
    mov rax, 1
.done:
    vzeroupper
    ret

; ============================================================================
; Category 4: Manipulation Operations
; ============================================================================

; ----------------------------------------------------------------------------
; fp_reverse_i64
; Reverse array order (SIMD with lane reversal)
; Haskell: reverse [1,2,3,4] → [4,3,2,1]
;
; Args: RCX = input, RDX = output, R8 = length
; ----------------------------------------------------------------------------
global fp_reverse_i64
fp_reverse_i64:
    test r8, r8
    jz .done

    ; Start from opposite ends
    mov r10, rcx                ; R10 = input start
    lea r11, [rcx + r8*8 - 8]   ; R11 = input end
    mov r12, rdx                ; R12 = output start

    mov rcx, r8                 ; RCX = count
    shr rcx, 1                  ; RCX = count / 2 (pairs to swap)

.loop:
    test rcx, rcx
    jz .check_odd

    mov rax, [r11]
    mov [r12], rax
    mov rax, [r10]
    mov [r12+8], rax

    sub r11, 8
    add r10, 8
    add r12, 16
    dec rcx
    jmp .loop

.check_odd:
    test r8, 1
    jz .done
    mov rax, [r11]
    mov [r12], rax

.done:
    ret

; ----------------------------------------------------------------------------
; fp_concat_i64
; Concatenate two arrays
; Haskell: [1,2,3] ++ [4,5,6] → [1,2,3,4,5,6]
;
; Args: RCX = input_a, RDX = input_b, R8 = output,
;       R9 = len_a, [RBP+48] = len_b
; Returns: RAX = total length (len_a + len_b)
; ----------------------------------------------------------------------------
global fp_concat_i64
fp_concat_i64:
    push rbp
    mov rbp, rsp

    mov r10, [rbp+48]           ; R10 = len_b

    ; Copy first array
    mov r11, rcx                ; R11 = input_a
    mov r12, r8                 ; R12 = output
    mov rcx, r9                 ; RCX = len_a

.copy_a_loop4:
    cmp rcx, 4
    jb .copy_a_tail
    vmovdqu ymm0, [r11]
    vmovdqu [r12], ymm0
    add r11, 32
    add r12, 32
    sub rcx, 4
    jmp .copy_a_loop4

.copy_a_tail:
    test rcx, rcx
    jz .copy_b
.copy_a_tail_loop:
    mov rax, [r11]
    mov [r12], rax
    add r11, 8
    add r12, 8
    dec rcx
    jnz .copy_a_tail_loop

.copy_b:
    ; Copy second array
    mov r11, rdx                ; R11 = input_b
    mov rcx, r10                ; RCX = len_b

.copy_b_loop4:
    cmp rcx, 4
    jb .copy_b_tail
    vmovdqu ymm0, [r11]
    vmovdqu [r12], ymm0
    add r11, 32
    add r12, 32
    sub rcx, 4
    jmp .copy_b_loop4

.copy_b_tail:
    test rcx, rcx
    jz .done
.copy_b_tail_loop:
    mov rax, [r11]
    mov [r12], rax
    add r11, 8
    add r12, 8
    dec rcx
    jnz .copy_b_tail_loop

.done:
    ; Return total length
    mov rax, r9
    add rax, r10
    vzeroupper
    pop rbp
    ret

; ----------------------------------------------------------------------------
; fp_replicate_i64
; Fill array with single value
; Haskell: replicate 5 7 → [7,7,7,7,7]
;
; Args: RCX = output, RDX = length, R8 = value
; ----------------------------------------------------------------------------
global fp_replicate_i64
fp_replicate_i64:
    test rdx, rdx
    jz .done

    mov r10, rcx                ; R10 = output
    mov rcx, rdx                ; RCX = count
    vpbroadcastq ymm0, r8       ; YMM0 = value broadcast

.loop4:
    cmp rcx, 4
    jb .tail
    vmovdqu [r10], ymm0
    add r10, 32
    sub rcx, 4
    jmp .loop4

.tail:
    test rcx, rcx
    jz .done
.tail_loop:
    mov [r10], r8
    add r10, 8
    dec rcx
    jnz .tail_loop

.done:
    vzeroupper
    ret

section .data
align 8
one_const: dq 1.0
