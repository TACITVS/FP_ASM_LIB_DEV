/* Validates the i64/f64 fused-map kernels: scale, offset, axpy, zip_add.
 * Exercises ABI_ARGS_INT (int scale/offset/zip), ABI_ARGS_INT3_F1 (float
 * scale/offset via xmm), and the 5th-arg scalar path (axpy: int on stack/r8,
 * float on stack/xmm0).
 *
 * NOTE: the narrow-width variants (i8/i16/i32, unsigned, f32) are intentionally NOT
 * tested here yet -- they carry a separate pre-existing callee-saved-register
 * preservation bug and are ported in a dedicated follow-up. */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>

extern void fp_map_scale_i64(const int64_t*, int64_t*, size_t, int64_t);
extern void fp_map_offset_i64(const int64_t*, int64_t*, size_t, int64_t);
extern void fp_map_axpy_i64(const int64_t*, const int64_t*, int64_t*, size_t, int64_t);
extern void fp_zip_add_i64(const int64_t*, const int64_t*, int64_t*, size_t);

extern void fp_map_scale_f64(const double*, double*, size_t, double);
extern void fp_map_offset_f64(const double*, double*, size_t, double);
extern void fp_map_axpy_f64(const double*, const double*, double*, size_t, double);
extern void fp_zip_add_f64(const double*, const double*, double*, size_t);

static int fails = 0;
static void okd(const char* t, double err){
    if (fabs(err) > 1e-9) { printf("FAIL %-14s total-abs-err=%.6g\n", t, err); fails++; }
    else printf("ok   %-14s\n", t);
}

int main(void){
    enum { N = 37 };  /* odd length hits the scalar remainder path */
    int64_t xl[N], yl[N], ol[N];
    double  xd[N], yd[N], od[N];
    for (int i=0;i<N;i++){
        xl[i]=(int64_t)(i-15); yl[i]=(i%5)+1;
        xd[i]=(double)(i-15)*0.25; yd[i]=(double)((i%5)+1);
    }
    double e;
    fp_map_scale_i64(xl,ol,N,3);   e=0; for(int i=0;i<N;i++) e+=fabs((double)(ol[i]-3*xl[i]));            okd("scale_i64", e);
    fp_map_offset_i64(xl,ol,N,7);  e=0; for(int i=0;i<N;i++) e+=fabs((double)(ol[i]-(xl[i]+7)));          okd("offset_i64", e);
    fp_map_axpy_i64(xl,yl,ol,N,4); e=0; for(int i=0;i<N;i++) e+=fabs((double)(ol[i]-(4*xl[i]+yl[i])));    okd("axpy_i64", e);
    fp_zip_add_i64(xl,yl,ol,N);    e=0; for(int i=0;i<N;i++) e+=fabs((double)(ol[i]-(xl[i]+yl[i])));      okd("zip_add_i64", e);
    fp_map_scale_f64(xd,od,N,2.5); e=0; for(int i=0;i<N;i++) e+=fabs(od[i]-2.5*xd[i]);                    okd("scale_f64", e);
    fp_map_offset_f64(xd,od,N,1.5);e=0; for(int i=0;i<N;i++) e+=fabs(od[i]-(xd[i]+1.5));                  okd("offset_f64", e);
    fp_map_axpy_f64(xd,yd,od,N,3.0);e=0;for(int i=0;i<N;i++) e+=fabs(od[i]-(3.0*xd[i]+yd[i]));            okd("axpy_f64", e);
    fp_zip_add_f64(xd,yd,od,N);    e=0; for(int i=0;i<N;i++) e+=fabs(od[i]-(xd[i]+yd[i]));                okd("zip_add_f64", e);

    printf("\n%s (%d failure%s)\n", fails?"FAILED":"ALL PASS", fails, fails==1?"":"s");
    return fails?1:0;
}
