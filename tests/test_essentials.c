/* Validates the essentials family: contains/find/take/drop/slice/reverse/
 * concat/product/replicate. slice and concat exercise the 5th-arg (stack on
 * Win64 / r8 on SysV) path. */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

extern bool    fp_contains_i64(const int64_t*, size_t, int64_t);
extern int64_t fp_find_index_i64(const int64_t*, size_t, int64_t);
extern size_t  fp_take_n_i64(const int64_t*, int64_t*, size_t, size_t);
extern size_t  fp_drop_n_i64(const int64_t*, int64_t*, size_t, size_t);
extern size_t  fp_slice_i64(const int64_t*, int64_t*, size_t, size_t, size_t);
extern void    fp_reverse_i64(const int64_t*, int64_t*, size_t);
extern size_t  fp_concat_i64(const int64_t*, const int64_t*, int64_t*, size_t, size_t);
extern int64_t fp_reduce_product_i64(const int64_t*, size_t);
extern double  fp_reduce_product_f64(const double*, size_t);
extern void    fp_replicate_i64(int64_t*, size_t, int64_t);

static int fails = 0;
static void ok(const char* t, int pass){ if(pass) printf("ok   %s\n",t); else {printf("FAIL %s\n",t); fails++;} }

int main(void){
    int64_t a[10]; for(int i=0;i<10;i++) a[i]=(i+1)*10;   /* 10,20,...,100 */
    int64_t out[20];

    ok("contains(true)",  fp_contains_i64(a,10,30)==true);
    ok("contains(false)", fp_contains_i64(a,10,35)==false);
    ok("find_index",      fp_find_index_i64(a,10,70)==6);

    size_t k = fp_take_n_i64(a,out,10,3);
    ok("take_n len",  k==3);
    ok("take_n data", out[0]==10&&out[1]==20&&out[2]==30);

    size_t d = fp_drop_n_i64(a,out,10,7);
    ok("drop_n len",  d==3);
    ok("drop_n data", out[0]==80&&out[1]==90&&out[2]==100);

    /* slice [2,6) -> 30,40,50,60 : 5-arg (end on stack/r8) */
    size_t sl = fp_slice_i64(a,out,10,2,6);
    ok("slice len",  sl==4);
    ok("slice data", out[0]==30&&out[1]==40&&out[2]==50&&out[3]==60);

    fp_reverse_i64(a,out,5);
    ok("reverse", out[0]==50&&out[1]==40&&out[2]==30&&out[3]==20&&out[4]==10);

    /* concat: 5-arg (len_b on stack/r8) */
    int64_t b[3]={7,8,9}, c[3]={1,2,3};
    size_t tot = fp_concat_i64(b,c,out,3,3);
    ok("concat len",  tot==6);
    ok("concat data", out[0]==7&&out[1]==8&&out[2]==9&&out[3]==1&&out[4]==2&&out[5]==3);

    int64_t p[5]={1,2,3,4,5};
    ok("product_i64", fp_reduce_product_i64(p,5)==120);
    double pf[4]={1.5,2.0,4.0,0.5};
    ok("product_f64", fabs(fp_reduce_product_f64(pf,4)-6.0)<1e-9);

    fp_replicate_i64(out,5,42);
    ok("replicate", out[0]==42&&out[1]==42&&out[2]==42&&out[3]==42&&out[4]==42);

    printf("\n%s (%d failure%s)\n", fails?"FAILED":"ALL PASS", fails, fails==1?"":"s");
    return fails?1:0;
}
