; fp_core_tier2.asm
; Module 9: TIER 2 Operations - Sorting and Set Operations
;
; Implements:
; - Sorting: quicksort with median-of-3 pivot + insertion sort for small arrays
; - Set operations: unique, union, intersect (using sort + linear scan)
;
; Brings library from 70% to ~85% FP completeness

bits 64
default rel

section .text

; ============================================================================
; SORTING OPERATIONS
; ============================================================================

; Insertion sort threshold - use insertion sort for arrays smaller than this
%define INSERTION_THRESHOLD 16

; ----------------------------------------------------------------------------
; fp_insertion_sort_i64
; Internal helper: insertion sort for small arrays
; Args: RCX = array, RDX = length
; ----------------------------------------------------------------------------
fp_insertion_sort_i64:
    cmp rdx, 1
    jbe .done

    mov r10, rcx                ; r10 = array base
    mov r11, 1                  ; r11 = i (start at index 1)

.outer:
    cmp r11, rdx
    jae .done

    ; Load key = array[i]
    mov rax, [r10 + r11*8]      ; rax = key
    mov r8, r11                 ; r8 = j

.inner:
    test r8, r8
    jz .insert

    ; Compare array[j-1] with key
    mov r9, [r10 + r8*8 - 8]
    cmp r9, rax
    jle .insert

    ; Shift right: array[j] = array[j-1]
    mov [r10 + r8*8], r9
    dec r8
    jmp .inner

.insert:
    ; Insert key at position j
    mov [r10 + r8*8], rax
    inc r11
    jmp .outer

.done:
    ret

; ----------------------------------------------------------------------------
; fp_partition_i64
; Internal helper: Partition array around pivot
; Args: RCX = array, RDX = low, R8 = high
; Returns: RAX = partition index
; ----------------------------------------------------------------------------
fp_partition_i64:
    push rbx
    push r12
    push r13
    push r14

    mov r10, rcx                ; r10 = array
    mov r11, rdx                ; r11 = low
    mov r12, r8                 ; r12 = high

    ; Median-of-three pivot selection
    mov r13, r11                ; r13 = left = low
    mov r14, r12                ; r14 = right = high
    mov rbx, r11
    add rbx, r12
    shr rbx, 1                  ; rbx = mid = (low + high) / 2

    ; Get three values
    mov rax, [r10 + r13*8]      ; val_left
    mov rcx, [r10 + rbx*8]      ; val_mid
    mov rdx, [r10 + r14*8]      ; val_right

    ; Sort three values and choose median
    ; Simple comparison network for 3 elements
    cmp rax, rcx
    jle .L1
    xchg rax, rcx
    xchg r13, rbx
.L1:
    cmp rcx, rdx
    jle .L2
    xchg rcx, rdx
    xchg rbx, r14
.L2:
    cmp rax, rcx
    jle .L3
    xchg rax, rcx
    xchg r13, rbx
.L3:
    ; Now rcx is median, rbx is its index
    ; Swap median to end
    mov rax, [r10 + rbx*8]
    mov rcx, [r10 + r12*8]
    mov [r10 + r12*8], rax
    mov [r10 + rbx*8], rcx

    ; Pivot is now at high
    mov rbx, [r10 + r12*8]      ; rbx = pivot
    mov r13, r11
    dec r13                     ; r13 = i = low - 1

    mov r14, r11                ; r14 = j

.partition_loop:
    cmp r14, r12
    jge .partition_done

    mov rax, [r10 + r14*8]
    cmp rax, rbx
    jg .skip_swap

    ; Swap array[++i] with array[j]
    inc r13
    mov rcx, [r10 + r13*8]
    mov [r10 + r13*8], rax
    mov [r10 + r14*8], rcx

.skip_swap:
    inc r14
    jmp .partition_loop

.partition_done:
    ; Swap array[i+1] with array[high]
    inc r13
    mov rax, [r10 + r13*8]
    mov rcx, [r10 + r12*8]
    mov [r10 + r13*8], rcx
    mov [r10 + r12*8], rax

    mov rax, r13                ; Return partition index

    pop r14
    pop r13
    pop r12
    pop rbx
    ret

; ----------------------------------------------------------------------------
; fp_quicksort_i64_recursive
; Internal helper: Recursive quicksort
; Args: RCX = array, RDX = low, R8 = high
; ----------------------------------------------------------------------------
fp_quicksort_i64_recursive:
    push rbp
    mov rbp, rsp
    sub rsp, 64                 ; Stack space for recursion state
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov r12, rcx                ; r12 = array
    mov r13, rdx                ; r13 = low
    mov r14, r8                 ; r14 = high

.tail_recurse:
    ; Check if subarray is small enough for insertion sort
    mov rax, r14
    sub rax, r13
    cmp rax, INSERTION_THRESHOLD
    jl .use_insertion

    cmp r13, r14
    jge .done

    ; Partition
    mov rcx, r12
    mov rdx, r13
    mov r8, r14
    call fp_partition_i64
    mov r15, rax                ; r15 = partition index

    ; Recursively sort smaller partition, tail-recurse on larger
    mov rax, r15
    sub rax, r13                ; size of left partition
    mov rbx, r14
    sub rbx, r15                ; size of right partition

    cmp rax, rbx
    jg .left_larger

    ; Right is larger - recurse on left, tail-recurse on right
    mov rcx, r12
    mov rdx, r13
    mov r8, r15
    dec r8
    call fp_quicksort_i64_recursive

    ; Tail recursion on right
    mov r13, r15
    inc r13
    jmp .tail_recurse

.left_larger:
    ; Left is larger - recurse on right, tail-recurse on left
    mov rcx, r12
    mov rdx, r15
    inc rdx
    mov r8, r14
    call fp_quicksort_i64_recursive

    ; Tail recursion on left
    mov r14, r15
    dec r14
    jmp .tail_recurse

.use_insertion:
    ; Use insertion sort for small subarray
    lea rcx, [r12 + r13*8]      ; Start of subarray
    mov rdx, r14
    sub rdx, r13
    inc rdx                     ; Length
    call fp_insertion_sort_i64
    jmp .done

.done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    mov rsp, rbp
    pop rbp
    ret

; ----------------------------------------------------------------------------
; fp_sort_i64
; Public API: Sort array of i64 in ascending order
; Haskell: sort [3,1,4,1,5,9] → [1,1,3,4,5,9]
;
; Args: RCX = array, RDX = length
; Note: Sorts in-place
; ----------------------------------------------------------------------------
global fp_sort_i64
fp_sort_i64:
    push rbp
    mov rbp, rsp

    test rdx, rdx
    jz .done
    cmp rdx, 1
    jbe .done

    ; Call quicksort
    push rdx                    ; Save length
    mov r8, rdx
    dec r8                      ; high = length - 1
    xor rdx, rdx                ; low = 0
    call fp_quicksort_i64_recursive
    pop rdx

.done:
    pop rbp
    ret

; ----------------------------------------------------------------------------
; Floating-point sort helpers
; ----------------------------------------------------------------------------

fp_insertion_sort_f64:
    cmp rdx, 1
    jbe .done

    mov r10, rcx
    mov r11, 1

.outer:
    cmp r11, rdx
    jae .done

    vmovsd xmm0, [r10 + r11*8]  ; key
    mov r8, r11

.inner:
    test r8, r8
    jz .insert

    vmovsd xmm1, [r10 + r8*8 - 8]
    vcomisd xmm1, xmm0
    jbe .insert

    vmovsd [r10 + r8*8], xmm1
    dec r8
    jmp .inner

.insert:
    vmovsd [r10 + r8*8], xmm0
    inc r11
    jmp .outer

.done:
    ret

fp_partition_f64:
    push rbx
    push r12
    push r13
    push r14
    sub rsp, 32

    mov r10, rcx
    mov r11, rdx
    mov r12, r8

    ; Use middle element as pivot (simple strategy for floats)
    mov rbx, r11
    add rbx, r12
    shr rbx, 1

    vmovsd xmm7, [r10 + rbx*8]  ; pivot

    ; Swap pivot to end
    vmovsd xmm0, [r10 + r12*8]
    vmovsd [r10 + r12*8], xmm7
    vmovsd [r10 + rbx*8], xmm0

    mov r13, r11
    dec r13
    mov r14, r11

.partition_loop:
    cmp r14, r12
    jge .partition_done

    vmovsd xmm0, [r10 + r14*8]
    vcomisd xmm0, xmm7
    ja .skip_swap

    inc r13
    vmovsd xmm1, [r10 + r13*8]
    vmovsd [r10 + r13*8], xmm0
    vmovsd [r10 + r14*8], xmm1

.skip_swap:
    inc r14
    jmp .partition_loop

.partition_done:
    inc r13
    vmovsd xmm0, [r10 + r13*8]
    vmovsd xmm1, [r10 + r12*8]
    vmovsd [r10 + r13*8], xmm1
    vmovsd [r10 + r12*8], xmm0

    mov rax, r13

    add rsp, 32
    pop r14
    pop r13
    pop r12
    pop rbx
    ret

fp_quicksort_f64_recursive:
    push rbp
    mov rbp, rsp
    sub rsp, 64
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov r12, rcx
    mov r13, rdx
    mov r14, r8

.tail_recurse:
    mov rax, r14
    sub rax, r13
    cmp rax, INSERTION_THRESHOLD
    jl .use_insertion

    cmp r13, r14
    jge .done

    mov rcx, r12
    mov rdx, r13
    mov r8, r14
    call fp_partition_f64
    mov r15, rax

    mov rax, r15
    sub rax, r13
    mov rbx, r14
    sub rbx, r15

    cmp rax, rbx
    jg .left_larger

    mov rcx, r12
    mov rdx, r13
    mov r8, r15
    dec r8
    call fp_quicksort_f64_recursive

    mov r13, r15
    inc r13
    jmp .tail_recurse

.left_larger:
    mov rcx, r12
    mov rdx, r15
    inc rdx
    mov r8, r14
    call fp_quicksort_f64_recursive

    mov r14, r15
    dec r14
    jmp .tail_recurse

.use_insertion:
    lea rcx, [r12 + r13*8]
    mov rdx, r14
    sub rdx, r13
    inc rdx
    call fp_insertion_sort_f64
    jmp .done

.done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    mov rsp, rbp
    pop rbp
    ret

; ----------------------------------------------------------------------------
; fp_sort_f64
; Public API: Sort array of f64 in ascending order
;
; Args: RCX = array, RDX = length
; ----------------------------------------------------------------------------
global fp_sort_f64
fp_sort_f64:
    push rbp
    mov rbp, rsp

    test rdx, rdx
    jz .done
    cmp rdx, 1
    jbe .done

    push rdx
    mov r8, rdx
    dec r8
    xor rdx, rdx
    call fp_quicksort_f64_recursive
    pop rdx

.done:
    vzeroupper
    pop rbp
    ret

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
