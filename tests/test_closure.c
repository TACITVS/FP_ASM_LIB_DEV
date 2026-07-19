/* Proves: portable escaping closures work, and the State monad built on them
 * computes correctly (a pure, reproducible RNG matched to an imperative LCG). */
#include "fp_closure.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

static int fails = 0;
static void ok(const char* t, int p){ if(p) printf("ok   %s\n",t); else {printf("FAIL %s\n",t); fails++;} }

/* a closure built here ESCAPES this frame and is still callable later */
static Fn make_scaler(int64_t k){ return fn_mul(k); }

/* State continuations (named functions — C's stand-in for `\s -> ...`) */
static ST k_report(int64_t s){ return st_then(st_put(s + 1), st_pure(s * 10)); } /* get >>= \s -> put(s+1) >> pure(s*10) */
static ST k_addN(int64_t n, int64_t s){ return st_put(s + n); }                  /* \n s -> put (s+n)  (captures n) */

/* a pure RNG in the State monad: seed is the state */
static int64_t lcg(int64_t s){ return (int64_t)((uint64_t)s * 6364136223846793005ULL + 1442695040888963407ULL); }
static ST roll_k(int64_t s){
    int64_t s2 = lcg(s);
    int64_t die = (int64_t)(((uint64_t)s2 >> 33) % 6) + 1;
    return st_then(st_put(s2), st_pure(die));      /* put the new seed, yield the die */
}
static ST roll(void){ return st_bind(st_get(), roll_k); }   /* do { s <- get; roll_k s } */

int main(void){
    /* --- escaping closures --- */
    Fn s3 = make_scaler(3);                         /* defined in a frame that already returned */
    ok("escaping closure survives its scope", fn_apply(s3, 7) == 21);
    fn_drop(s3);
    Fn c = fn_compose(fn_add(1), fn_mul(2));         /* \x -> (x*2)+1, a closure over two closures */
    ok("closure composed of closures", fn_apply(c, 5) == 11);
    fn_drop(c);                                      /* recursively frees both children */
    { int64_t in[4] = {1,2,3,4}, out[4]; Fn f = fn_add(10); fn_map(f, in, out, 4); fn_drop(f);
      ok("map a closure over an array", out[0]==11 && out[3]==14); }

    /* --- State monad basics --- */
    ok("evalState get",   st_eval(st_get(), 42) == 42);
    ok("execState put",   st_exec(st_put(99), 0) == 99);
    ok("evalState pure",  st_eval(st_pure(7), 0) == 7);
    ok("modify (+10)",    st_exec(st_modify(fn_add(10)), 5) == 15);

    /* get >>= \s -> put(s+1) >> pure(s*10)  from state 5  ==> value 50, state 6 */
    { SR r = st_run(st_bind(st_get(), k_report), 5);
      ok("bind/get/put/pure compose", r.value == 50 && r.state == 6); }
    /* capturing continuation: addN 100 on state 5 -> state 105 */
    ok("bind1 captures a value", st_exec(st_bind1(st_get(), k_addN, 100), 5) == 105);

    /* --- the RNG: pure, reproducible, correct --- */
    int64_t a[8], b[8];
    { int64_t seed = 12345; for (int i=0;i<8;i++){ SR r = st_run(roll(), seed); a[i]=r.value; seed=r.state; } }
    { int64_t seed = 12345; for (int i=0;i<8;i++){ SR r = st_run(roll(), seed); b[i]=r.value; seed=r.state; } }
    ok("RNG is reproducible (same seed -> same draws)", memcmp(a, b, sizeof a) == 0);
    { int inrange = 1; for (int i=0;i<8;i++) if (a[i] < 1 || a[i] > 6) inrange = 0;
      ok("every draw in 1..6", inrange); }
    { int64_t s = 12345, match = 1;                 /* hand-rolled LCG must agree */
      for (int i=0;i<8;i++){ s = lcg(s); int64_t d = (int64_t)(((uint64_t)s >> 33) % 6) + 1; if (d != a[i]) match = 0; }
      ok("State RNG matches imperative LCG", match); }

    printf("\n%s (%d failure%s)\n", fails?"FAILED":"ALL PASS", fails, fails==1?"":"s");
    return fails ? 1 : 0;
}
