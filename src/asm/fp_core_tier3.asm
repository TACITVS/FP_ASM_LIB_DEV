; fp_core_tier3.asm
; Module 10: TIER 3 Operations - Advanced FP (Grouping, Unfold, Boolean)
;
; Implements:
; - Grouping: group (consecutive equal elements)
; - Unfold: iterate (generate sequences)
; - Boolean reductions: and, or
;
; Brings library from 85% to 100% FP completeness

bits 64
default rel

section .text

; ============================================================================
; GROUPING OPERATIONS
; ============================================================================

; ----------------------------------------------------------------------------
; fp_group_i64
; Group consecutive equal elements
; Haskell: group [1,1,2,2,2,3] → [[1,1],[2,2,2],[3]]
;
; Due to C constraints, returns:
; - groups[]: array of group representative values
; - counts[]: array of group sizes
; - Returns: number of groups
;
; Args: RCX = input, RDX = groups_out, R8 = counts_out, R9 = length
; Returns: RAX = number of groups
; ----------------------------------------------------------------------------
global fp_group_i64
fp_group_i64:
    test r9, r9
    jz .empty

    push rbx
    push r12
    push r13
    push r14

    mov r10, rcx                ; r10 = input
    mov r11, rdx                ; r11 = groups_out
    mov r12, r8                 ; r12 = counts_out
    xor r13, r13                ; r13 = group_count = 0
    xor r14, r14                ; r14 = input_index = 0

.start_new_group:
    cmp r14, r9
    jae .done

    ; Start new group with current element
    mov rax, [r10 + r14*8]      ; rax = current value
    mov [r11 + r13*8], rax      ; groups[group_count] = value

    mov rbx, 1                  ; rbx = current_group_size = 1
    inc r14

.count_group:
    cmp r14, r9
    jae .end_group

    mov rcx, [r10 + r14*8]      ; rcx = next value
    cmp rcx, rax                ; Compare with group value
    jne .end_group              ; Different value, end group

    inc rbx                     ; Same value, extend group
    inc r14
    jmp .count_group

.end_group:
    ; Store group size
    mov [r12 + r13*8], rbx      ; counts[group_count] = size
    inc r13                     ; Move to next group
    jmp .start_new_group

.done:
    mov rax, r13                ; Return number of groups
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

.empty:
    xor rax, rax
    ret

; ----------------------------------------------------------------------------
; fp_run_length_encode_i64
; Alternative grouping that's more efficient for run-length encoding
; Returns alternating [value1, count1, value2, count2, ...]
;
; Args: RCX = input, RDX = output, R8 = length
; Returns: RAX = number of output elements (pairs*2)
; ----------------------------------------------------------------------------
global fp_run_length_encode_i64
fp_run_length_encode_i64:
    test r8, r8
    jz .empty

    push rbx
    push r12
    push r13

    mov r10, rcx                ; r10 = input
    mov r11, rdx                ; r11 = output
    xor r12, r12                ; r12 = output_index = 0
    xor r13, r13                ; r13 = input_index = 0

.start_run:
    cmp r13, r8
    jae .done

    mov rax, [r10 + r13*8]      ; rax = current value
    mov rbx, 1                  ; rbx = run length
    inc r13

.count_run:
    cmp r13, r8
    jae .end_run

    mov rcx, [r10 + r13*8]
    cmp rcx, rax
    jne .end_run

    inc rbx
    inc r13
    jmp .count_run

.end_run:
    ; Store value and count
    mov [r11 + r12*8], rax
    mov [r11 + r12*8 + 8], rbx
    add r12, 2
    jmp .start_run

.done:
    mov rax, r12                ; Return output size
    pop r13
    pop r12
    pop rbx
    ret

.empty:
    xor rax, rax
    ret

; ============================================================================
; UNFOLD OPERATIONS (Sequence Generation)
; ============================================================================

; ----------------------------------------------------------------------------
; fp_iterate_add_i64
; Generate sequence by repeatedly adding a constant
; Haskell: take n $ iterate (+c) start → [start, start+c, start+2c, ...]
;
; Args: RCX = output, RDX = length, R8 = start_value, R9 = step
; ----------------------------------------------------------------------------
global fp_iterate_add_i64
fp_iterate_add_i64:
    test rdx, rdx
    jz .done

    mov r10, rcx                ; r10 = output
    mov rax, r8                 ; rax = current value
    xor r11, r11                ; r11 = index

.loop:
    cmp r11, rdx
    jae .done

    mov [r10 + r11*8], rax
    add rax, r9                 ; Add step
    inc r11
    jmp .loop

.done:
    ret

; ----------------------------------------------------------------------------
; fp_iterate_mul_i64
; Generate geometric sequence by repeatedly multiplying
; Haskell: take n $ iterate (*c) start → [start, start*c, start*c^2, ...]
;
; Args: RCX = output, RDX = length, R8 = start_value, R9 = multiplier
; ----------------------------------------------------------------------------
global fp_iterate_mul_i64
fp_iterate_mul_i64:
    test rdx, rdx
    jz .done

    mov r10, rcx                ; r10 = output
    mov rax, r8                 ; rax = current value
    xor r11, r11                ; r11 = index

.loop:
    cmp r11, rdx
    jae .done

    mov [r10 + r11*8], rax
    imul rax, r9                ; Multiply by factor
    inc r11
    jmp .loop

.done:
    ret

; ----------------------------------------------------------------------------
; fp_range_i64
; Generate range [start, start+1, ..., end-1]
; Haskell: [start..end-1]
;
; Args: RCX = output, RDX = start, R8 = end
; Returns: RAX = number of elements
; ----------------------------------------------------------------------------
global fp_range_i64
fp_range_i64:
    cmp rdx, r8
    jge .empty

    mov r10, rcx                ; r10 = output
    mov rax, rdx                ; rax = current
    xor r11, r11                ; r11 = index

.loop:
    cmp rax, r8
    jge .done

    mov [r10 + r11*8], rax
    inc rax
    inc r11
    jmp .loop

.done:
    mov rax, r11                ; Return count
    ret

.empty:
    xor rax, rax
    ret

; ============================================================================
; BOOLEAN REDUCTIONS
; ============================================================================

; ----------------------------------------------------------------------------
; fp_reduce_and_bool
; AND all boolean values (represented as int64_t: 0=false, non-zero=true)
; Haskell: and [True, True, False] → False
;
; Args: RCX = input, RDX = length
; Returns: RAX = 1 if all true, 0 if any false
; ----------------------------------------------------------------------------
global fp_reduce_and_bool
fp_reduce_and_bool:
    test rdx, rdx
    jz .all_true                ; Empty array → vacuously true

    mov r10, rcx                ; r10 = input
    xor r11, r11                ; r11 = index

    ; Use SIMD for parallel checking
    vpxor ymm0, ymm0, ymm0      ; ymm0 = all zeros

.loop4:
    mov rax, rdx
    sub rax, r11
    cmp rax, 4
    jb .tail

    ; Load 4 values and OR them with accumulator
    vmovdqu ymm1, [r10 + r11*8]
    vpor ymm0, ymm0, ymm1       ; Accumulate any non-zero

    ; Check if any are zero
    vpcmpeqq ymm2, ymm1, ymm0   ; Compare with zero
    vmovmskpd r8d, ymm2
    cmp r8d, 0
    jne .found_zero             ; If any zeros, AND fails

    add r11, 4
    jmp .loop4

.tail:
    cmp r11, rdx
    jae .all_true

.tail_loop:
    mov rax, [r10 + r11*8]
    test rax, rax
    jz .found_zero

    inc r11
    cmp r11, rdx
    jb .tail_loop

.all_true:
    mov rax, 1
    jmp .done

.found_zero:
    xor rax, rax

.done:
    vzeroupper
    ret

; ----------------------------------------------------------------------------
; fp_reduce_or_bool
; OR all boolean values
; Haskell: or [False, False, True] → True
;
; Args: RCX = input, RDX = length
; Returns: RAX = 1 if any true, 0 if all false
; ----------------------------------------------------------------------------
global fp_reduce_or_bool
fp_reduce_or_bool:
    test rdx, rdx
    jz .all_false               ; Empty array → false

    mov r10, rcx                ; r10 = input
    xor r11, r11                ; r11 = index

    ; Use SIMD for parallel checking
.loop4:
    mov rax, rdx
    sub rax, r11
    cmp rax, 4
    jb .tail

    ; Load 4 values and check if any non-zero
    vmovdqu ymm0, [r10 + r11*8]
    vpxor ymm1, ymm1, ymm1      ; ymm1 = zeros
    vpcmpeqq ymm2, ymm0, ymm1   ; Compare with zero
    vmovmskpd r8d, ymm2
    cmp r8d, 15                 ; All equal to zero?
    jne .found_true             ; If not all zero, found true

    add r11, 4
    jmp .loop4

.tail:
    cmp r11, rdx
    jae .all_false

.tail_loop:
    mov rax, [r10 + r11*8]
    test rax, rax
    jnz .found_true

    inc r11
    cmp r11, rdx
    jb .tail_loop

.all_false:
    xor rax, rax
    jmp .done

.found_true:
    mov rax, 1

.done:
    vzeroupper
    ret

; ============================================================================
; ZIP WITH INDEX
; ============================================================================

; ----------------------------------------------------------------------------
; fp_zip_with_index_i64
; Pair each element with its index
; Haskell: zip [0..] list → [(0,x0), (1,x1), (2,x2), ...]
;
; Returns interleaved array: [idx0, val0, idx1, val1, ...]
;
; Args: RCX = input, RDX = output, R8 = length
; Returns: RAX = output length (n*2)
; ----------------------------------------------------------------------------
global fp_zip_with_index_i64
fp_zip_with_index_i64:
    test r8, r8
    jz .empty

    mov r10, rcx                ; r10 = input
    mov r11, rdx                ; r11 = output
    xor rax, rax                ; rax = index

.loop:
    cmp rax, r8
    jae .done

    ; Store [index, value] pair
    mov [r11], rax
    mov r9, [r10 + rax*8]
    mov [r11 + 8], r9

    add r11, 16                 ; Move output pointer by 2
    inc rax
    jmp .loop

.done:
    shl rax, 1                  ; Return n*2
    ret

.empty:
    xor rax, rax
    ret

; ============================================================================
; ADDITIONAL UTILITY OPERATIONS
; ============================================================================

; ----------------------------------------------------------------------------
; fp_replicate_f64
; Fill array with single double value
; Haskell: replicate n x → [x,x,x,...] (n times)
;
; Args: RCX = output, RDX = length, XMM0 = value
; ----------------------------------------------------------------------------
global fp_replicate_f64
fp_replicate_f64:
    test rdx, rdx
    jz .done

    mov r10, rcx                ; r10 = output
    mov rcx, rdx                ; rcx = count
    vbroadcastsd ymm0, xmm0     ; Broadcast value

.loop4:
    cmp rcx, 4
    jb .tail
    vmovupd [r10], ymm0
    add r10, 32
    sub rcx, 4
    jmp .loop4

.tail:
    test rcx, rcx
    jz .done
.tail_loop:
    vmovsd [r10], xmm0
    add r10, 8
    dec rcx
    jnz .tail_loop

.done:
    vzeroupper
    ret

; ----------------------------------------------------------------------------
; fp_count_i64
; Count occurrences of a value
; Haskell: length . filter (== x)
;
; Args: RCX = input, RDX = length, R8 = target_value
; Returns: RAX = count
; ----------------------------------------------------------------------------
global fp_count_i64
fp_count_i64:
    test rdx, rdx
    jz .zero

    mov r10, rcx                ; r10 = input
    xor rax, rax                ; rax = count
    xor r11, r11                ; r11 = index
    vpbroadcastq ymm7, r8       ; ymm7 = target

.loop4:
    mov r9, rdx
    sub r9, r11
    cmp r9, 4
    jb .tail

    vmovdqu ymm0, [r10 + r11*8]
    vpcmpeqq ymm1, ymm0, ymm7
    vmovmskpd r9d, ymm1

    ; Count set bits in mask
    popcnt r9d, r9d
    add rax, r9

    add r11, 4
    jmp .loop4

.tail:
    cmp r11, rdx
    jae .done

.tail_loop:
    mov r9, [r10 + r11*8]
    cmp r9, r8                  ; Compare with target
    jne .skip
    inc rax
.skip:
    inc r11
    cmp r11, rdx
    jb .tail_loop

.zero:
    xor rax, rax
.done:
    vzeroupper
    ret
