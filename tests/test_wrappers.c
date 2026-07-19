/* Stress-tests the L1 C wrapper API: general higher-order functions
 * (map/filter/foldl with callbacks), Maybe/Either monads, and function
 * composition. These wrappers had no test coverage previously. */
#include "fp_core.h"
#include "fp_monads.h"
#include "fp_compose.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

static int fails = 0;
static void ok(const char* t, int p){ if(p) printf("ok   %s\n",t); else {printf("FAIL %s\n",t); fails++;} }

/* --- callbacks for the general HOFs --- */
static int64_t sq(int64_t x, void* c){ (void)c; return x*x; }
static int64_t add_acc(int64_t acc, int64_t x, void* c){ (void)c; return acc + x; }
static bool    is_even(int64_t x, void* c){ (void)c; return (x % 2) == 0; }
static bool    gt_ctx(int64_t x, void* c){ return x > *(int64_t*)c; }

/* --- pure unary fns for composition --- */
static int64_t inc(int64_t x){ return x + 1; }
static int64_t dbl(int64_t x){ return x * 2; }
static int64_t neg(int64_t x){ return -x; }

/* --- Maybe/Either kleisli fns --- */
static Maybe half_if_even(int64_t x){ return (x%2==0) ? fp_just_i64(x/2) : fp_nothing(); }

int main(void){
    /* general HOFs ------------------------------------------------------- */
    int64_t in[8] = {1,2,3,4,5,6,7,8}, out[8];
    fp_map_i64(in, out, 8, sq, NULL);
    { int p=1; for(int i=0;i<8;i++) p &= (out[i]==in[i]*in[i]); ok("map (square)", p); }

    ok("foldl (sum)", fp_foldl_i64(in, 8, 0, add_acc, NULL) == 36);

    size_t nf = fp_filter_i64(in, out, 8, is_even, NULL);
    ok("filter (even)", nf==4 && out[0]==2 && out[1]==4 && out[2]==6 && out[3]==8);

    int64_t thr = 5;
    size_t ng = fp_filter_i64(in, out, 8, gt_ctx, &thr);
    ok("filter (ctx >5)", ng==3 && out[0]==6);

    /* Maybe monad -------------------------------------------------------- */
    ok("just is_just",    fp_is_just(fp_just_i64(42)));
    ok("nothing is_noth", fp_is_nothing(fp_nothing()));
    ok("from_maybe deflt", fp_from_maybe_i64(fp_nothing(), -1) == -1);
    ok("from_maybe value", fp_from_maybe_i64(fp_just_i64(7), -1) == 7);

    ok("safe_divide ok",   fp_is_just(fp_safe_divide_i64(10, 2)) &&
                           fp_from_maybe_i64(fp_safe_divide_i64(10,2),0)==5);
    ok("safe_divide by0",  fp_is_nothing(fp_safe_divide_i64(10, 0)));

    ok("safe_at in-range",  fp_from_maybe_i64(fp_safe_at_i64(in, 8, 3), -1) == 4);
    ok("safe_at oob",       fp_is_nothing(fp_safe_at_i64(in, 8, 99)));

    /* bind: Just(4) >>= half_if_even = Just(2); Just(3) >>= ... = Nothing */
    ok("bind maybe even", fp_from_maybe_i64(fp_bind_maybe_i64(fp_just_i64(4), half_if_even), -1) == 2);
    ok("bind maybe odd",  fp_is_nothing(fp_bind_maybe_i64(fp_just_i64(3), half_if_even)));

    /* Either monad ------------------------------------------------------- */
    ok("right is_right",  fp_is_right(fp_right_i64(1)));
    ok("left is_left",    fp_is_left(fp_left("err", 7)));
    ok("checked_div ok",  fp_is_right(fp_checked_divide_i64(20, 4)));
    ok("checked_div by0", fp_is_left(fp_checked_divide_i64(20, 0)));

    /* function composition ----------------------------------------------- */
    int64_t (*fns[3])(int64_t) = { inc, dbl, neg };  /* neg(dbl(inc(x))) */
    fp_composed_chain_t chain = fp_compose_chain_i64(fns, 3);
    /* order: apply left-to-right or right-to-left? verify with x=3 both ways */
    int64_t r = fp_apply_composed_i64(&chain, 3);
    /* accept either compose convention, but result must be one of them */
    int64_t lr = neg(dbl(inc(3)));   /* = -8  (inc then dbl then neg) */
    int64_t rl = inc(dbl(neg(3)));   /* = -5  */
    ok("compose chain", r==lr || r==rl);

    printf("\n%s (%d failure%s)\n", fails?"FAILED":"ALL PASS", fails, fails==1?"":"s");
    return fails?1:0;
}
