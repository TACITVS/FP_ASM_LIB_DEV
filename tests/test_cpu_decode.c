/* Decodes recorded CPUID dumps and checks the feature / tier conclusions.
 * The Raptor Lake values are the raw CPUID of an i7-13700HX (HP Victus,
 * CPU-Z 3.01 "Thread dumps", P-core thread 0) — the development laptop —
 * so the detector is verified against that machine even when the tests run
 * somewhere else. */
#include "fp_cpu.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); failures++; } \
                         else printf("ok   %s\n", #cond); } while (0)
#define HAS(ci, f) (((ci).features & FP_CPU_BIT(FP_CPU_##f)) != 0)

/* i7-13700HX: Raptor Lake-HX, CPUID 6.BF.2, 8 P + 8 E cores, Windows + VBS. */
static const fp_cpuid_leaf raptor_lake_13700hx[] = {
    { 0x00000000, 0, 0x00000020, 0x756E6547, 0x6C65746E, 0x49656E69 },
    { 0x00000001, 0, 0x000B06F2, 0x00800800, 0xFFFAF38B, 0xBFCBFBFF },
    { 0x00000007, 0, 0x00000002, 0x239C27A9, 0x184007A4, 0xBC18C410 },
    { 0x00000007, 1, 0x00400810, 0x00000000, 0x00000000, 0x00000000 },
    { 0x00000007, 2, 0x00000000, 0x00000000, 0x00000000, 0x00000011 },
    { 0x0000000D, 0, 0x00000007, 0x00000340, 0x00000340, 0x00000000 },
    { 0x0000001A, 0, 0x40000001, 0x00000000, 0x00000000, 0x00000000 },
    { 0x80000000, 0, 0x80000008, 0x00000000, 0x00000000, 0x00000000 },
    { 0x80000001, 0, 0x00000000, 0x00000000, 0x00000121, 0x2C100800 },
    { 0x80000002, 0, 0x68743331, 0x6E654720, 0x746E4920, 0x52286C65 },
    { 0x80000003, 0, 0x6F432029, 0x54286572, 0x6920294D, 0x33312D37 },
    { 0x80000004, 0, 0x48303037, 0x00000058, 0x00000000, 0x00000000 },
};

/* Haswell i7-4600M-like: AVX2/FMA, no AVX-VNNI, no AVX-512 (synthetic,
 * built from the documented feature bits). */
static const fp_cpuid_leaf haswell[] = {
    { 0x00000000, 0, 0x0000000D, 0x756E6547, 0x6C65746E, 0x49656E69 },
    { 0x00000001, 0, 0x00040651, 0x00000800, 0x7FDAFBBF, 0xBFEBFBFF },
    { 0x00000007, 0, 0x00000000, 0x000027AB, 0x00000000, 0x00000000 },
    { 0x80000000, 0, 0x80000008, 0, 0, 0 },
    { 0x80000001, 0, 0x00000000, 0x00000000, 0x00000021, 0x2C100800 },
};

int main(void) {
    fp_cpu_info_t ci;

    puts("-- i7-13700HX (Raptor Lake), XCR0 = 0x7 (x87/SSE/AVX) --");
    CHECK(fp_cpu_decode(raptor_lake_13700hx, sizeof raptor_lake_13700hx / sizeof raptor_lake_13700hx[0],
                        0x7, &ci) == 0);
    CHECK(strcmp(ci.vendor, "GenuineIntel") == 0);
    CHECK(strstr(ci.brand, "i7-13700HX") != NULL);
    CHECK(ci.family == 6 && ci.model == 0xBF && ci.stepping == 2);
    CHECK(strstr(ci.uarch, "Raptor Lake") != NULL);
    CHECK(ci.hybrid == 1);
    CHECK(ci.hypervisor == 1);              /* Windows VBS / Hyper-V */
    CHECK(ci.isa_level == 3);
    CHECK(HAS(ci, AVX2) && HAS(ci, FMA) && HAS(ci, BMI2) && HAS(ci, F16C));
    CHECK(HAS(ci, AVX_VNNI));               /* the Raptor Lake tier */
    CHECK(HAS(ci, SHA) && HAS(ci, GFNI) && HAS(ci, VAES) && HAS(ci, VPCLMULQDQ));
    CHECK(!HAS(ci, AVX512F));               /* fused off on hybrid parts */
    CHECK(!(ci.cpuid_features & FP_CPU_BIT(FP_CPU_AVX512F)));
    CHECK(!HAS(ci, AVX_VNNI_INT8) && !HAS(ci, AVX10));

    puts("-- same CPU, OS without AVX state (XCR0 = 0x3) --");
    fp_cpu_decode(raptor_lake_13700hx, sizeof raptor_lake_13700hx / sizeof raptor_lake_13700hx[0], 0x3, &ci);
    CHECK(!HAS(ci, AVX2) && !HAS(ci, AVX_VNNI));
    CHECK(ci.cpuid_features & FP_CPU_BIT(FP_CPU_AVX2));
    CHECK(ci.isa_level == 2);

    puts("-- Haswell --");
    fp_cpu_decode(haswell, sizeof haswell / sizeof haswell[0], 0x7, &ci);
    CHECK(strstr(ci.uarch, "Haswell") != NULL);
    CHECK(ci.isa_level == 3);
    CHECK(HAS(ci, AVX2) && HAS(ci, FMA));
    CHECK(!HAS(ci, AVX_VNNI) && !HAS(ci, AVX512F) && !ci.hybrid);

    puts("-- live CPU --");
    CHECK(fp_cpu_info()->isa_level >= 1);
    printf("     %s / %s, x86-64-v%d\n", fp_cpu_info()->brand, fp_cpu_info()->uarch, fp_cpu_info()->isa_level);

    printf("\n%s (%d failures)\n", failures ? "SOME FAILED" : "ALL PASS", failures);
    return failures != 0;
}
