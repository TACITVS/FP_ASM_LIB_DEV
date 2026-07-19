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
/* milestone C */
static Maybe  nz(int64_t x){ return x==0 ? fp_nothing() : fp_just_i64(x); }
static Either inrange(int64_t x){ return (x>=0 && x<100) ? fp_right_i64(x) : fp_left("out of range", 1); }
static int64_t iabs(int64_t x){ return x<0 ? -x : x; }
static int64_t imax1(int64_t a, int64_t b){ return a>b ? a : b; }
static int64_t isq(int64_t x){ return x*x; }
static int64_t iadd(int64_t a, int64_t b){ return a+b; }
/* milestone D */
static int64_t addxa(int64_t x, int64_t acc, void* c){ (void)c; return x + acc; }
static int64_t suffix_excl(int64_t acc, int64_t x, int64_t* out, void* c){ (void)c; *out = acc; return acc + x; }
static int64_t keyabs(int64_t x, void* c){ (void)c; return x<0 ? -x : x; }
static int64_t keymod10(int64_t x, void* c){ (void)c; return x % 10; }

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

    /* ---- Milestone C: traverse (Maybe/Either), foldMap ---- */
    { Maybe r = fp_traverse_maybe_i64(a,4,nz,out);
      ok("traverse_maybe all-just -> Just, out filled", fp_is_just(r) && eqa(out,(int64_t[]){1,2,3,4},4)); }
    { Maybe r = fp_traverse_maybe_i64((int64_t[]){1,0,3},3,nz,out);
      ok("traverse_maybe has-zero -> Nothing", fp_is_nothing(r)); }
    { Either e = fp_traverse_either_i64((int64_t[]){1,2,3},3,inrange,out);
      ok("traverse_either all-in-range -> Right", fp_is_right(e)); }
    { Either e = fp_traverse_either_i64((int64_t[]){1,200,3},3,inrange,out);
      ok("traverse_either out-of-range -> Left", fp_is_left(e)); }
    ok("foldMap max . abs -> 9", fp_fold_map_i64((int64_t[]){-5,3,-9,2},4,0,iabs,imax1) == 9);
    ok("foldMap (+) . sq -> 14", fp_fold_map_i64((int64_t[]){1,2,3},3,0,isq,iadd) == 14);

    /* ---- Milestone D: scanr1, mapAccumR, zip/unzip, sortOn ---- */
    fp_scanr1_i64(a,out,4,addxa,NULL);
    ok("scanr1 (+) suffix sums -> [10,9,7,4]", eqa(out,(int64_t[]){10,9,7,4},4));
    { int64_t racc = fp_mapAccumR_i64(a,out,4,0,suffix_excl,NULL);
      ok("mapAccumR exclusive-suffix -> [9,7,4,0]", eqa(out,(int64_t[]){9,7,4,0},4) && racc==10); }
    { fp_pair_i64 pr[3]; fp_zip_i64((int64_t[]){1,2,3},(int64_t[]){10,20,30},pr,3);
      int64_t ua[3], ub[3]; fp_unzip_i64(pr,ua,ub,3);
      ok("zip/unzip roundtrip", pr[1].fst==2 && pr[1].snd==20 && eqa(ua,(int64_t[]){1,2,3},3) && eqa(ub,(int64_t[]){10,20,30},3)); }
    { int64_t s[] = {5,-3,8,-1}; fp_sort_on_i64(s,4,keyabs,NULL);
      ok("sortOn abs -> [-1,-3,5,8]", eqa(s,(int64_t[]){-1,-3,5,8},4)); }
    { int64_t s[] = {21,13,11,23}; fp_sort_on_i64(s,4,keymod10,NULL);   /* keys 1,3,1,3 */
      ok("sortOn stable (ties keep order)", eqa(s,(int64_t[]){21,11,13,23},4)); }

    /* ---- Milestone E: chunksOf, windows, enumerate, splitAt, catMaybes ---- */
    { int64_t src[] = {1,2,3,4,5,6,7}; int64_t of[7]; size_t lens[4];
      size_t nc = fp_chunks_of_i64(src,7,3,of,lens);
      ok("chunksOf 3 -> lens [3,3,1]", nc==3 && lens[0]==3 && lens[1]==3 && lens[2]==1); }
    { int64_t of[6]; size_t nw = fp_windows_i64((int64_t[]){1,2,3,4},4,2,of);
      ok("windows 2 -> [1,2,2,3,3,4]", nw==3 && eqa(of,(int64_t[]){1,2,2,3,3,4},6)); }
    { fp_pair_i64 e[3]; fp_enumerate_i64((int64_t[]){10,20,30},3,e);
      ok("enumerate -> (i,x)", e[0].fst==0 && e[0].snd==10 && e[2].fst==2 && e[2].snd==30); }
    { int64_t L[5], R[5]; size_t la = fp_split_at_i64((int64_t[]){1,2,3,4,5},5,2,L,R);
      ok("splitAt 2 -> ([1,2],[3,4,5])", la==2 && eqa(L,(int64_t[]){1,2},2) && eqa(R,(int64_t[]){3,4,5},3)); }
    { Maybe ms[3] = { fp_just_i64(1), fp_nothing(), fp_just_i64(3) }; int64_t o5[3];
      size_t cm = fp_cat_maybes_i64(ms,3,o5);
      ok("catMaybes -> [1,3]", cm==2 && eqa(o5,(int64_t[]){1,3},2)); }

    printf("\n%s (%d failure%s)\n", fails?"FAILED":"ALL PASS", fails, fails==1?"":"s");
    return fails?1:0;
}
