	.file	"demo_bench_matrix_realworld.c"
	.intel_syntax noprefix
	.text
	.p2align 4
	.globl	create_perspective_matrix
	.def	create_perspective_matrix;	.scl	2;	.type	32;	.endef
	.seh_proc	create_perspective_matrix
create_perspective_matrix:
	push	rbx
	.seh_pushreg	rbx
	sub	rsp, 64
	.seh_stackalloc	64
	vmovups	XMMWORD PTR 48[rsp], xmm6
	.seh_savexmm	xmm6, 48
	.seh_endprologue
	vmulss	xmm0, xmm1, DWORD PTR .LC0[rip]
	vmovss	xmm6, DWORD PTR 112[rsp]
	mov	rbx, rcx
	vmovss	DWORD PTR 44[rsp], xmm2
	vmovss	DWORD PTR 104[rsp], xmm3
	call	tanf
	vmovss	xmm3, DWORD PTR 104[rsp]
	vmovss	xmm1, DWORD PTR .LC1[rip]
	mov	eax, 3212836864
	mov	QWORD PTR 4[rbx], 0
	vdivss	xmm0, xmm1, xmm0
	vsubss	xmm1, xmm3, xmm6
	mov	QWORD PTR 12[rbx], 0
	mov	QWORD PTR 24[rbx], 0
	mov	QWORD PTR 32[rbx], 0
	mov	QWORD PTR 44[rbx], rax
	mov	DWORD PTR 52[rbx], 0x00000000
	mov	DWORD PTR 60[rbx], 0x00000000
	vdivss	xmm2, xmm0, DWORD PTR 44[rsp]
	vmovss	DWORD PTR [rbx], xmm2
	vmovss	DWORD PTR 20[rbx], xmm0
	vaddss	xmm0, xmm6, xmm3
	vaddss	xmm6, xmm6, xmm6
	vmulss	xmm6, xmm6, xmm3
	vdivss	xmm0, xmm0, xmm1
	vdivss	xmm6, xmm6, xmm1
	vmovss	DWORD PTR 40[rbx], xmm0
	vmovss	DWORD PTR 56[rbx], xmm6
	vmovups	xmm6, XMMWORD PTR 48[rsp]
	add	rsp, 64
	pop	rbx
	ret
	.seh_endproc
	.p2align 4
	.globl	create_view_matrix
	.def	create_view_matrix;	.scl	2;	.type	32;	.endef
	.seh_proc	create_view_matrix
create_view_matrix:
	.seh_endprologue
	movabs	rax, 4575657221408423936
	vxorps	xmm1, xmm1, XMMWORD PTR .LC3[rip]
	vxorps	xmm2, xmm2, XMMWORD PTR .LC3[rip]
	vxorps	xmm3, xmm3, XMMWORD PTR .LC3[rip]
	mov	QWORD PTR [rcx], 1065353216
	mov	QWORD PTR 8[rcx], 0
	mov	QWORD PTR 16[rcx], rax
	mov	QWORD PTR 24[rcx], 0
	mov	QWORD PTR 32[rcx], 0
	mov	QWORD PTR 40[rcx], 1065353216
	mov	DWORD PTR 60[rcx], 0x3f800000
	vmovss	DWORD PTR 48[rcx], xmm1
	vmovss	DWORD PTR 52[rcx], xmm2
	vmovss	DWORD PTR 56[rcx], xmm3
	ret
	.seh_endproc
	.p2align 4
	.globl	create_model_matrix
	.def	create_model_matrix;	.scl	2;	.type	32;	.endef
	.seh_proc	create_model_matrix
create_model_matrix:
	.seh_endprologue
	vmovss	xmm0, DWORD PTR 40[rsp]
	mov	QWORD PTR 4[rcx], 0
	mov	QWORD PTR 12[rcx], 0
	mov	QWORD PTR 24[rcx], 0
	mov	QWORD PTR 32[rcx], 0
	mov	DWORD PTR 44[rcx], 0x00000000
	mov	DWORD PTR 60[rcx], 0x3f800000
	vmovss	DWORD PTR [rcx], xmm0
	vmovss	DWORD PTR 48[rcx], xmm1
	vmovss	DWORD PTR 20[rcx], xmm0
	vmovss	DWORD PTR 52[rcx], xmm2
	vmovss	DWORD PTR 40[rcx], xmm0
	vmovss	DWORD PTR 56[rcx], xmm3
	ret
	.seh_endproc
	.p2align 4
	.globl	compute_mvp_scalar
	.def	compute_mvp_scalar;	.scl	2;	.type	32;	.endef
	.seh_proc	compute_mvp_scalar
compute_mvp_scalar:
	sub	rsp, 232
	.seh_stackalloc	232
	vmovups	XMMWORD PTR 64[rsp], xmm6
	.seh_savexmm	xmm6, 64
	vmovups	XMMWORD PTR 80[rsp], xmm7
	.seh_savexmm	xmm7, 80
	vmovups	XMMWORD PTR 96[rsp], xmm8
	.seh_savexmm	xmm8, 96
	vmovups	XMMWORD PTR 112[rsp], xmm9
	.seh_savexmm	xmm9, 112
	vmovups	XMMWORD PTR 128[rsp], xmm10
	.seh_savexmm	xmm10, 128
	vmovups	XMMWORD PTR 144[rsp], xmm11
	.seh_savexmm	xmm11, 144
	vmovups	XMMWORD PTR 160[rsp], xmm12
	.seh_savexmm	xmm12, 160
	vmovups	XMMWORD PTR 176[rsp], xmm13
	.seh_savexmm	xmm13, 176
	vmovups	XMMWORD PTR 192[rsp], xmm14
	.seh_savexmm	xmm14, 192
	vmovups	XMMWORD PTR 208[rsp], xmm15
	.seh_savexmm	xmm15, 208
	.seh_endprologue
	vmovss	xmm3, DWORD PTR 28[r8]
	vmovss	xmm0, DWORD PTR 24[r8]
	vmovss	xmm13, DWORD PTR 60[r8]
	vmovss	xmm2, DWORD PTR [r8]
	vmovss	xmm15, DWORD PTR 4[r9]
	vmovss	xmm12, DWORD PTR 16[r8]
	vmovss	xmm14, DWORD PTR 44[r8]
	vmovss	xmm11, DWORD PTR 32[r8]
	vmovss	DWORD PTR 24[rsp], xmm3
	vmovss	xmm3, DWORD PTR [r9]
	mov	rax, rdx
	vmovss	xmm6, DWORD PTR 52[r8]
	vmovss	DWORD PTR 16[rsp], xmm0
	vxorps	xmm0, xmm0, xmm0
	vmovss	xmm10, DWORD PTR 48[r8]
	vmovss	xmm5, DWORD PTR 4[r8]
	vmovss	DWORD PTR 32[rsp], xmm13
	vmovaps	xmm13, xmm3
	vmovss	xmm7, DWORD PTR 20[r8]
	vmovss	xmm9, DWORD PTR 36[r8]
	vfmadd132ss	xmm13, xmm0, xmm2
	vmovss	DWORD PTR 28[rsp], xmm14
	vmovss	xmm14, DWORD PTR 8[r9]
	vmovss	xmm1, DWORD PTR 8[r8]
	vmovss	DWORD PTR 12[rsp], xmm6
	vmovss	xmm6, DWORD PTR 56[r8]
	vmovss	xmm4, DWORD PTR 40[r8]
	vmovss	DWORD PTR 4[rsp], xmm7
	vmovss	xmm8, DWORD PTR 12[r8]
	vmovss	DWORD PTR 20[rsp], xmm6
	vmovss	DWORD PTR 8[rsp], xmm9
	vfmadd231ss	xmm13, xmm15, xmm12
	vfmadd231ss	xmm13, xmm14, xmm11
	vmovaps	xmm6, xmm13
	vmovss	xmm13, DWORD PTR 12[r9]
	vfmadd231ss	xmm6, xmm13, xmm10
	vmovss	DWORD PTR 44[rsp], xmm6
	vmovaps	xmm6, xmm3
	vfmadd132ss	xmm6, xmm0, xmm5
	vfmadd231ss	xmm6, xmm15, xmm7
	vfmadd231ss	xmm6, xmm14, xmm9
	vmovaps	xmm9, xmm13
	vmovaps	xmm7, xmm6
	vfmadd231ss	xmm7, xmm13, DWORD PTR 12[rsp]
	vmovaps	xmm6, xmm3
	vfmadd132ss	xmm6, xmm0, xmm1
	vfmadd132ss	xmm3, xmm0, xmm8
	vfmadd231ss	xmm6, xmm15, DWORD PTR 16[rsp]
	vmovss	DWORD PTR 48[rsp], xmm7
	vfmadd231ss	xmm6, xmm14, xmm4
	vfmadd231ss	xmm6, xmm13, DWORD PTR 20[rsp]
	vfmadd231ss	xmm3, xmm15, DWORD PTR 24[rsp]
	vmovaps	xmm13, xmm2
	vmovss	DWORD PTR 40[rsp], xmm8
	vfmadd231ss	xmm3, xmm14, DWORD PTR 28[rsp]
	vmovss	xmm15, DWORD PTR 20[r9]
	vmovss	DWORD PTR 36[rsp], xmm4
	vfmadd132ss	xmm9, xmm3, DWORD PTR 32[rsp]
	vmovss	xmm3, DWORD PTR 16[r9]
	vmovss	xmm14, DWORD PTR 24[r9]
	vfmadd132ss	xmm13, xmm0, xmm3
	vmovss	DWORD PTR 52[rsp], xmm6
	vmovaps	xmm6, xmm10
	vmovss	DWORD PTR 56[rsp], xmm9
	vmovaps	xmm9, xmm1
	vfmadd132ss	xmm9, xmm0, xmm3
	vfmadd231ss	xmm13, xmm12, xmm15
	vfmadd231ss	xmm9, xmm15, DWORD PTR 16[rsp]
	vfmadd231ss	xmm13, xmm11, xmm14
	vfmadd231ss	xmm9, xmm14, xmm4
	vmovss	xmm4, DWORD PTR 16[rsp]
	vmovaps	xmm7, xmm13
	vmovss	xmm13, DWORD PTR 28[r9]
	vfmadd231ss	xmm9, xmm13, DWORD PTR 20[rsp]
	vfmadd231ss	xmm7, xmm13, xmm10
	vmovaps	xmm10, xmm5
	vfmadd132ss	xmm10, xmm0, xmm3
	vfmadd132ss	xmm3, xmm0, xmm8
	vmovaps	xmm8, xmm6
	vfmadd231ss	xmm10, xmm15, DWORD PTR 4[rsp]
	vfmadd231ss	xmm3, xmm15, DWORD PTR 24[rsp]
	vfmadd231ss	xmm3, xmm14, DWORD PTR 28[rsp]
	vfmadd231ss	xmm10, xmm14, DWORD PTR 8[rsp]
	vfmadd231ss	xmm10, xmm13, DWORD PTR 12[rsp]
	vfmadd132ss	xmm13, xmm3, DWORD PTR 32[rsp]
	vmovss	xmm3, DWORD PTR 32[r9]
	vmovss	xmm14, DWORD PTR 40[r9]
	vmovss	DWORD PTR 60[rsp], xmm7
	vmovss	xmm15, DWORD PTR 4[rsp]
	vmovd	edx, xmm13
	vmovaps	xmm13, xmm2
	vfmadd132ss	xmm13, xmm0, xmm3
	vfmadd231ss	xmm13, xmm12, DWORD PTR 36[r9]
	vfmadd231ss	xmm13, xmm11, xmm14
	vmovaps	xmm7, xmm13
	vmovss	xmm13, DWORD PTR 44[r9]
	vfmadd231ss	xmm7, xmm13, xmm6
	vmovaps	xmm6, xmm5
	vfmadd132ss	xmm6, xmm0, xmm3
	vfmadd231ss	xmm6, xmm15, DWORD PTR 36[r9]
	vmovaps	xmm15, xmm1
	vfmadd132ss	xmm15, xmm0, xmm3
	vfmadd231ss	xmm6, xmm14, DWORD PTR 8[rsp]
	vfmadd231ss	xmm15, xmm4, DWORD PTR 36[r9]
	vfmadd231ss	xmm6, xmm13, DWORD PTR 12[rsp]
	vfmadd231ss	xmm15, xmm14, DWORD PTR 36[rsp]
	vfmadd231ss	xmm15, xmm13, DWORD PTR 20[rsp]
	vfmadd132ss	xmm3, xmm0, DWORD PTR 40[rsp]
	vmovss	xmm4, DWORD PTR 24[rsp]
	vfmadd231ss	xmm3, xmm4, DWORD PTR 36[r9]
	vfmadd231ss	xmm3, xmm14, DWORD PTR 28[rsp]
	vmovss	xmm4, DWORD PTR 12[rsp]
	vfmadd132ss	xmm13, xmm3, DWORD PTR 32[rsp]
	vmovss	xmm3, DWORD PTR 48[r9]
	vfmadd132ss	xmm2, xmm0, xmm3
	vfmadd132ss	xmm5, xmm0, xmm3
	vfmadd132ss	xmm1, xmm0, xmm3
	vmovaps	xmm14, xmm13
	vmovss	xmm13, DWORD PTR 52[r9]
	vfmadd231ss	xmm5, xmm13, DWORD PTR 4[rsp]
	vfmadd231ss	xmm1, xmm13, DWORD PTR 16[rsp]
	vfmadd132ss	xmm12, xmm2, xmm13
	vmovss	xmm2, DWORD PTR 56[r9]
	vfmadd231ss	xmm5, xmm2, DWORD PTR 8[rsp]
	vfmadd231ss	xmm1, xmm2, DWORD PTR 36[rsp]
	vfmadd231ss	xmm12, xmm11, xmm2
	vmovss	xmm11, DWORD PTR 60[r9]
	vfmadd231ss	xmm1, xmm11, DWORD PTR 20[rsp]
	vfmadd132ss	xmm4, xmm5, xmm11
	vmovss	xmm5, DWORD PTR 40[rsp]
	vfmadd132ss	xmm5, xmm0, xmm3
	vfmadd231ss	xmm5, xmm13, DWORD PTR 24[rsp]
	vfmadd231ss	xmm12, xmm11, xmm8
	vmovss	xmm13, DWORD PTR 52[rsp]
	vfmadd132ss	xmm2, xmm5, DWORD PTR 28[rsp]
	vmovss	xmm5, DWORD PTR 44[rsp]
	vfmadd231ss	xmm2, xmm11, DWORD PTR 32[rsp]
	vmovss	xmm11, DWORD PTR 48[rsp]
	vmovaps	xmm3, xmm5
	vfmadd132ss	xmm3, xmm0, DWORD PTR [rax]
	vmovss	xmm8, DWORD PTR 56[rsp]
	vfmadd231ss	xmm3, xmm11, DWORD PTR 16[rax]
	vfmadd231ss	xmm3, xmm13, DWORD PTR 32[rax]
	vfmadd231ss	xmm3, xmm8, DWORD PTR 48[rax]
	vmovss	DWORD PTR [rcx], xmm3
	vmovaps	xmm3, xmm5
	vfmadd132ss	xmm3, xmm0, DWORD PTR 4[rax]
	vfmadd231ss	xmm3, xmm11, DWORD PTR 20[rax]
	vfmadd231ss	xmm3, xmm13, DWORD PTR 36[rax]
	vfmadd231ss	xmm3, xmm8, DWORD PTR 52[rax]
	vmovss	DWORD PTR 4[rcx], xmm3
	vmovaps	xmm3, xmm5
	vfmadd132ss	xmm3, xmm0, DWORD PTR 8[rax]
	vfmadd231ss	xmm3, xmm11, DWORD PTR 24[rax]
	vfmadd231ss	xmm3, xmm13, DWORD PTR 40[rax]
	vfmadd231ss	xmm3, xmm8, DWORD PTR 56[rax]
	vmovss	DWORD PTR 8[rcx], xmm3
	vfmadd132ss	xmm5, xmm0, DWORD PTR 12[rax]
	vmovaps	xmm3, xmm5
	vfmadd231ss	xmm3, xmm11, DWORD PTR 28[rax]
	vmovss	xmm5, DWORD PTR 60[rsp]
	vmovd	xmm11, edx
	vfmadd231ss	xmm3, xmm13, DWORD PTR 44[rax]
	vfmadd231ss	xmm3, xmm8, DWORD PTR 60[rax]
	vmovss	DWORD PTR 12[rcx], xmm3
	vmovaps	xmm3, xmm5
	vfmadd132ss	xmm3, xmm0, DWORD PTR [rax]
	vfmadd231ss	xmm3, xmm10, DWORD PTR 16[rax]
	vfmadd231ss	xmm3, xmm9, DWORD PTR 32[rax]
	vfmadd231ss	xmm3, xmm11, DWORD PTR 48[rax]
	vmovss	DWORD PTR 16[rcx], xmm3
	vmovaps	xmm3, xmm5
	vfmadd132ss	xmm3, xmm0, DWORD PTR 4[rax]
	vfmadd231ss	xmm3, xmm10, DWORD PTR 20[rax]
	vfmadd231ss	xmm3, xmm9, DWORD PTR 36[rax]
	vfmadd231ss	xmm3, xmm11, DWORD PTR 52[rax]
	vmovss	DWORD PTR 20[rcx], xmm3
	vmovaps	xmm3, xmm5
	vfmadd132ss	xmm3, xmm0, DWORD PTR 8[rax]
	vfmadd231ss	xmm3, xmm10, DWORD PTR 24[rax]
	vfmadd231ss	xmm3, xmm9, DWORD PTR 40[rax]
	vfmadd231ss	xmm3, xmm11, DWORD PTR 56[rax]
	vmovss	DWORD PTR 24[rcx], xmm3
	vfmadd132ss	xmm5, xmm0, DWORD PTR 12[rax]
	vmovaps	xmm3, xmm5
	vfmadd231ss	xmm3, xmm10, DWORD PTR 28[rax]
	vmovd	xmm5, edx
	vfmadd231ss	xmm3, xmm9, DWORD PTR 44[rax]
	vfmadd132ss	xmm5, xmm3, DWORD PTR 60[rax]
	vmovaps	xmm3, xmm7
	vmovss	DWORD PTR 28[rcx], xmm5
	vfmadd132ss	xmm3, xmm0, DWORD PTR [rax]
	vfmadd231ss	xmm3, xmm6, DWORD PTR 16[rax]
	vfmadd231ss	xmm3, xmm15, DWORD PTR 32[rax]
	vfmadd231ss	xmm3, xmm14, DWORD PTR 48[rax]
	vmovss	DWORD PTR 32[rcx], xmm3
	vmovaps	xmm3, xmm7
	vfmadd132ss	xmm3, xmm0, DWORD PTR 4[rax]
	vfmadd231ss	xmm3, xmm6, DWORD PTR 20[rax]
	vfmadd231ss	xmm3, xmm15, DWORD PTR 36[rax]
	vfmadd231ss	xmm3, xmm14, DWORD PTR 52[rax]
	vmovss	DWORD PTR 36[rcx], xmm3
	vmovaps	xmm3, xmm7
	vfmadd132ss	xmm3, xmm0, DWORD PTR 8[rax]
	vfmadd231ss	xmm3, xmm6, DWORD PTR 24[rax]
	vfmadd231ss	xmm3, xmm15, DWORD PTR 40[rax]
	vfmadd231ss	xmm3, xmm14, DWORD PTR 56[rax]
	vmovss	DWORD PTR 40[rcx], xmm3
	vfmadd132ss	xmm7, xmm0, DWORD PTR 12[rax]
	vmovaps	xmm3, xmm7
	vfmadd231ss	xmm3, xmm6, DWORD PTR 28[rax]
	vfmadd231ss	xmm3, xmm15, DWORD PTR 44[rax]
	vfmadd231ss	xmm3, xmm14, DWORD PTR 60[rax]
	vmovss	DWORD PTR 44[rcx], xmm3
	vmovaps	xmm3, xmm12
	vfmadd132ss	xmm3, xmm0, DWORD PTR [rax]
	vfmadd231ss	xmm3, xmm4, DWORD PTR 16[rax]
	vfmadd231ss	xmm3, xmm1, DWORD PTR 32[rax]
	vfmadd231ss	xmm3, xmm2, DWORD PTR 48[rax]
	vmovss	DWORD PTR 48[rcx], xmm3
	vmovaps	xmm3, xmm12
	vfmadd132ss	xmm3, xmm0, DWORD PTR 4[rax]
	vfmadd231ss	xmm3, xmm4, DWORD PTR 20[rax]
	vfmadd231ss	xmm3, xmm1, DWORD PTR 36[rax]
	vfmadd231ss	xmm3, xmm2, DWORD PTR 52[rax]
	vmovss	DWORD PTR 52[rcx], xmm3
	vmovaps	xmm3, xmm12
	vfmadd132ss	xmm3, xmm0, DWORD PTR 8[rax]
	vfmadd231ss	xmm3, xmm4, DWORD PTR 24[rax]
	vfmadd231ss	xmm3, xmm1, DWORD PTR 40[rax]
	vmovups	xmm6, XMMWORD PTR 64[rsp]
	vfmadd231ss	xmm3, xmm2, DWORD PTR 56[rax]
	vmovups	xmm7, XMMWORD PTR 80[rsp]
	vmovups	xmm8, XMMWORD PTR 96[rsp]
	vmovups	xmm9, XMMWORD PTR 112[rsp]
	vmovups	xmm10, XMMWORD PTR 128[rsp]
	vmovups	xmm11, XMMWORD PTR 144[rsp]
	vmovups	xmm13, XMMWORD PTR 176[rsp]
	vmovups	xmm14, XMMWORD PTR 192[rsp]
	vmovups	xmm15, XMMWORD PTR 208[rsp]
	vmovss	DWORD PTR 56[rcx], xmm3
	vfmadd132ss	xmm12, xmm0, DWORD PTR 12[rax]
	vfmadd132ss	xmm4, xmm12, DWORD PTR 28[rax]
	vmovups	xmm12, XMMWORD PTR 160[rsp]
	vfmadd132ss	xmm1, xmm4, DWORD PTR 44[rax]
	vfmadd132ss	xmm2, xmm1, DWORD PTR 60[rax]
	vmovss	DWORD PTR 60[rcx], xmm2
	add	rsp, 232
	ret
	.seh_endproc
	.p2align 4
	.globl	transform_vertices_scalar
	.def	transform_vertices_scalar;	.scl	2;	.type	32;	.endef
	.seh_proc	transform_vertices_scalar
transform_vertices_scalar:
	sub	rsp, 152
	.seh_stackalloc	152
	vmovups	XMMWORD PTR [rsp], xmm6
	.seh_savexmm	xmm6, 0
	vmovups	XMMWORD PTR 16[rsp], xmm7
	.seh_savexmm	xmm7, 16
	vmovups	XMMWORD PTR 32[rsp], xmm8
	.seh_savexmm	xmm8, 32
	vmovups	XMMWORD PTR 48[rsp], xmm9
	.seh_savexmm	xmm9, 48
	vmovups	XMMWORD PTR 64[rsp], xmm10
	.seh_savexmm	xmm10, 64
	vmovups	XMMWORD PTR 80[rsp], xmm11
	.seh_savexmm	xmm11, 80
	vmovups	XMMWORD PTR 96[rsp], xmm12
	.seh_savexmm	xmm12, 96
	vmovups	XMMWORD PTR 112[rsp], xmm13
	.seh_savexmm	xmm13, 112
	vmovups	XMMWORD PTR 128[rsp], xmm14
	.seh_savexmm	xmm14, 128
	.seh_endprologue
	test	r9d, r9d
	jle	.L10
	movsx	r9, r9d
	vmovss	xmm14, DWORD PTR [rdx]
	vmovss	xmm12, DWORD PTR 16[rdx]
	sal	r9, 4
	vmovss	xmm11, DWORD PTR 32[rdx]
	vmovss	xmm10, DWORD PTR 48[rdx]
	vmovss	xmm9, DWORD PTR 4[rdx]
	vmovss	xmm8, DWORD PTR 20[rdx]
	lea	rax, [r8+r9]
	vmovss	xmm7, DWORD PTR 36[rdx]
	vmovss	xmm6, DWORD PTR 52[rdx]
	vmovss	xmm5, DWORD PTR 8[rdx]
	vmovss	xmm13, DWORD PTR 24[rdx]
	vmovss	xmm4, DWORD PTR 40[rdx]
	vmovss	xmm3, DWORD PTR 56[rdx]
	.p2align 4,,10
	.p2align 3
.L8:
	vmovss	xmm0, DWORD PTR 4[r8]
	vmovss	xmm1, DWORD PTR 8[r8]
	add	r8, 16
	add	rcx, 16
	vmulss	xmm2, xmm12, xmm0
	vmulss	xmm0, xmm0, xmm8
	vfmadd231ss	xmm2, xmm14, DWORD PTR -16[r8]
	vfmadd231ss	xmm2, xmm11, xmm1
	vaddss	xmm2, xmm2, xmm10
	vmovss	DWORD PTR -16[rcx], xmm2
	vmovss	xmm2, DWORD PTR -16[r8]
	vfmadd231ss	xmm0, xmm9, xmm2
	vfmadd231ss	xmm0, xmm1, xmm7
	vaddss	xmm0, xmm0, xmm6
	vmovss	DWORD PTR -12[rcx], xmm0
	vmulss	xmm0, xmm13, DWORD PTR -12[r8]
	vfmadd132ss	xmm2, xmm0, xmm5
	vfmadd132ss	xmm1, xmm2, xmm4
	vaddss	xmm1, xmm1, xmm3
	vmovss	DWORD PTR -8[rcx], xmm1
	cmp	rax, r8
	jne	.L8
.L10:
	vmovups	xmm6, XMMWORD PTR [rsp]
	vmovups	xmm7, XMMWORD PTR 16[rsp]
	vmovups	xmm8, XMMWORD PTR 32[rsp]
	vmovups	xmm9, XMMWORD PTR 48[rsp]
	vmovups	xmm10, XMMWORD PTR 64[rsp]
	vmovups	xmm11, XMMWORD PTR 80[rsp]
	vmovups	xmm12, XMMWORD PTR 96[rsp]
	vmovups	xmm13, XMMWORD PTR 112[rsp]
	vmovups	xmm14, XMMWORD PTR 128[rsp]
	add	rsp, 152
	ret
	.seh_endproc
	.section .rdata,"dr"
.LC4:
	.ascii "AVX2 WINS!\0"
.LC5:
	.ascii "Needs optimization\0"
	.align 8
.LC6:
	.ascii "==============================================================\0"
	.align 8
.LC7:
	.ascii "  REAL-WORLD Game Engine Benchmark\0"
	.align 8
.LC8:
	.ascii "  Simulating 60 FPS @ 100 objects with 1000 vertices each\0"
	.align 8
.LC9:
	.ascii "==============================================================\12\0"
	.align 8
.LC11:
	.ascii "=== Scenario 1: MVP Chain (per-object, per-frame) ===\0"
	.align 8
.LC12:
	.ascii "Computing Model-View-Projection for %d objects \303\227 %d frames\12\0"
.LC13:
	.ascii "Total: %d MVP chains\12\12\0"
	.align 8
.LC17:
	.ascii "ASM (AVX2):         %.3f seconds (%.1f ns per MVP chain)\12\0"
	.align 8
.LC18:
	.ascii "Scalar (no SIMD):   %.3f seconds (%.1f ns per MVP chain)\12\0"
.LC19:
	.ascii "Speedup:            %.2fx\12\0"
	.align 8
.LC21:
	.ascii "FPS if this was the bottleneck (ASM):    %.1f FPS\12\0"
	.align 8
.LC22:
	.ascii "FPS if this was the bottleneck (Scalar): %.1f FPS\12\12\0"
	.align 8
.LC23:
	.ascii "=== Scenario 2: Vertex Transformation ===\0"
	.align 8
.LC24:
	.ascii "Transforming %d vertices per frame\12\0"
.LC25:
	.ascii "Total frames: %d\12\0"
.LC26:
	.ascii "Total: %d vertex transforms\12\12\0"
	.align 8
.LC30:
	.ascii "ASM (AVX2):         %.3f seconds (%.2f ns per vertex)\12\0"
	.align 8
.LC31:
	.ascii "Scalar (no SIMD):   %.3f seconds (%.2f ns per vertex)\12\0"
	.align 8
.LC32:
	.ascii "Throughput (ASM):   %.1f million vertices/sec\12\0"
	.align 8
.LC33:
	.ascii "Throughput (Scalar): %.1f million vertices/sec\12\12\0"
.LC34:
	.ascii "  Result: %s\12\0"
	.section	.text.startup,"x"
	.p2align 4
	.globl	main
	.def	main;	.scl	2;	.type	32;	.endef
	.seh_proc	main
main:
	push	r15
	.seh_pushreg	r15
	push	r14
	.seh_pushreg	r14
	push	r13
	.seh_pushreg	r13
	push	r12
	.seh_pushreg	r12
	push	rbp
	.seh_pushreg	rbp
	push	rdi
	.seh_pushreg	rdi
	push	rsi
	.seh_pushreg	rsi
	push	rbx
	.seh_pushreg	rbx
	sub	rsp, 504
	.seh_stackalloc	504
	vmovups	XMMWORD PTR 336[rsp], xmm6
	.seh_savexmm	xmm6, 336
	vmovups	XMMWORD PTR 352[rsp], xmm7
	.seh_savexmm	xmm7, 352
	vmovups	XMMWORD PTR 368[rsp], xmm8
	.seh_savexmm	xmm8, 368
	vmovups	XMMWORD PTR 384[rsp], xmm9
	.seh_savexmm	xmm9, 384
	vmovups	XMMWORD PTR 400[rsp], xmm10
	.seh_savexmm	xmm10, 400
	vmovups	XMMWORD PTR 416[rsp], xmm11
	.seh_savexmm	xmm11, 416
	vmovups	XMMWORD PTR 432[rsp], xmm12
	.seh_savexmm	xmm12, 432
	vmovups	XMMWORD PTR 448[rsp], xmm13
	.seh_savexmm	xmm13, 448
	vmovups	XMMWORD PTR 464[rsp], xmm14
	.seh_savexmm	xmm14, 464
	vmovups	XMMWORD PTR 480[rsp], xmm15
	.seh_savexmm	xmm15, 480
	.seh_endprologue
	call	__main
	lea	rcx, .LC6[rip]
	call	puts
	lea	rcx, .LC7[rip]
	call	puts
	lea	rcx, .LC8[rip]
	call	puts
	lea	rcx, .LC9[rip]
	call	puts
	mov	ecx, 16000
	movabs	rax, 4575685920379895808
	mov	QWORD PTR 80[rsp], 1058020701
	mov	QWORD PTR 96[rsp], rax
	movabs	rax, -4647714812233513331
	mov	QWORD PTR 120[rsp], rax
	mov	eax, 3192705547
	mov	QWORD PTR 136[rsp], rax
	movabs	rax, 4575657221408423936
	mov	QWORD PTR 160[rsp], rax
	movabs	rax, -4566650020006199296
	mov	QWORD PTR 192[rsp], rax
	movabs	rax, 4575657222501040128
	mov	QWORD PTR 88[rsp], 0
	mov	QWORD PTR 104[rsp], 0
	mov	QWORD PTR 112[rsp], 0
	mov	QWORD PTR 128[rsp], 0
	mov	QWORD PTR 144[rsp], 1065353216
	mov	QWORD PTR 152[rsp], 0
	mov	QWORD PTR 168[rsp], 0
	mov	QWORD PTR 176[rsp], 0
	mov	QWORD PTR 184[rsp], 1065353216
	mov	QWORD PTR 200[rsp], rax
	call	malloc
	mov	ecx, 16000
	mov	rbp, rax
	call	malloc
	vmovss	xmm1, DWORD PTR .LC10[rip]
	mov	r8, rbp
	xor	r9d, r9d
	mov	r12, rax
	mov	r10d, 3435973837
.L12:
	mov	ecx, r9d
	mov	r11d, r9d
	vxorps	xmm4, xmm4, xmm4
	inc	r9d
	mov	rax, rcx
	mov	DWORD PTR 12[r8], 0x00000000
	add	r8, 16
	imul	rax, r10
	shr	rax, 35
	lea	edx, [rax+rax*4]
	add	edx, edx
	sub	r11d, edx
	mov	edx, eax
	vcvtsi2ss	xmm0, xmm4, r11d
	imul	rdx, r10
	shr	rdx, 35
	vsubss	xmm0, xmm0, xmm1
	lea	edx, [rdx+rdx*4]
	add	edx, edx
	sub	eax, edx
	vmovss	DWORD PTR -16[r8], xmm0
	vcvtsi2ss	xmm0, xmm4, eax
	imul	rax, rcx, 1374389535
	vsubss	xmm0, xmm0, xmm1
	shr	rax, 37
	vmovss	DWORD PTR -12[r8], xmm0
	vcvtsi2ss	xmm0, xmm4, eax
	vsubss	xmm0, xmm0, xmm1
	vmovss	DWORD PTR -8[r8], xmm0
	cmp	r9d, 1000
	jne	.L12
	lea	rcx, .LC11[rip]
	call	puts
	mov	r8d, 1000
	mov	edx, 100
	lea	rcx, .LC12[rip]
	call	__mingw_printf
	mov	edx, 100000
	lea	rcx, .LC13[rip]
	call	__mingw_printf
	mov	ecx, 6400
	call	malloc
	vmovss	xmm0, DWORD PTR .LC1[rip]
	xor	ecx, ecx
	mov	r10d, 3435973837
	mov	QWORD PTR 40[rsp], rax
.L13:
	mov	edx, ecx
	mov	r9d, ecx
	vxorps	xmm5, xmm5, xmm5
	mov	DWORD PTR 16[rax], 0x00000000
	imul	rdx, r10
	mov	DWORD PTR 32[rax], 0x00000000
	add	rax, 64
	vmovss	DWORD PTR -64[rax], xmm0
	mov	DWORD PTR -60[rax], 0x00000000
	shr	rdx, 35
	vmovss	DWORD PTR -44[rax], xmm0
	lea	r8d, [rdx+rdx*4]
	mov	DWORD PTR -28[rax], 0x00000000
	add	r8d, r8d
	mov	DWORD PTR -56[rax], 0x00000000
	sub	r9d, r8d
	mov	DWORD PTR -40[rax], 0x00000000
	vcvtsi2ss	xmm1, xmm5, r9d
	vmovss	DWORD PTR -24[rax], xmm0
	mov	DWORD PTR -52[rax], 0x00000000
	mov	DWORD PTR -36[rax], 0x00000000
	mov	DWORD PTR -20[rax], 0x00000000
	vmovss	DWORD PTR -16[rax], xmm1
	vcvtsi2ss	xmm1, xmm5, edx
	vmovss	DWORD PTR -4[rax], xmm0
	vmovss	DWORD PTR -12[rax], xmm1
	vcvtsi2ss	xmm1, xmm5, ecx
	inc	ecx
	vmovss	DWORD PTR -8[rax], xmm1
	cmp	ecx, 100
	jne	.L13
	mov	DWORD PTR 76[rsp], 0x00000000
	mov	r13d, 1000
	lea	rsi, 80[rsp]
	lea	rbx, 272[rsp]
	call	clock
	mov	QWORD PTR 48[rsp], rbp
	mov	DWORD PTR 36[rsp], eax
	mov	rax, QWORD PTR 40[rsp]
	mov	QWORD PTR 56[rsp], r12
	lea	r14, 6400[rax]
	mov	r12, rax
.L14:
	mov	rbp, r12
	.p2align 4,,10
	.p2align 3
.L15:
	mov	r8, rbp
	lea	rdx, 144[rsp]
	add	rbp, 64
	lea	rcx, 208[rsp]
	call	fp_mat4_mul
	lea	r8, 208[rsp]
	mov	rdx, rsi
	mov	rcx, rbx
	call	fp_mat4_mul
	vmovss	xmm0, DWORD PTR 76[rsp]
	vaddss	xmm0, xmm0, DWORD PTR 272[rsp]
	vmovss	DWORD PTR 76[rsp], xmm0
	cmp	r14, rbp
	jne	.L15
	dec	r13d
	jne	.L14
	mov	rbp, QWORD PTR 48[rsp]
	mov	r12, QWORD PTR 56[rsp]
	mov	r13d, 1000
	call	clock
	vxorpd	xmm7, xmm7, xmm7
	sub	eax, DWORD PTR 36[rsp]
	vcvtsi2sd	xmm7, xmm7, eax
	vdivsd	xmm7, xmm7, QWORD PTR .LC14[rip]
	call	clock
	mov	r10, QWORD PTR 40[rsp]
	mov	edi, eax
	lea	r8, 144[rsp]
.L17:
	mov	r9, r10
	.p2align 4,,10
	.p2align 3
.L18:
	mov	rdx, rsi
	mov	rcx, rbx
	call	compute_mvp_scalar
	vmovss	xmm0, DWORD PTR 76[rsp]
	add	r9, 64
	vaddss	xmm0, xmm0, DWORD PTR 272[rsp]
	vmovss	DWORD PTR 76[rsp], xmm0
	cmp	r14, r9
	jne	.L18
	dec	r13d
	jne	.L17
	mov	r15, r8
	call	clock
	lea	rcx, .LC17[rip]
	vxorpd	xmm6, xmm6, xmm6
	vmovsd	xmm8, QWORD PTR .LC15[rip]
	vmovq	rdx, xmm7
	sub	eax, edi
	vdivsd	xmm0, xmm7, xmm8
	vcvtsi2sd	xmm6, xmm6, eax
	vdivsd	xmm6, xmm6, QWORD PTR .LC14[rip]
	vmulsd	xmm1, xmm0, QWORD PTR .LC16[rip]
	vmovq	r8, xmm1
	vmovapd	xmm2, xmm1
	vmovapd	xmm1, xmm7
	call	__mingw_printf
	lea	rcx, .LC18[rip]
	vdivsd	xmm8, xmm6, xmm8
	vmovq	rdx, xmm6
	vmulsd	xmm1, xmm8, QWORD PTR .LC16[rip]
	vmovapd	xmm2, xmm1
	vmovq	r8, xmm1
	vmovapd	xmm1, xmm6
	call	__mingw_printf
	lea	rcx, .LC19[rip]
	vdivsd	xmm1, xmm6, xmm7
	vmovq	rdx, xmm1
	call	__mingw_printf
	vdivsd	xmm1, xmm7, QWORD PTR .LC14[rip]
	vmovsd	xmm7, QWORD PTR .LC20[rip]
	lea	rcx, .LC21[rip]
	vdivsd	xmm1, xmm7, xmm1
	vmovq	rdx, xmm1
	call	__mingw_printf
	vmovsd	xmm7, QWORD PTR .LC20[rip]
	vdivsd	xmm1, xmm6, QWORD PTR .LC14[rip]
	lea	rcx, .LC22[rip]
	vdivsd	xmm1, xmm7, xmm1
	vmovq	rdx, xmm1
	call	__mingw_printf
	lea	rcx, .LC23[rip]
	call	puts
	mov	edx, 1000
	lea	rcx, .LC24[rip]
	call	__mingw_printf
	mov	edx, 1000
	lea	rcx, .LC25[rip]
	call	__mingw_printf
	mov	edx, 1000000
	lea	rcx, .LC26[rip]
	call	__mingw_printf
	mov	rdx, rsi
	mov	r8, r15
	mov	rcx, rbx
	call	fp_mat4_mul
	mov	esi, 1000
	call	clock
	mov	edi, eax
.L20:
	mov	r9d, 1000
	mov	r8, rbp
	mov	rdx, rbx
	mov	rcx, r12
	call	fp_mat4_mul_vec3_batch
	vmovss	xmm6, DWORD PTR 0[rbp]
	vmovss	xmm0, DWORD PTR .LC27[rip]
	vfmadd132ss	xmm0, xmm6, DWORD PTR 15984[r12]
	vmovss	DWORD PTR 0[rbp], xmm0
	dec	esi
	jne	.L20
	call	clock
	vxorpd	xmm7, xmm7, xmm7
	vmovss	xmm1, DWORD PTR 76[rsp]
	sub	eax, edi
	vcvtsi2sd	xmm0, xmm7, eax
	vdivsd	xmm7, xmm0, QWORD PTR .LC14[rip]
	vmovq	rbx, xmm7
	vmovss	xmm0, DWORD PTR [r12]
	vaddss	xmm0, xmm0, DWORD PTR 0[rbp]
	mov	DWORD PTR 0[rbp], 0xc0a00000
	vaddss	xmm0, xmm0, xmm1
	vmovss	DWORD PTR 76[rsp], xmm0
	call	clock
	vmovss	xmm1, DWORD PTR 0[rbp]
	lea	r9, 4[rbp]
	vmovss	xmm15, DWORD PTR 272[rsp]
	vmovss	xmm14, DWORD PTR 276[rsp]
	mov	esi, eax
	vmovss	xmm13, DWORD PTR 280[rsp]
	mov	r8d, 1000
	vmovss	xmm12, DWORD PTR 288[rsp]
	lea	rcx, 16004[rbp]
	vmovss	xmm11, DWORD PTR 292[rsp]
	vmovss	DWORD PTR 36[rsp], xmm15
	vmovss	xmm10, DWORD PTR 296[rsp]
	vmovss	xmm9, DWORD PTR 304[rsp]
	vmovss	xmm8, DWORD PTR 308[rsp]
	vmovss	xmm7, DWORD PTR 312[rsp]
	vmovss	xmm6, DWORD PTR 320[rsp]
	vmovss	xmm5, DWORD PTR 324[rsp]
	vmovss	xmm4, DWORD PTR 328[rsp]
.L23:
	mov	rax, r9
	mov	rdx, r12
	vmovaps	xmm0, xmm1
	jmp	.L22
	.p2align 4,,10
	.p2align 3
.L34:
	vmovss	xmm0, DWORD PTR -4[rax]
	add	rdx, 16
.L22:
	vmovss	xmm15, DWORD PTR [rax]
	vmovss	xmm2, DWORD PTR 4[rax]
	add	rax, 16
	vmulss	xmm3, xmm12, xmm15
	vfmadd231ss	xmm3, xmm0, DWORD PTR 36[rsp]
	vfmadd231ss	xmm3, xmm9, xmm2
	vaddss	xmm3, xmm3, xmm6
	vmovss	DWORD PTR [rdx], xmm3
	vmulss	xmm3, xmm11, xmm15
	vmulss	xmm15, xmm10, xmm15
	vfmadd231ss	xmm3, xmm14, xmm0
	vfmadd231ss	xmm15, xmm13, xmm0
	vfmadd231ss	xmm3, xmm8, xmm2
	vfmadd132ss	xmm2, xmm15, xmm7
	vaddss	xmm3, xmm3, xmm5
	vaddss	xmm2, xmm2, xmm4
	vmovss	DWORD PTR 4[rdx], xmm3
	vmovss	DWORD PTR 8[rdx], xmm2
	cmp	rcx, rax
	jne	.L34
	vmovss	xmm2, DWORD PTR .LC27[rip]
	vfmadd231ss	xmm1, xmm2, DWORD PTR 15984[r12]
	vmovss	DWORD PTR 0[rbp], xmm1
	dec	r8d
	jne	.L23
	call	clock
	vmovss	xmm0, DWORD PTR [r12]
	vaddss	xmm0, xmm0, DWORD PTR 0[rbp]
	mov	rdx, rbx
	vmovss	xmm1, DWORD PTR 76[rsp]
	sub	eax, esi
	vxorpd	xmm7, xmm7, xmm7
	lea	rcx, .LC30[rip]
	vcvtsi2sd	xmm6, xmm7, eax
	vmovsd	xmm7, QWORD PTR .LC29[rip]
	vdivsd	xmm6, xmm6, QWORD PTR .LC14[rip]
	vaddss	xmm0, xmm0, xmm1
	vmovq	xmm1, rbx
	vmovss	DWORD PTR 76[rsp], xmm0
	vdivsd	xmm0, xmm1, xmm7
	vmulsd	xmm1, xmm0, QWORD PTR .LC16[rip]
	vmovq	r8, xmm1
	vmovapd	xmm2, xmm1
	vmovq	xmm1, rbx
	call	__mingw_printf
	lea	rcx, .LC31[rip]
	vdivsd	xmm0, xmm6, xmm7
	vmovq	rdx, xmm6
	vmulsd	xmm1, xmm0, QWORD PTR .LC16[rip]
	vmovq	r8, xmm1
	vmovapd	xmm2, xmm1
	vmovapd	xmm1, xmm6
	call	__mingw_printf
	vmovq	xmm1, rbx
	lea	rcx, .LC19[rip]
	vdivsd	xmm8, xmm6, xmm1
	vmovapd	xmm1, xmm8
	vmovq	rdx, xmm8
	call	__mingw_printf
	vmovq	xmm1, rbx
	lea	rcx, .LC32[rip]
	vdivsd	xmm1, xmm7, xmm1
	vdivsd	xmm1, xmm1, xmm7
	vmovq	rdx, xmm1
	call	__mingw_printf
	lea	rcx, .LC33[rip]
	vdivsd	xmm1, xmm7, xmm6
	vdivsd	xmm1, xmm1, xmm7
	vmovq	rdx, xmm1
	call	__mingw_printf
	lea	rcx, .LC6[rip]
	call	puts
	vcomisd	xmm8, QWORD PTR .LC20[rip]
	lea	rax, .LC5[rip]
	lea	rdx, .LC4[rip]
	lea	rcx, .LC34[rip]
	cmovbe	rdx, rax
	call	__mingw_printf
	lea	rcx, .LC6[rip]
	call	puts
	mov	rcx, rbp
	call	free
	mov	rcx, r12
	call	free
	mov	rcx, QWORD PTR 40[rsp]
	call	free
	xor	eax, eax
	vmovups	xmm6, XMMWORD PTR 336[rsp]
	vmovups	xmm7, XMMWORD PTR 352[rsp]
	vmovups	xmm8, XMMWORD PTR 368[rsp]
	vmovups	xmm9, XMMWORD PTR 384[rsp]
	vmovups	xmm10, XMMWORD PTR 400[rsp]
	vmovups	xmm11, XMMWORD PTR 416[rsp]
	vmovups	xmm12, XMMWORD PTR 432[rsp]
	vmovups	xmm13, XMMWORD PTR 448[rsp]
	vmovups	xmm14, XMMWORD PTR 464[rsp]
	vmovups	xmm15, XMMWORD PTR 480[rsp]
	add	rsp, 504
	pop	rbx
	pop	rsi
	pop	rdi
	pop	rbp
	pop	r12
	pop	r13
	pop	r14
	pop	r15
	ret
	.seh_endproc
	.section .rdata,"dr"
	.align 4
.LC0:
	.long	1056964608
	.align 4
.LC1:
	.long	1065353216
	.align 16
.LC3:
	.long	-2147483648
	.long	0
	.long	0
	.long	0
	.align 4
.LC10:
	.long	1084227584
	.align 8
.LC14:
	.long	0
	.long	1083129856
	.align 8
.LC15:
	.long	0
	.long	1090021888
	.align 8
.LC16:
	.long	0
	.long	1104006501
	.align 8
.LC20:
	.long	0
	.long	1072693248
	.align 4
.LC27:
	.long	953267991
	.align 8
.LC29:
	.long	0
	.long	1093567616
	.def	__main;	.scl	2;	.type	32;	.endef
	.ident	"GCC: (Rev5, Built by MSYS2 project) 15.1.0"
	.def	tanf;	.scl	2;	.type	32;	.endef
	.def	puts;	.scl	2;	.type	32;	.endef
	.def	malloc;	.scl	2;	.type	32;	.endef
	.def	clock;	.scl	2;	.type	32;	.endef
	.def	fp_mat4_mul;	.scl	2;	.type	32;	.endef
	.def	fp_mat4_mul_vec3_batch;	.scl	2;	.type	32;	.endef
	.def	free;	.scl	2;	.type	32;	.endef
