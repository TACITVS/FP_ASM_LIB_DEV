/* Validates the extended FP operations in fp_functional.h against their
 * Haskell-equivalent results. */
#include "fp_functional.h"
#include "fp_monads.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

static int fails = 0;
static void ok(const char* t, int p){ if(p) printf("ok   %s\n",t); else {printf("FAIL %s\n",t); fails++;} }
static int eqa(const int64_t* a, const int64_t* b, size_t n){ for(size_t i=0;i<n;i++) if(a[i]!=b[i]) return 0; return 1; }

/* callbacks */
static int64_t sub_xa(int64_t x, int64_t acc, void* c){ (void)c; return x - acc; }
static int64_t add_ax(int64_t acc, int64_t x, void* c){ (void)c; return acc + x; }
static int64_t add_xa(int64_t x, int64_t acc, void* c){ (void)c; return x + acc; }
static bool lt4(int64_t x, void* c){ (void)c; return x < 4; }
static bool is_even(int64_t x, void* c){ (void)c; return (x % 2) == 0; }
static bool gt0(int64_t x, void* c){ (void)c; return x > 0; }
static bool eq3(int64_t x, void* c){ (void)c; return x == 3; }
static bool eq9(int64_t x, void* c){ (void)c; return x == 9; }
static bool gt3(int64_t x, void* c){ (void)c; return x > 3; }
static int64_t times2(int64_t x, void* c){ (void)c; return x * 2; }
static size_t dup(int64_t x, int64_t* out, size_t cap, void* c){ (void)c; if(cap<2) return 0; out[0]=x; out[1]=x; return 2; }
static bool countdown(int64_t* st, int64_t* out, void* c){ (void)c; if(*st>0){ *out=*st; (*st)--; return true; } return false; }
static Maybe even_half(int64_t x){ return (x%2==0) ? fp_just_i64(x/2) : fp_nothing(); }
static double add_ax_f(double acc, double x, void* c){ (void)c; return acc + x; }
static bool   lt3_f(double x, void* c){ (void)c; return x < 3.0; }
/* milestone A */
static int64_t add2(int64_t a, int64_t b, void* c){ (void)c; return a + b; }
static int64_t max2(int64_t a, int64_t b, void* c){ (void)c; return a > b ? a : b; }
static int64_t sub_xa2(int64_t x, int64_t acc, void* c){ (void)c; return x - acc; }
static int64_t runsum(int64_t acc, int64_t x, int64_t* out, void* c){ (void)c; *out = acc + x; return acc + x; }
static int64_t sum3(int64_t a, int64_t b, int64_t d, void* c){ (void)c; return a + b + d; }
/* milestone B */
static int cmp_asc(int64_t a, int64_t b, void* c){ (void)c; return (a<b)?-1:((a>b)?1:0); }
static int cmp_mod10(int64_t a, int64_t b, void* c){ (void)c; return (int)((a%10)-(b%10)); }
static bool beq(int64_t a, int64_t b, void* c){ (void)c; return a==b; }

int main(void){
    int64_t a[] = {1,2,3,4};
    int64_t out[16], tk[16], dr[16];

    ok("foldr (-) 0 [1,2,3,4] = -2", fp_foldr_i64(a,4,0,sub_xa,NULL) == -2);

    fp_scanl_i64(a,out,4,0,add_ax,NULL);
    ok("scanl (+) 0 -> [1,3,6,10]", eqa(out,(int64_t[]){1,3,6,10},4));
    fp_scanr_i64(a,out,4,0,add_xa,NULL);
    ok("scanr (+) 0 -> [10,9,7,4]", eqa(out,(int64_t[]){10,9,7,4},4));

    int64_t b[] = {1,2,3,4,5,1};
    size_t k = fp_take_while_i64(b,out,6,lt4,NULL);
    ok("takeWhile (<4) -> [1,2,3]", k==3 && eqa(out,(int64_t[]){1,2,3},3));
    k = fp_drop_while_i64(b,out,6,lt4,NULL);
    ok("dropWhile (<4) -> [4,5,1]", k==3 && eqa(out,(int64_t[]){4,5,1},3));

    int64_t c[] = {1,2,3,4,5};
    size_t split = fp_span_i64(c,tk,dr,5,lt4,NULL);
    ok("span (<4) split=3", split==3 && eqa(tk,(int64_t[]){1,2,3},3) && eqa(dr,(int64_t[]){4,5},2));

    int64_t d[] = {1,2,3,4,5,6};
    size_t ny = fp_partition_i64(d,tk,dr,6,is_even,NULL);
    ok("partition even -> yes[2,4,6] no[1,3,5]", ny==3 && eqa(tk,(int64_t[]){2,4,6},3) && eqa(dr,(int64_t[]){1,3,5},3));

    ok("all (>0) [1,2,3]",       fp_all_i64((int64_t[]){1,2,3},3,gt0,NULL)==true);
    ok("all (>0) [1,-2,3] false", fp_all_i64((int64_t[]){1,-2,3},3,gt0,NULL)==false);
    ok("any (==3) [1,2,3]",      fp_any_i64((int64_t[]){1,2,3},3,eq3,NULL)==true);
    ok("any (==9) false",        fp_any_i64((int64_t[]){1,2,3},3,eq9,NULL)==false);
    ok("countIf even [1..8]=4",  fp_count_if_i64((int64_t[]){1,2,3,4,5,6,7,8},8,is_even,NULL)==4);

    Maybe f = fp_find_i64(c,5,gt3,NULL);
    ok("find (>3) = Just 4", fp_is_just(f) && fp_from_maybe_i64(f,0)==4);
    ok("find (>3) [1,2,3] = Nothing", fp_is_nothing(fp_find_i64((int64_t[]){1,2,3},3,gt3,NULL)));

    int64_t scratch[4];
    size_t tot = fp_concat_map_i64((int64_t[]){1,2,3},3,out,16,scratch,4,dup,NULL);
    ok("concatMap dup [1,2,3] -> [1,1,2,2,3,3]", tot==6 && eqa(out,(int64_t[]){1,1,2,2,3,3},6));

    fp_iterate_i64(out,5,1,times2,NULL);
    ok("iterate (*2) 1 (5) -> [1,2,4,8,16]", eqa(out,(int64_t[]){1,2,4,8,16},5));

    size_t u = fp_unfoldr_i64(out,16,5,countdown,NULL);
    ok("unfoldr countdown 5 -> [5,4,3,2,1]", u==5 && eqa(out,(int64_t[]){5,4,3,2,1},5));

    size_t m = fp_map_maybe_i64(d,6,even_half,out);   /* from fp_monads.h */
    ok("mapMaybe even/2 [1..6] -> [1,2,3]", m==3 && eqa(out,(int64_t[]){1,2,3},3));

    /* f64 spot-checks */
    double df[] = {1,2,3,4}, dout[8];
    fp_scanl_f64(df,dout,4,0.0,add_ax_f,NULL);
    ok("scanl_f64 (+) 0 -> [1,3,6,10]", dout[0]==1 && dout[1]==3 && dout[2]==6 && dout[3]==10);
    size_t dk = fp_take_while_f64(df,dout,4,lt3_f,NULL);
    ok("takeWhile_f64 (<3) -> [1,2]", dk==2 && dout[0]==1 && dout[1]==2);

    /* ---- Milestone A: foldl1/foldr1, scanl1, mapAccumL, zipWith3 ---- */
    ok("foldl1 (+) [1,2,3,4] = 10", fp_foldl1_i64(a,4,add2,NULL) == 10);
    ok("foldl1 max [3,1,4,1,5] = 5", fp_foldl1_i64((int64_t[]){3,1,4,1,5},5,max2,NULL) == 5);
    ok("foldr1 (-) [1,2,3,4] = -2", fp_foldr1_i64(a,4,sub_xa2,NULL) == -2);
    fp_scanl1_i64(a,out,4,add2,NULL);
    ok("scanl1 (+) -> [1,3,6,10]", eqa(out,(int64_t[]){1,3,6,10},4));
    fp_scanl1_i64((int64_t[]){3,1,4,1,5},out,5,max2,NULL);
    ok("scanl1 max -> [3,3,4,4,5]", eqa(out,(int64_t[]){3,3,4,4,5},5));
    int64_t facc = fp_mapAccumL_i64(a,out,4,0,runsum,NULL);
    ok("mapAccumL running-sum out+acc", eqa(out,(int64_t[]){1,3,6,10},4) && facc==10);
    fp_zipWith3_i64((int64_t[]){1,2,3},(int64_t[]){10,20,30},(int64_t[]){100,200,300},out,3,sum3,NULL);
    ok("zipWith3 (+) -> [111,222,333]", eqa(out,(int64_t[]){111,222,333},3));

    /* ---- Milestone B: sortBy, groupBy, nubBy, intersperse, transpose ---- */
    { int64_t s[] = {3,1,4,1,5,9,2,6}; fp_sort_by_i64(s,8,cmp_asc,NULL);
      ok("sortBy asc", eqa(s,(int64_t[]){1,1,2,3,4,5,6,9},8)); }
    { int64_t s[] = {21,13,11,23}; fp_sort_by_i64(s,4,cmp_mod10,NULL);  /* keys 1,3,1,3 */
      ok("sortBy stable (ties keep order)", eqa(s,(int64_t[]){21,11,13,23},4)); }
    { int64_t g[] = {1,1,2,3,3,3,4}; int64_t of[8]; size_t lens[8];
      size_t ng = fp_group_by_i64(g,7,of,lens,beq,NULL);
      ok("groupBy consecutive -> 4 groups", ng==4 && lens[0]==2 && lens[1]==1 && lens[2]==3 && lens[3]==1); }
    { int64_t nb[] = {1,2,1,3,2,4}; int64_t o2[6]; size_t m2 = fp_nub_by_i64(nb,6,o2,beq,NULL);
      ok("nubBy -> [1,2,3,4]", m2==4 && eqa(o2,(int64_t[]){1,2,3,4},4)); }
    { int64_t o3[8]; size_t m3 = fp_intersperse_i64((int64_t[]){1,2,3},3,0,o3);
      ok("intersperse 0 -> [1,0,2,0,3]", m3==5 && eqa(o3,(int64_t[]){1,0,2,0,3},5)); }
    { int64_t o4[6]; fp_transpose_i64((int64_t[]){1,2,3,4,5,6},2,3,o4);   /* 2x3 -> 3x2 */
      ok("transpose 2x3 -> [1,4,2,5,3,6]", eqa(o4,(int64_t[]){1,4,2,5,3,6},6)); }

    printf("\n%s (%d failure%s)\n", fails?"FAILED":"ALL PASS", fails, fails==1?"":"s");
    return fails?1:0;
}
