/* Validates simple maps: abs (i64/f64), sqrt (f64), clamp (i64/f64).
 * clamp_i64 has 5 int args (max on stack/r8); clamp_f64 has 3 int + 2 float
 * args (min in xmm3/xmm0, max on stack/xmm1). */
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <math.h>

extern void fp_map_abs_i64(const int64_t*, int64_t*, size_t);
extern void fp_map_abs_f64(const double*, double*, size_t);
extern void fp_map_sqrt_f64(const double*, double*, size_t);
extern void fp_map_clamp_i64(const int64_t*, int64_t*, size_t, int64_t, int64_t);
extern void fp_map_clamp_f64(const double*, double*, size_t, double, double);

static int fails=0;
static void ok(const char* t, int p){ if(p) printf("ok   %s\n",t); else {printf("FAIL %s\n",t); fails++;} }

int main(void){
    volatile long guard = 0;   /* force compiler to keep live values across calls */
    for (size_t N=1; N<=40; N++){
        int64_t xi[40], oi[40]; double xd[40], od[40];
        for(size_t i=0;i<N;i++){ xi[i]=(int64_t)i-20; xd[i]=(double)i*0.5-10.0; }

        fp_map_abs_i64(xi,oi,N);   for(size_t i=0;i<N;i++){ if(oi[i]!=(xi[i]<0?-xi[i]:xi[i])){ok("abs_i64",0);goto end;} }
        fp_map_abs_f64(xd,od,N);   for(size_t i=0;i<N;i++){ if(fabs(od[i]-fabs(xd[i]))>1e-12){ok("abs_f64",0);goto end;} }
        for(size_t i=0;i<N;i++) xd[i]=(double)i+1.0;
        fp_map_sqrt_f64(xd,od,N);  for(size_t i=0;i<N;i++){ if(fabs(od[i]-sqrt(xd[i]))>1e-9){ok("sqrt_f64",0);goto end;} }
        for(size_t i=0;i<N;i++){ xi[i]=(int64_t)i-20; xd[i]=(double)i-20.0; }
        fp_map_clamp_i64(xi,oi,N,-5,5);  for(size_t i=0;i<N;i++){ int64_t w=xi[i]<-5?-5:(xi[i]>5?5:xi[i]); if(oi[i]!=w){ok("clamp_i64",0);goto end;} }
        fp_map_clamp_f64(xd,od,N,-5.0,5.0); for(size_t i=0;i<N;i++){ double w=xd[i]<-5?-5:(xd[i]>5?5:xd[i]); if(fabs(od[i]-w)>1e-12){ok("clamp_f64",0);goto end;} }
        guard += oi[0];
    }
    ok("abs_i64 (N=1..40)",1); ok("abs_f64 (N=1..40)",1); ok("sqrt_f64 (N=1..40)",1);
    ok("clamp_i64 (N=1..40, 5-arg)",1); ok("clamp_f64 (N=1..40, 3int+2float)",1);
end:
    printf("\n%s (%d failure%s) guard=%ld\n", fails?"FAILED":"ALL PASS", fails, fails==1?"":"s", guard);
    return fails?1:0;
}
