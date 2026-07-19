/* bench_math.c — honest micro-benchmarks of the FP-ASM kernels against a
 * scalar C reference compiled at the SAME optimization level (-O3
 * -march=native), so the scalar path is autovectorized by the compiler too.
 *
 * The reference implements the standard per-element algorithms a header-only
 * math library (e.g. raylib's raymath.h) uses: a Vector3Transform / dot /
 * multiply-add applied one element at a time inside a loop.
 *
 * Emits a JSON array of {name, n, ns_ref, ns_asm, speedup} to stdout.
 */
#include "fp_types.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

/* ---- library kernels under test ---- */
extern void  fp_mat4_mul_vec3_batch(Vec3f* out, const Mat4* m, const Vec3f* in, int count);
extern void  fp_map_axpy_f32(const float*, const float*, float*, size_t, float);
extern float fp_reduce_add_f32(const float*, size_t);
extern float fp_fold_dotp_f32(const float*, const float*, size_t);
extern void  fp_map_scale_f32(const float*, float*, size_t, float);

/* ---- scalar reference (the "per-element math library" approach) ---- */
static void ref_transform_batch(Vec3f* out, const Mat4* m, const Vec3f* in, int count){
    const float* M = m->m;
    for (int i=0;i<count;i++){
        float x=in[i].x, y=in[i].y, z=in[i].z;    /* raymath Vector3Transform */
        out[i].x = M[0]*x + M[4]*y + M[8]*z  + M[12];
        out[i].y = M[1]*x + M[5]*y + M[9]*z  + M[13];
        out[i].z = M[2]*x + M[6]*y + M[10]*z + M[14];
    }
}
static void ref_axpy(const float* x, const float* y, float* o, size_t n, float a){
    for (size_t i=0;i<n;i++) o[i] = a*x[i] + y[i];
}
static float ref_reduce(const float* a, size_t n){ float s=0; for(size_t i=0;i<n;i++) s+=a[i]; return s; }
static float ref_dot(const float* a, const float* b, size_t n){ float s=0; for(size_t i=0;i<n;i++) s+=a[i]*b[i]; return s; }
static void  ref_scale(const float* a, float* o, size_t n, float s){ for(size_t i=0;i<n;i++) o[i]=a[i]*s; }

/* ---- timing helpers ---- */
static double now_ns(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec*1e9 + t.tv_nsec; }
static volatile float g_sink;   /* defeat dead-code elimination */

#define REPS 200
/* run `body` REPS times, return best (min) elapsed ns for the whole call */
#define BEST(body) ({ double best=1e30; for(int r=0;r<REPS;r++){ double t0=now_ns(); body; double dt=now_ns()-t0; if(dt<best)best=dt; } best; })

int main(void){
    enum { N = 100000 };          /* 100k elements / vertices */
    Vec3f *vin  = malloc(N*sizeof(Vec3f)), *vout = malloc(N*sizeof(Vec3f));
    float *a = malloc(N*sizeof(float)), *b = malloc(N*sizeof(float)), *o = malloc(N*sizeof(float));
    for (int i=0;i<N;i++){ vin[i]=(Vec3f){(float)(i%97)*0.3f,(float)(i%53)*0.7f,(float)(i%31)*1.1f,0};
                           a[i]=(float)(i%1000)*0.01f; b[i]=(float)(i%777)*0.013f; }
    Mat4 m; for(int i=0;i<16;i++) m.m[i]=(float)((i*7)%13)*0.1f + (i%5==0?1.0f:0.0f);

    struct { const char* name; int n; double ref, asm_; } R[8]; int nb=0;
    double t;

    t = BEST( ref_transform_batch(vout,&m,vin,N) );  double b1=t;
    g_sink = vout[N-1].x;
    t = BEST( fp_mat4_mul_vec3_batch(vout,&m,vin,N) ); double a1=t;
    g_sink = vout[N-1].x;
    R[nb++] = (typeof(R[0])){"mat4 x vec3 (batch transform)", N, b1, a1};

    t = BEST( ref_axpy(a,b,o,N,2.5f) ); double b2=t; g_sink=o[N-1];
    t = BEST( fp_map_axpy_f32(a,b,o,N,2.5f) ); double a2=t; g_sink=o[N-1];
    R[nb++] = (typeof(R[0])){"axpy  o=2.5*x+y (f32)", N, b2, a2};

    t = BEST( g_sink = ref_reduce(a,N) ); double b3=t;
    t = BEST( g_sink = fp_reduce_add_f32(a,N) ); double a3=t;
    R[nb++] = (typeof(R[0])){"reduce sum (f32)", N, b3, a3};

    t = BEST( g_sink = ref_dot(a,b,N) ); double b4=t;
    t = BEST( g_sink = fp_fold_dotp_f32(a,b,N) ); double a4=t;
    R[nb++] = (typeof(R[0])){"dot product (f32)", N, b4, a4};

    t = BEST( ref_scale(a,o,N,3.0f) ); double b5=t; g_sink=o[N-1];
    t = BEST( fp_map_scale_f32(a,o,N,3.0f) ); double a5=t; g_sink=o[N-1];
    R[nb++] = (typeof(R[0])){"scale o=3*x (f32)", N, b5, a5};

    /* JSON out */
    printf("[\n");
    for (int i=0;i<nb;i++){
        printf("  {\"name\":\"%s\",\"n\":%d,\"ns_ref\":%.1f,\"ns_asm\":%.1f,\"speedup\":%.2f}%s\n",
               R[i].name, R[i].n, R[i].ref, R[i].asm_, R[i].ref/R[i].asm_, i<nb-1?",":"");
    }
    printf("]\n");
    /* human summary to stderr */
    fprintf(stderr, "\n%-34s %12s %12s %8s\n", "benchmark", "scalar ns", "asm ns", "speedup");
    for (int i=0;i<nb;i++)
        fprintf(stderr, "%-34s %12.0f %12.0f %7.2fx\n", R[i].name, R[i].ref, R[i].asm_, R[i].ref/R[i].asm_);
    free(vin);free(vout);free(a);free(b);free(o);
    return 0;
}
