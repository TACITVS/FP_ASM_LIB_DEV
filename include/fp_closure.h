/**
 * fp_closure.h — portable escaping closures + a real State monad.
 *
 * The point this makes concrete: C *can* express first-class, escaping closures
 * and the monads built on them — you just write, by hand, the representation
 * that GHC/Rust/Swift generate for you: a code pointer + a heap-allocated
 * environment (the captured variables) + a destructor. No nested functions, no
 * executable stack, no compiler extension — plain C11, gcc and clang alike.
 *
 * Cost, stated up front: every `bind`/`then`/`put`/`pure` heap-allocates its
 * environment, and running frees the tree. This is the "clarity / FP-purist"
 * path, NOT a hot path — for stateful traversal at speed use fp_mapAccumL_*.
 */
#ifndef FP_CLOSURE_H
#define FP_CLOSURE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Fn : a portable, escaping closure  int64_t -> int64_t
 * ==========================================================================*/
typedef struct Fn {
    int64_t (*call)(void* env, int64_t x);   /* the code                     */
    void    (*drop)(void* env);              /* frees env (+ nested); or NULL */
    void*   env;                             /* the captured variables       */
} Fn;

static inline int64_t fn_apply(Fn f, int64_t x) { return f.call(f.env, x); }
static inline void    fn_drop (Fn f)            { if (f.drop) f.drop(f.env); }

Fn   fn_add(int64_t k);       /* \x -> x + k                                  */
Fn   fn_mul(int64_t k);       /* \x -> x * k                                  */
Fn   fn_compose(Fn g, Fn h);  /* \x -> g (h x) — captures two closures        */
void fn_map(Fn f, const int64_t* in, int64_t* out, size_t n);

/* ============================================================================
 * State monad:  State s a  with s = a = int64_t.   run :: s -> (a, s)
 * ==========================================================================*/
typedef struct { int64_t value, state; } SR;    /* the (result, state) pair   */
typedef struct ST {
    SR   (*run)(void* env, int64_t s);
    void (*drop)(void* env);
    void* env;
} ST;

ST st_pure(int64_t a);                     /* return a  : \s -> (a, s)         */
ST st_get(void);                           /* get       : \s -> (s, s)         */
ST st_put(int64_t x);                      /* put x     : \s -> (0, x)         */
ST st_modify(Fn f);                        /* modify f  : \s -> (0, f s)       */
ST st_then(ST m, ST n);                    /* m >> n    (sequence, drop result)*/
ST st_bind(ST m, ST (*f)(int64_t));        /* m >>= f   (capture-free cont.)   */
ST st_bind1(ST m, ST (*f)(int64_t cap, int64_t a), int64_t cap); /* 1-var capture */

SR      st_run (ST m, int64_t s0);         /* runState  — consumes m           */
int64_t st_eval(ST m, int64_t s0);         /* evalState — result only          */
int64_t st_exec(ST m, int64_t s0);         /* execState — final state only     */

/* Small do-notation-ish sugar over the above. C has no closures-in-expressions,
 * so a step's continuation is a named function; these just read better. */
#define RET(x)      st_pure((x))
#define GET         st_get()
#define PUT(x)      st_put((x))
#define MODIFY(f)   st_modify((f))
#define THEN(m, n)  st_then((m), (n))
#define BIND(m, f)  st_bind((m), (f))

#ifdef __cplusplus
}
#endif
#endif /* FP_CLOSURE_H */
