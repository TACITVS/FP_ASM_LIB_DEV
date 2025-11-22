; fp_core_tier2.asm
; Module 9: TIER 2 Operations - Set Operations
;
; Implements:
; - Set operations: unique, union, intersect (using sort + linear scan)
;
; Brings library from 70% to ~85% FP completeness

bits 64
default rel

section .text


; ============================================================================
; SET OPERATIONS
; ============================================================================

; ----------------------------------------------------------------------------
; fp_unique_i64
; Remove duplicate elements (requires sorted input for efficiency)
; Haskell: nub [1,2,2,3,3,3,4] → [1,2,3,4]
;
; Args: RCX = input (sorted), RDX = output, R8 = length
; Returns: RAX = number of unique elements
; ----------------------------------------------------------------------------
global fp_unique_i64
fp_unique_i64:
    test r8, r8
    jz .empty

    mov r10, rcx                ; r10 = input
    mov r11, rdx                ; r11 = output
    xor rax, rax                ; rax = output count

    ; Copy first element
    mov r9, [r10]
    mov [r11], r9
    inc rax
    add r11, 8
    add r10, 8

    mov rcx, 1                  ; rcx = input index

.loop:
    cmp rcx, r8
    jae .done

    mov r9, [r10]
    cmp r9, [r10 - 8]           ; Compare with previous
    je .skip                    ; Skip if duplicate

    mov [r11], r9
    inc rax
    add r11, 8

.skip:
    add r10, 8
    inc rcx
    jmp .loop

.empty:
    xor rax, rax
.done:
    ret

; ----------------------------------------------------------------------------
; fp_union_i64
; Union of two sorted arrays (with deduplication)
; Haskell: union [1,2,3] [2,3,4] → [1,2,3,4]
;
; Args: RCX = array_a (sorted), RDX = array_b (sorted), R8 = output,
;       R9 = len_a, [RBP+48] = len_b
; Returns: RAX = number of elements in union
; ----------------------------------------------------------------------------
global fp_union_i64
fp_union_i64:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov r10, rcx                ; r10 = array_a
    mov r11, rdx                ; r11 = array_b
    mov r12, r8                 ; r12 = output
    mov r13, r9                 ; r13 = len_a
    mov r14, [rbp+48]           ; r14 = len_b

    xor r15, r15                ; r15 = output count
    xor rbx, rbx                ; rbx = index_a
    xor rax, rax                ; rax = index_b

.merge_loop:
    cmp rbx, r13
    jae .copy_b_rest
    cmp rax, r14
    jae .copy_a_rest

    mov r8, [r10 + rbx*8]
    mov r9, [r11 + rax*8]

    cmp r8, r9
    jl .take_a
    jg .take_b

    ; Equal - take one and skip both
    mov [r12 + r15*8], r8
    inc r15
    inc rbx
    inc rax
    jmp .merge_loop

.take_a:
    mov [r12 + r15*8], r8
    inc r15
    inc rbx
    jmp .merge_loop

.take_b:
    mov [r12 + r15*8], r9
    inc r15
    inc rax
    jmp .merge_loop

.copy_a_rest:
    cmp rbx, r13
    jae .done
    mov r8, [r10 + rbx*8]
    mov [r12 + r15*8], r8
    inc r15
    inc rbx
    jmp .copy_a_rest

.copy_b_rest:
    cmp rax, r14
    jae .done
    mov r9, [r11 + rax*8]
    mov [r12 + r15*8], r9
    inc r15
    inc rax
    jmp .copy_b_rest

.done:
    mov rax, r15
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

; ----------------------------------------------------------------------------
; fp_intersect_i64
; Intersection of two sorted arrays
; Haskell: intersect [1,2,3] [2,3,4] → [2,3]
;
; Args: RCX = array_a (sorted), RDX = array_b (sorted), R8 = output,
;       R9 = len_a, [RBP+48] = len_b
; Returns: RAX = number of elements in intersection
; ----------------------------------------------------------------------------
global fp_intersect_i64
fp_intersect_i64:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov r10, rcx                ; r10 = array_a
    mov r11, rdx                ; r11 = array_b
    mov r12, r8                 ; r12 = output
    mov r13, r9                 ; r13 = len_a
    mov r14, [rbp+48]           ; r14 = len_b

    xor r15, r15                ; r15 = output count
    xor rbx, rbx                ; rbx = index_a
    xor rax, rax                ; rax = index_b

.merge_loop:
    cmp rbx, r13
    jae .done
    cmp rax, r14
    jae .done

    mov r8, [r10 + rbx*8]
    mov r9, [r11 + rax*8]

    cmp r8, r9
    jl .advance_a
    jg .advance_b

    ; Equal - add to intersection
    mov [r12 + r15*8], r8
    inc r15
    inc rbx
    inc rax
    jmp .merge_loop

.advance_a:
    inc rbx
    jmp .merge_loop

.advance_b:
    inc rax
    jmp .merge_loop

.done:
    mov rax, r15
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret