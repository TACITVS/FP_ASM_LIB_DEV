/**
 * fp_closure.c — portable escaping closures + State monad. See fp_closure.h.
 * Strict C11: no nested functions, no statement-expressions, no exec stack.
 */
#include "../../include/fp_closure.h"
#include <stdlib.h>

/* ============================ Fn (closures) ============================ */
static int64_t add_call(void* e, int64_t x) { return x + *(int64_t*)e; }
static int64_t mul_call(void* e, int64_t x) { return x * *(int64_t*)e; }
static void    box_drop(void* e)            { free(e); }

static void* box_i64(int64_t v) { int64_t* p = (int64_t*)malloc(sizeof v); if (p) *p = v; return p; }

Fn fn_add(int64_t k) { Fn f = { add_call, box_drop, box_i64(k) }; return f; }
Fn fn_mul(int64_t k) { Fn f = { mul_call, box_drop, box_i64(k) }; return f; }

typedef struct { Fn g, h; } ComposeEnv;
static int64_t compose_call(void* e, int64_t x) {
    ComposeEnv* c = (ComposeEnv*)e;
    return c->g.call(c->g.env, c->h.call(c->h.env, x));   /* g (h x) */
}
static void compose_drop(void* e) {
    ComposeEnv* c = (ComposeEnv*)e;
    fn_drop(c->g); fn_drop(c->h);                          /* recursive: free both children */
    free(c);
}
Fn fn_compose(Fn g, Fn h) {
    ComposeEnv* e = (ComposeEnv*)malloc(sizeof *e);
    e->g = g; e->h = h;
    { Fn f = { compose_call, compose_drop, e }; return f; }
}

void fn_map(Fn f, const int64_t* in, int64_t* out, size_t n) {
    if (!in || !out) return;
    for (size_t i = 0; i < n; i++) out[i] = f.call(f.env, in[i]);
}

/* ============================ State monad ============================= */
static SR pure_run(void* e, int64_t s) { SR r = { *(int64_t*)e, s }; return r; }
ST st_pure(int64_t a) { ST m = { pure_run, box_drop, box_i64(a) }; return m; }

static SR get_run(void* e, int64_t s) { (void)e; { SR r = { s, s }; return r; } }
ST st_get(void) { ST m = { get_run, NULL, NULL }; return m; }

static SR put_run(void* e, int64_t s) { (void)s; { SR r = { 0, *(int64_t*)e }; return r; } }
ST st_put(int64_t x) { ST m = { put_run, box_drop, box_i64(x) }; return m; }

static SR modify_run(void* e, int64_t s) { Fn* f = (Fn*)e; SR r = { 0, f->call(f->env, s) }; return r; }
static void modify_drop(void* e) { Fn* f = (Fn*)e; fn_drop(*f); free(f); }
ST st_modify(Fn f) {
    Fn* boxed = (Fn*)malloc(sizeof f); *boxed = f;
    { ST m = { modify_run, modify_drop, boxed }; return m; }
}

typedef struct { ST m, n; } ThenEnv;
static SR then_run(void* e, int64_t s) {
    ThenEnv* t = (ThenEnv*)e;
    SR r1 = t->m.run(t->m.env, s);      /* run m, discard its value */
    return t->n.run(t->n.env, r1.state);/* run n from m's resulting state */
}
static void then_drop(void* e) {
    ThenEnv* t = (ThenEnv*)e;
    if (t->m.drop) t->m.drop(t->m.env);
    if (t->n.drop) t->n.drop(t->n.env);
    free(t);
}
ST st_then(ST m, ST n) {
    ThenEnv* e = (ThenEnv*)malloc(sizeof *e);
    e->m = m; e->n = n;
    { ST r = { then_run, then_drop, e }; return r; }
}

typedef struct { ST m; ST (*f)(int64_t); } BindEnv;
static SR bind_run(void* e, int64_t s) {
    BindEnv* b = (BindEnv*)e;
    SR r = b->m.run(b->m.env, s);        /* run m -> (a, s')            */
    ST next = b->f(r.value);             /* f a -> a FRESH State tree   */
    SR r2 = next.run(next.env, r.state); /* run it from s'              */
    if (next.drop) next.drop(next.env);  /* free that fresh tree        */
    return r2;
}
static void bind_drop(void* e) {
    BindEnv* b = (BindEnv*)e;
    if (b->m.drop) b->m.drop(b->m.env);
    free(b);
}
ST st_bind(ST m, ST (*f)(int64_t)) {
    BindEnv* e = (BindEnv*)malloc(sizeof *e);
    e->m = m; e->f = f;
    { ST r = { bind_run, bind_drop, e }; return r; }
}

typedef struct { ST m; ST (*f)(int64_t, int64_t); int64_t cap; } Bind1Env;
static SR bind1_run(void* e, int64_t s) {
    Bind1Env* b = (Bind1Env*)e;
    SR r = b->m.run(b->m.env, s);
    ST next = b->f(b->cap, r.value);
    SR r2 = next.run(next.env, r.state);
    if (next.drop) next.drop(next.env);
    return r2;
}
static void bind1_drop(void* e) {
    Bind1Env* b = (Bind1Env*)e;
    if (b->m.drop) b->m.drop(b->m.env);
    free(b);
}
ST st_bind1(ST m, ST (*f)(int64_t, int64_t), int64_t cap) {
    Bind1Env* e = (Bind1Env*)malloc(sizeof *e);
    e->m = m; e->f = f; e->cap = cap;
    { ST r = { bind1_run, bind1_drop, e }; return r; }
}

SR st_run(ST m, int64_t s0) {
    SR r = m.run(m.env, s0);
    if (m.drop) m.drop(m.env);           /* runState consumes the action */
    return r;
}
int64_t st_eval(ST m, int64_t s0) { return st_run(m, s0).value; }
int64_t st_exec(ST m, int64_t s0) { return st_run(m, s0).state; }
