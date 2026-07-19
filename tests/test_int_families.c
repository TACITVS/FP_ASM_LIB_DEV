/* ABI/correctness smoke test for the pure-integer-arg families:
 * reductions (multiple widths), fused folds, predicates, tier3 ops.
 * Any Win64/SysV ABI mismatch shows up as a wrong result or a crash. */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* reductions */
extern int32_t  fp_reduce_add_i32(const int32_t*, size_t);
extern int32_t  fp_reduce_max_i32(const int32_t*, size_t);
extern int32_t  fp_reduce_min_i32(const int32_t*, size_t);
extern int32_t  fp_reduce_mul_i32(const int32_t*, size_t);
extern uint16_t fp_reduce_add_u16(const uint16_t*, size_t);
extern int8_t   fp_reduce_max_i8(const int8_t*, size_t);
extern uint64_t fp_reduce_add_u64(const uint64_t*, size_t);
extern float    fp_reduce_add_f32(const float*, size_t);
extern float    fp_reduce_max_f32(const float*, size_t);
/* fused folds */
extern int64_t  fp_fold_sumsq_i64(const int64_t*, size_t);
extern int64_t  fp_fold_dotp_i64(const int64_t*, const int64_t*, size_t);
extern double   fp_fold_dotp_f64(const double*, const double*, size_t);
extern int64_t  fp_fold_sad_i64(const int64_t*, const int64_t*, size_t);
/* predicates */
extern bool     fp_pred_all_eq_const_i64(const int64_t*, size_t, int64_t);
extern bool     fp_pred_any_gt_const_i64(const int64_t*, size_t, int64_t);
extern bool     fp_pred_all_gt_zip_i64(const int64_t*, const int64_t*, size_t);
/* tier3 */
extern size_t   fp_count_i64(const int64_t*, size_t, int64_t);
extern size_t   fp_range_i64(int64_t*, int64_t, int64_t);
extern void     fp_iterate_add_i64(int64_t*, size_t, int64_t, int64_t);

static int fails = 0;
#define OKI(name, got, want) do { long long g=(long long)(got), w=(long long)(want); \
    if (g!=w){printf("FAIL %-26s got=%lld want=%lld\n",name,g,w);fails++;} \
    else printf("ok   %-26s = %lld\n",name,g);} while(0)
#define OKF(name, got, want) do { double g=(got),w=(want); \
    if (fabs(g-w)>1e-4*(1+fabs(w))){printf("FAIL %-26s got=%.6g want=%.6g\n",name,g,w);fails++;} \
    else printf("ok   %-26s = %.6g\n",name,g);} while(0)

int main(void){
    enum { N = 512 };
    int32_t i32[N]; uint16_t u16[N]; int8_t i8[N]; uint64_t u64[N];
    float f32[N]; int64_t a[N], b[N];
    int64_t s_sumsq=0, s_dotp=0, s_sad=0;
    int32_t s_add32=0, s_max32=INT32_MIN, s_min32=INT32_MAX, s_mul32=1;
    uint32_t s_addu16=0; int s_maxi8=-128; uint64_t s_addu64=0;
    double s_addf32=0, s_maxf32=-1e30, s_dotpf=0;

    for(int i=0;i<N;i++){
        i32[i]=(i%7)-3; s_add32+=i32[i];
        if(i32[i]>s_max32)s_max32=i32[i];
        if(i32[i]<s_min32)s_min32=i32[i];
        if(i<8) s_mul32*=i32[i];
        u16[i]=(uint16_t)(i%50); s_addu16+=u16[i];
        i8[i]=(int8_t)((i%17)-8); if(i8[i]>s_maxi8)s_maxi8=i8[i];
        u64[i]=(uint64_t)(i*3+1); s_addu64+=u64[i];
        f32[i]=(float)((i%11)-5)*0.5f; s_addf32+=f32[i]; if(f32[i]>s_maxf32)s_maxf32=f32[i];
        a[i]=(i%13)-6; b[i]=(i%5)-2;
        s_sumsq+=a[i]*a[i]; s_dotp+=a[i]*b[i]; s_sad+=llabs(a[i]-b[i]); s_dotpf+=(double)a[i]*b[i];
    }
    /* mul over first 8 only: make rest 1 so full-array product == s_mul32 */
    for(int i=8;i<N;i++) i32[i]=1;

    { long long s=0; for(int i=0;i<N;i++) s+=i32[i]; OKI("reduce_add_i32", fp_reduce_add_i32(i32,N), s); }
    { int32_t mx=INT32_MIN,mn=INT32_MAX; for(int i=0;i<N;i++){if(i32[i]>mx)mx=i32[i];if(i32[i]<mn)mn=i32[i];}
      OKI("reduce_max_i32", fp_reduce_max_i32(i32,N), mx);
      OKI("reduce_min_i32", fp_reduce_min_i32(i32,N), mn); }
    OKI("reduce_mul_i32", fp_reduce_mul_i32(i32,N), s_mul32);
    OKI("reduce_add_u16", fp_reduce_add_u16(u16,N), (uint16_t)s_addu16);
    OKI("reduce_max_i8",  fp_reduce_max_i8(i8,N),  s_maxi8);
    OKI("reduce_add_u64", fp_reduce_add_u64(u64,N), (int64_t)s_addu64);
    OKF("reduce_add_f32", fp_reduce_add_f32(f32,N), s_addf32);
    OKF("reduce_max_f32", fp_reduce_max_f32(f32,N), s_maxf32);

    OKI("fold_sumsq_i64", fp_fold_sumsq_i64(a,N), s_sumsq);
    OKI("fold_dotp_i64",  fp_fold_dotp_i64(a,b,N), s_dotp);
    OKI("fold_sad_i64",   fp_fold_sad_i64(a,b,N), s_sad);
    OKF("fold_dotp_f64",  fp_fold_dotp_f64((double[]){1,2,3,4},(double[]){5,6,7,8},4), 70.0);

    OKI("pred_all_eq(false)", fp_pred_all_eq_const_i64(a,N,0), 0);
    { int64_t c[4]={7,7,7,7}; OKI("pred_all_eq(true)", fp_pred_all_eq_const_i64(c,4,7), 1); }
    OKI("pred_any_gt_const", fp_pred_any_gt_const_i64(a,N,5), 1);
    { int64_t x[3]={5,6,7},y[3]={1,2,3}; OKI("pred_all_gt_zip", fp_pred_all_gt_zip_i64(x,y,3), 1); }

    { long long cnt=0; for(int i=0;i<N;i++) if(a[i]==3) cnt++;
      OKI("count_i64", fp_count_i64(a,N,3), cnt); }
    { int64_t out[20]; size_t m=fp_range_i64(out,10,20);
      OKI("range_i64 len", m, 10); OKI("range_i64 first", out[0], 10); OKI("range_i64 last", out[9], 19); }
    { int64_t out[10]; fp_iterate_add_i64(out,10,100,5);
      OKI("iterate_add first", out[0], 100); OKI("iterate_add last", out[9], 145); }

    printf("\n%s (%d failure%s)\n", fails?"FAILED":"ALL PASS", fails, fails==1?"":"s");
    return fails?1:0;
}
