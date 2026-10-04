/* Callee-saved register canary test (System V x86-64 / ELF).
 *
 * Every fold/reduction kernel — and every ISA variant this CPU can run — is
 * called through a trampoline that loads rbx, rbp, r12-r15 with canaries and
 * checks them afterwards. A kernel that clobbers one corrupts its caller's
 * variables in ways that depend on compiler flags (that is how 24 fold
 * kernels shipped broken: they happened to work at -O0). Also re-checks the
 * f32/f64 fold tails that summed into a register the main loop clobbered.
 *
 * Win64 additionally requires xmm6-xmm15 to be preserved. Build with
 * `make ASMFLAGS=-DFP_FORCE_XMM_SAVE` and run with FP_TEST_CHECK_XMM=1 to
 * check that contract on Linux (the kernels then emit their Win64 saves). */
#include "fp_core.h"
#include "fp_cpu.h"
#include "fp_dispatch.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0, checks = 0;

#if defined(__x86_64__) && defined(__ELF__)

/* fp_test_abi_probe(fn, a0, a1, a2, uint64_t gpr_out[6], uint64_t xmm_io[20])
 * xmm_io: xmm6..xmm15 are loaded from it before the call and stored back
 * after, so the caller can see whether the kernel preserved them. */
void fp_test_abi_probe(void (*fn)(void), uintptr_t a0, uintptr_t a1, uintptr_t a2,
                       uint64_t* out, uint64_t* xmm_io);
__asm__(
    ".text\n"
    ".globl fp_test_abi_probe\n"
    ".type fp_test_abi_probe,@function\n"
    "fp_test_abi_probe:\n"
    "  push %rbx\n  push %rbp\n  push %r12\n  push %r13\n  push %r14\n  push %r15\n"
    "  push %r8\n  push %r9\n  sub $8, %rsp\n"     /* 16-byte aligned at the call */
    "  movdqu 0(%r9), %xmm6\n    movdqu 16(%r9), %xmm7\n   movdqu 32(%r9), %xmm8\n"
    "  movdqu 48(%r9), %xmm9\n   movdqu 64(%r9), %xmm10\n  movdqu 80(%r9), %xmm11\n"
    "  movdqu 96(%r9), %xmm12\n  movdqu 112(%r9), %xmm13\n movdqu 128(%r9), %xmm14\n"
    "  movdqu 144(%r9), %xmm15\n"
    "  mov %rdi, %rax\n  mov %rsi, %rdi\n  mov %rdx, %rsi\n  mov %rcx, %rdx\n"
    "  movabs $0x1BADB0021BADB001, %rbx\n"
    "  movabs $0x1BADB0021BADB002, %rbp\n"
    "  movabs $0x1BADB0021BADB003, %r12\n"
    "  movabs $0x1BADB0021BADB004, %r13\n"
    "  movabs $0x1BADB0021BADB005, %r14\n"
    "  movabs $0x1BADB0021BADB006, %r15\n"
    "  call *%rax\n"
    "  add $8, %rsp\n  pop %r9\n  pop %r8\n"
    "  mov %rbx, 0(%r8)\n  mov %rbp, 8(%r8)\n  mov %r12, 16(%r8)\n"
    "  mov %r13, 24(%r8)\n  mov %r14, 32(%r8)\n  mov %r15, 40(%r8)\n"
    "  movdqu %xmm6, 0(%r9)\n    movdqu %xmm7, 16(%r9)\n   movdqu %xmm8, 32(%r9)\n"
    "  movdqu %xmm9, 48(%r9)\n   movdqu %xmm10, 64(%r9)\n  movdqu %xmm11, 80(%r9)\n"
    "  movdqu %xmm12, 96(%r9)\n  movdqu %xmm13, 112(%r9)\n movdqu %xmm14, 128(%r9)\n"
    "  movdqu %xmm15, 144(%r9)\n"
    "  pop %r15\n  pop %r14\n  pop %r13\n  pop %r12\n  pop %rbp\n  pop %rbx\n"
    "  ret\n"
    ".size fp_test_abi_probe, .-fp_test_abi_probe\n");

/* X(symbol, arity: 1 = (in, n) | 2 = (a, b, n), tier needed or 0) */
#define KERNELS(X) \
    X(fp_fold_dotp_f32, 2, 0) \
    X(fp_fold_dotp_f64, 2, 0) \
    X(fp_fold_dotp_i16, 2, 0) \
    X(fp_fold_dotp_i32, 2, 0) \
    X(fp_fold_dotp_i64, 2, 0) \
    X(fp_fold_dotp_i8, 2, 0) \
    X(fp_fold_dotp_u16, 2, 0) \
    X(fp_fold_dotp_u32, 2, 0) \
    X(fp_fold_dotp_u64, 2, 0) \
    X(fp_fold_dotp_u8, 2, 0) \
    X(fp_fold_sad_f32, 2, 0) \
    X(fp_fold_sad_i16, 2, 0) \
    X(fp_fold_sad_i32, 2, 0) \
    X(fp_fold_sad_i64, 2, 0) \
    X(fp_fold_sad_i8, 2, 0) \
    X(fp_fold_sad_u16, 2, 0) \
    X(fp_fold_sad_u32, 2, 0) \
    X(fp_fold_sad_u64, 2, 0) \
    X(fp_fold_sad_u8, 2, 0) \
    X(fp_fold_sumsq_f32, 1, 0) \
    X(fp_fold_sumsq_i16, 1, 0) \
    X(fp_fold_sumsq_i32, 1, 0) \
    X(fp_fold_sumsq_i64, 1, 0) \
    X(fp_fold_sumsq_i8, 1, 0) \
    X(fp_fold_sumsq_u16, 1, 0) \
    X(fp_fold_sumsq_u32, 1, 0) \
    X(fp_fold_sumsq_u64, 1, 0) \
    X(fp_fold_sumsq_u8, 1, 0) \
    X(fp_reduce_add_f32, 1, 0) \
    X(fp_reduce_add_f64, 1, 0) \
    X(fp_reduce_add_i16, 1, 0) \
    X(fp_reduce_add_i32, 1, 0) \
    X(fp_reduce_add_i64, 1, 0) \
    X(fp_reduce_add_i8, 1, 0) \
    X(fp_reduce_add_u16, 1, 0) \
    X(fp_reduce_add_u32, 1, 0) \
    X(fp_reduce_add_u64, 1, 0) \
    X(fp_reduce_add_u8, 1, 0) \
    X(fp_reduce_max_f32, 1, 0) \
    X(fp_reduce_max_f64, 1, 0) \
    X(fp_reduce_max_i16, 1, 0) \
    X(fp_reduce_max_i32, 1, 0) \
    X(fp_reduce_max_i64, 1, 0) \
    X(fp_reduce_max_i8, 1, 0) \
    X(fp_reduce_max_u16, 1, 0) \
    X(fp_reduce_max_u32, 1, 0) \
    X(fp_reduce_max_u64, 1, 0) \
    X(fp_reduce_max_u8, 1, 0) \
    X(fp_reduce_min_f32, 1, 0) \
    X(fp_reduce_min_f64, 1, 0) \
    X(fp_reduce_min_i16, 1, 0) \
    X(fp_reduce_min_i32, 1, 0) \
    X(fp_reduce_min_i64, 1, 0) \
    X(fp_reduce_min_i8, 1, 0) \
    X(fp_reduce_min_u16, 1, 0) \
    X(fp_reduce_min_u32, 1, 0) \
    X(fp_reduce_min_u64, 1, 0) \
    X(fp_reduce_min_u8, 1, 0) \
    X(fp_reduce_mul_f32, 1, 0) \
    X(fp_reduce_mul_i16, 1, 0) \
    X(fp_reduce_mul_i32, 1, 0) \
    X(fp_reduce_mul_i8, 1, 0) \
    X(fp_reduce_mul_u16, 1, 0) \
    X(fp_reduce_mul_u32, 1, 0) \
    X(fp_reduce_mul_u64, 1, 0) \
    X(fp_reduce_mul_u8, 1, 0) \
    X(fp_reduce_product_f64, 1, 0) \
    X(fp_reduce_product_i64, 1, 0) \
    X(fp_reduce_add_f32_avx2, 1, FP_TIER_AVX2) \
    X(fp_reduce_add_f64_avx2, 1, FP_TIER_AVX2) \
    X(fp_fold_sumsq_f32_avx2, 1, FP_TIER_AVX2) \
    X(fp_fold_dotp_f32_avx2, 2, FP_TIER_AVX2) \
    X(fp_fold_dotp_f64_avx2, 2, FP_TIER_AVX2) \
    X(fp_fold_dotp_i8_avx2, 2, FP_TIER_AVX2) \
    X(fp_fold_dotp_u8_avx2, 2, FP_TIER_AVX2) \
    X(fp_fold_dotp_i16_avx2, 2, FP_TIER_AVX2) \
    X(fp_fold_dotp_u16_avx2, 2, FP_TIER_AVX2) \
    X(fp_fold_dotp_i8_avxvnni, 2, FP_TIER_AVXVNNI) \
    X(fp_fold_dotp_u8_avxvnni, 2, FP_TIER_AVXVNNI) \
    X(fp_fold_dotp_i16_avxvnni, 2, FP_TIER_AVXVNNI) \
    X(fp_fold_dotp_u16_avxvnni, 2, FP_TIER_AVXVNNI) \
    X(fp_reduce_add_f32_avx512, 1, FP_TIER_AVX512) \
    X(fp_reduce_add_f64_avx512, 1, FP_TIER_AVX512) \
    X(fp_fold_sumsq_f32_avx512, 1, FP_TIER_AVX512) \
    X(fp_fold_dotp_f32_avx512, 2, FP_TIER_AVX512) \
    X(fp_fold_dotp_f64_avx512, 2, FP_TIER_AVX512) \
    X(fp_fold_dotp_i8_avx512, 2, FP_TIER_AVX512) \
    X(fp_fold_dotp_u8_avx512, 2, FP_TIER_AVX512) \
    X(fp_fold_dotp_i16_avx512, 2, FP_TIER_AVX512) \
    X(fp_fold_dotp_u16_avx512, 2, FP_TIER_AVX512) \


/* FP_TEST_CHECK_XMM=1: also require xmm6-15 to survive, i.e. the Win64
 * contract. Meaningful for a library assembled with -DFP_FORCE_XMM_SAVE
 * (`make ASMFLAGS=-DFP_FORCE_XMM_SAVE`); on plain SysV they are volatile. */
static int g_check_xmm;

static void probe(const char* name, void (*fn)(void), int arity, const void* a, const void* b, size_t n) {
    static const char* const reg[6] = { "rbx", "rbp", "r12", "r13", "r14", "r15" };
    uint64_t out[6], xmm[20];
    int i;
    for (i = 0; i < 20; i++) xmm[i] = 0xC0FFEE0000000000ull + (uint64_t)i;
    if (getenv("FP_TEST_VERBOSE")) { fprintf(stderr, "probe %s n=%zu\n", name, n); fflush(stderr); }
    if (arity == 1) fp_test_abi_probe(fn, (uintptr_t)a, (uintptr_t)n, 0, out, xmm);
    else            fp_test_abi_probe(fn, (uintptr_t)a, (uintptr_t)b, (uintptr_t)n, out, xmm);
    if (g_check_xmm)
        for (i = 0; i < 20; i++) {
            checks++;
            if (xmm[i] != 0xC0FFEE0000000000ull + (uint64_t)i) {
                printf("FAIL %-28s n=%-4zu clobbers xmm%d (Win64 callee-saved)\n", name, n, 6 + i / 2);
                failures++;
                i |= 1;   /* one report per register */
            }
        }
    for (i = 0; i < 6; i++) {
        checks++;
        if (out[i] != (0x1BADB0021BADB001ull + (uint64_t)i)) {
            printf("FAIL %-28s n=%-4zu clobbers %s\n", name, n, reg[i]);
            failures++;
        }
    }
}

static int runnable(const char* name, int tier) {
    if (tier == 0) return 1;                      /* public symbol: always runs */
    if (!fp_dispatch_tier_available((fp_tier)tier)) return 0;
    if (tier == FP_TIER_AVX512 && strstr(name, "dotp_") &&
        !strstr(name, "_f32") && !strstr(name, "_f64"))
        return fp_cpu_has(FP_CPU_AVX512_VNNI);    /* integer dots use AVX512-VNNI */
    return 1;
}

static void run_probes(void) {
    static uint64_t a[512], b[512];   /* zeros: valid input for every element type */
    static const size_t ns[] = { 0, 1, 7, 33, 100, 257 };
    size_t k;
    int count = 0;
    for (k = 0; k < sizeof ns / sizeof ns[0]; k++) {
#define RUN(name, ar, tier) \
        if (runnable(#name, tier)) { probe(#name, (void (*)(void))name, ar, a, b, ns[k]); count++; }
        KERNELS(RUN)
#undef RUN
    }
    printf("abi: %d kernel calls probed\n", count);
}
#else
static void run_probes(void) { puts("abi: SKIP (needs x86-64 ELF / System V)"); }
#endif

/* ---- tail regressions: the scalar tail must not clobber the vector sum ---- */
static void tails(void) {
    float  fa[100], fb[100];
    double da[100], db[100];
    size_t n, i;
    for (i = 0; i < 100; i++) {
        fa[i] = (float)(i % 7) - 3.0f; fb[i] = (float)(i % 5) * 0.5f;
        da[i] = (double)fa[i];         db[i] = (double)fb[i];
    }
    for (n = 0; n < 100; n++) {
        double sq = 0, dot = 0, sad = 0;
        for (i = 0; i < n; i++) { sq += fa[i] * fa[i]; dot += fa[i] * fb[i]; sad += fabs(fa[i] - fb[i]); }
        checks += 4;   /* small integers / halves: every variant is exact */
        if (fp_fold_sumsq_f32(fa, n) != (float)sq)  { printf("FAIL sumsq_f32 n=%zu\n", n); failures++; }
        if (fp_fold_dotp_f32(fa, fb, n) != (float)dot) { printf("FAIL dotp_f32 n=%zu\n", n); failures++; }
        if (fp_fold_sad_f32(fa, fb, n) != (float)sad) { printf("FAIL sad_f32 n=%zu\n", n); failures++; }
        if (fp_fold_dotp_f64(da, db, n) != dot)      { printf("FAIL dotp_f64 n=%zu\n", n); failures++; }
    }
    puts("tails: f32 sumsq/dotp/sad and f64 dotp exact for n = 0..99");
}

int main(void) {
    g_check_xmm = getenv("FP_TEST_CHECK_XMM") && strcmp(getenv("FP_TEST_CHECK_XMM"), "0") != 0;
    if (g_check_xmm) puts("abi: also checking xmm6-xmm15 (Win64 contract)");
    run_probes();
    tails();
    printf("\n%s (%d checks, %d failures)\n", failures ? "SOME FAILED" : "ALL PASS", checks, failures);
    return failures != 0;
}
