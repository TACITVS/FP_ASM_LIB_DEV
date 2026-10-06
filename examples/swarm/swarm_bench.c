/* swarm_bench.c — headless "Verdant Swarm" benchmark and self-check.
 *
 *   swarm_bench [--stars N] [--frames F] [--threads 1,4,all]
 *
 * For each thread count, runs the galaxy frame with FP-ASM kernels and with
 * the plain C loops (same compiler flags) and prints the median time of
 * every stage. Before timing it verifies that
 *   - both modes compute the same frame (within float reassociation), and
 *   - results are bit-identical at 1 thread and at the most threads.
 * Exit code 0 = checks passed. */
#include "swarm_sim.h"
#include "fp_core.h"
#include "fp_cpu.h"
#include "fp_dispatch.h"
#include "fp_gfx.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cmp_double(const void* a, const void* b) {
    double x = *(const double*)a, y = *(const double*)b;
    return x < y ? -1 : x > y;
}

static void camera(Mat4* vp) {
    fp_gfx_conventions c;
    Mat4 v, p;
    fp_gfx_conventions_init(&c, FP_GFX_D3D11, 1);
    fp_mat4_lookat_gfx(&v, 0, 9, 22, 0, 0, 0, 0, 1, 0, &c);
    fp_mat4_perspective_gfx(&p, 0.9f, 16.0f / 9.0f, 0.05f, INFINITY, &c);
    fp_mat4_mul(vp, &p, &v);
}

/* frame f of an animation, from a fresh galaxy (so both modes see the same prev) */
static void run_frames(size_t n, swarm_pool* pool, swarm_mode mode, int frames,
                       Vec3f* clip, swarm_stats* st, Vec3f* world_copy) {
    swarm_galaxy g;
    Mat4 vp;
    int f;
    camera(&vp);
    swarm_galaxy_create(&g, n, 7);
    for (f = 0; f < frames; f++) swarm_frame(&g, pool, mode, 0.5 + f / 60.0, &vp, clip, st, NULL);
    if (world_copy) memcpy(world_copy, g.world, n * sizeof(Vec3f));
    swarm_galaxy_destroy(&g);
}

static int verify(size_t n, int maxthreads) {
    swarm_pool *p1 = swarm_pool_create(1), *pn = swarm_pool_create(maxthreads);
    Vec3f *ca = malloc(n * sizeof(Vec3f)), *cc = malloc(n * sizeof(Vec3f)), *cn = malloc(n * sizeof(Vec3f));
    Vec3f *wa = malloc(n * sizeof(Vec3f)), *wc = malloc(n * sizeof(Vec3f)), *wn = malloc(n * sizeof(Vec3f));
    swarm_stats sa, sc, sn;
    double maxw = 0, maxclip = 0;
    size_t i;
    int fails = 0;

    run_frames(n, p1, SWARM_FPASM, 3, ca, &sa, wa);
    run_frames(n, p1, SWARM_PLAIN_C, 3, cc, &sc, wc);
    run_frames(n, pn, SWARM_FPASM, 3, cn, &sn, wn);
    for (i = 0; i < n; i++) {
        double d = fabs(wa[i].x - wc[i].x) + fabs(wa[i].y - wc[i].y) + fabs(wa[i].z - wc[i].z);
        double dc = (fabs(ca[i].x - cc[i].x) + fabs(ca[i].y - cc[i].y) + fabs(ca[i].z - cc[i].z) +
                     fabs(ca[i]._pad - cc[i]._pad)) / (1.0 + fabs(cc[i]._pad));
        if (d > maxw) maxw = d;
        if (dc > maxclip) maxclip = dc;
    }
    printf("check: FP-ASM vs plain C, world positions max |diff| = %.2e", maxw);
    if (maxw > 1e-3) { printf("  FAIL\n"); fails++; } else printf("  ok\n");
    printf("check: FP-ASM vs plain C, clip positions max rel diff = %.2e", maxclip);
    if (maxclip > 1e-4) { printf("  FAIL\n"); fails++; } else printf("  ok\n");
    printf("check: centre of mass  FP-ASM (%.5f %.5f %.5f)  C (%.5f %.5f %.5f)",
           sa.com.x, sa.com.y, sa.com.z, sc.com.x, sc.com.y, sc.com.z);
    if (fabs(sa.com.x - sc.com.x) + fabs(sa.com.y - sc.com.y) + fabs(sa.com.z - sc.com.z) > 1e-3) { printf("  FAIL\n"); fails++; }
    else printf("  ok\n");
    printf("check: spread FP-ASM %.5f  C %.5f", sa.spread, sc.spread);
    if (fabs(sa.spread - sc.spread) > 1e-3 * (1 + sc.spread)) { printf("  FAIL\n"); fails++; } else printf("  ok\n");
    printf("check: energy FP-ASM %.6g  C %.6g", sa.energy, sc.energy);
    if (fabs(sa.energy - sc.energy) > 1e-3 * (sc.energy + 1e-9)) { printf("  FAIL\n"); fails++; } else printf("  ok\n");
    printf("check: 1 thread vs %d threads bit-identical", maxthreads);
    if (memcmp(wa, wn, n * sizeof(Vec3f)) || memcmp(ca, cn, n * sizeof(Vec3f)) || memcmp(&sa, &sn, sizeof sa)) {
        printf("  FAIL\n"); fails++;
    } else printf("  ok\n");
    free(ca); free(cc); free(cn); free(wa); free(wc); free(wn);
    swarm_pool_destroy(p1); swarm_pool_destroy(pn);
    return fails;
}

int main(int argc, char** argv) {
    size_t n = 1u << 20;
    int frames = 60, i, nt = 0, threads[16];
    const char* tlist = NULL;
    const fp_cpu_info_t* ci;

    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--stars") && i + 1 < argc) n = (size_t)strtoull(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc) frames = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--threads") && i + 1 < argc) tlist = argv[++i];
        else { printf("usage: swarm_bench [--stars N] [--frames F] [--threads 1,4,all]\n"); return 2; }
    }
    if (n < 1024) n = 1024;
    if (frames < 3) frames = 3;
    if (!tlist) tlist = "1,all";
    {
        char buf[128], *tok;
        strncpy(buf, tlist, sizeof buf - 1); buf[sizeof buf - 1] = 0;
        for (tok = strtok(buf, ","); tok && nt < 16; tok = strtok(NULL, ","))
            threads[nt++] = !strcmp(tok, "all") ? swarm_cpu_count() : atoi(tok) > 0 ? atoi(tok) : 1;
    }

    fp_dispatch_init();
    ci = fp_cpu_info();
    printf("Verdant Swarm benchmark: %zu stars, %d frames per run\n", n, frames);
    printf("CPU: %s (%s), %d logical CPUs, kernels: %s\n\n", ci->brand, ci->uarch, swarm_cpu_count(),
           fp_tier_name(fp_dispatch_best_tier()));

    if (verify(n < (1u << 18) ? n : (1u << 18), threads[nt - 1] > 1 ? threads[nt - 1] : 4)) {
        printf("\nVERIFICATION FAILED\n");
        return 1;
    }

    printf("\nmedian ms per frame%*s", 2, "");
    for (i = 0; i < SWARM_STAGES; i++) printf(" %11s", swarm_stage_names[i]);
    printf(" %9s\n", "total");
    for (i = 0; i < nt; i++) {
        swarm_pool* pool = swarm_pool_create(threads[i]);
        double med[2][SWARM_STAGES + 1];
        int m, s, f;
        Vec3f* clip = malloc(n * sizeof(Vec3f));
        for (m = 0; m < 2; m++) {
            swarm_galaxy g;
            Mat4 vp;
            double* samples = malloc((size_t)frames * (SWARM_STAGES + 1) * sizeof *samples);
            camera(&vp);
            swarm_galaxy_create(&g, n, 7);
            for (f = 0; f < 5; f++) swarm_frame(&g, pool, (swarm_mode)m, f / 60.0, &vp, clip, NULL, NULL);
            for (f = 0; f < frames; f++) {
                swarm_timing tm;
                double tot = 0;
                swarm_frame(&g, pool, (swarm_mode)m, (5 + f) / 60.0, &vp, clip, NULL, &tm);
                for (s = 0; s < SWARM_STAGES; s++) { samples[s * frames + f] = tm.ms[s]; tot += tm.ms[s]; }
                samples[SWARM_STAGES * frames + f] = tot;
            }
            for (s = 0; s <= SWARM_STAGES; s++) {
                qsort(samples + s * frames, (size_t)frames, sizeof *samples, cmp_double);
                med[m][s] = samples[s * frames + frames / 2];
            }
            free(samples);
            swarm_galaxy_destroy(&g);
        }
        for (m = 0; m < 2; m++) {
            printf("%2d thread%s %-8s", threads[i], threads[i] == 1 ? " " : "s", swarm_mode_names[m]);
            for (s = 0; s <= SWARM_STAGES; s++) printf(" %11.2f", med[m][s]);
            printf("\n");
        }
        printf("%-19s", "   FP-ASM speedup");
        for (s = 0; s <= SWARM_STAGES; s++) printf(" %10.2fx", med[1][s] / med[0][s]);
        printf("\n\n");
        free(clip);
        swarm_pool_destroy(pool);
    }
    return 0;
}
