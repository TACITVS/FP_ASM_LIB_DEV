/* swarm_pool.c — see swarm_pool.h. */
#include "swarm_pool.h"

#include <stdlib.h>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
typedef HANDLE             thread_t;
typedef CRITICAL_SECTION   mutex_t;
typedef CONDITION_VARIABLE cond_t;
#  define MUTEX_INIT(m)    InitializeCriticalSection(m)
#  define MUTEX_FREE(m)    DeleteCriticalSection(m)
#  define LOCK(m)          EnterCriticalSection(m)
#  define UNLOCK(m)        LeaveCriticalSection(m)
#  define COND_INIT(c)     InitializeConditionVariable(c)
#  define COND_FREE(c)     ((void)0)
#  define COND_WAIT(c, m)  SleepConditionVariableCS(c, m, INFINITE)
#  define COND_WAKE_ALL(c) WakeAllConditionVariable(c)
#else
#  include <pthread.h>
#  include <unistd.h>
typedef pthread_t       thread_t;
typedef pthread_mutex_t mutex_t;
typedef pthread_cond_t  cond_t;
#  define MUTEX_INIT(m)    pthread_mutex_init(m, NULL)
#  define MUTEX_FREE(m)    pthread_mutex_destroy(m)
#  define LOCK(m)          pthread_mutex_lock(m)
#  define UNLOCK(m)        pthread_mutex_unlock(m)
#  define COND_INIT(c)     pthread_cond_init(c, NULL)
#  define COND_FREE(c)     pthread_cond_destroy(c)
#  define COND_WAIT(c, m)  pthread_cond_wait(c, m)
#  define COND_WAKE_ALL(c) pthread_cond_broadcast(c)
#endif

#define LOAD(p)       __atomic_load_n((p), __ATOMIC_ACQUIRE)
#define STORE(p, v)   __atomic_store_n((p), (v), __ATOMIC_RELEASE)
#define FETCH_ADD(p)  __atomic_fetch_add((p), 1, __ATOMIC_ACQ_REL)

struct swarm_pool {
    int          nthreads;      /* including the caller */
    thread_t*    workers;       /* nthreads - 1 */
    mutex_t      mu;
    cond_t       wake;
    long         generation;    /* bumped per run, under mu */
    int          quit;
    swarm_job_fn fn;
    void*        ctx;
    int          nchunks;
    int          next;          /* next unclaimed chunk */
    int          finished;      /* workers done with this generation */
};

static void claim_and_run(swarm_pool* p) {
    int c;
    while ((c = FETCH_ADD(&p->next)) < p->nchunks) p->fn(p->ctx, c);
}

#ifdef _WIN32
static DWORD WINAPI worker_main(LPVOID arg)
#else
static void* worker_main(void* arg)
#endif
{
    swarm_pool* p = (swarm_pool*)arg;
    long seen = 0;
    for (;;) {
        LOCK(&p->mu);
        while (p->generation == seen && !p->quit) COND_WAIT(&p->wake, &p->mu);
        if (p->quit) { UNLOCK(&p->mu); break; }
        seen = p->generation;
        UNLOCK(&p->mu);
        claim_and_run(p);
        FETCH_ADD(&p->finished);
    }
    return 0;
}

int swarm_cpu_count(void) {
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (int)si.dwNumberOfProcessors;
#else
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return n > 0 ? (int)n : 1;
#endif
}

swarm_pool* swarm_pool_create(int threads) {
    swarm_pool* p = calloc(1, sizeof *p);
    int i;
    if (!p) return NULL;
    if (threads < 1) threads = 1;
    p->nthreads = threads;
    MUTEX_INIT(&p->mu);
    COND_INIT(&p->wake);
    p->workers = calloc((size_t)threads, sizeof *p->workers);
    for (i = 0; i < threads - 1; i++) {
#ifdef _WIN32
        p->workers[i] = CreateThread(NULL, 0, worker_main, p, 0, NULL);
#else
        pthread_create(&p->workers[i], NULL, worker_main, p);
#endif
    }
    return p;
}

void swarm_pool_run(swarm_pool* p, swarm_job_fn fn, void* ctx, int nchunks) {
    int workers = p->nthreads - 1;
    p->fn = fn;
    p->ctx = ctx;
    p->nchunks = nchunks;
    STORE(&p->next, 0);
    STORE(&p->finished, 0);
    if (workers > 0) {
        LOCK(&p->mu);
        p->generation++;
        COND_WAKE_ALL(&p->wake);
        UNLOCK(&p->mu);
    }
    claim_and_run(p);                              /* the caller works too */
    while (LOAD(&p->finished) < workers) {          /* stragglers finish their last chunk */
#ifdef _WIN32
        YieldProcessor();
#else
        __builtin_ia32_pause();
#endif
    }
}

int swarm_pool_threads(const swarm_pool* p) { return p->nthreads; }

void swarm_pool_destroy(swarm_pool* p) {
    int i;
    if (!p) return;
    LOCK(&p->mu);
    p->quit = 1;
    COND_WAKE_ALL(&p->wake);
    UNLOCK(&p->mu);
    for (i = 0; i < p->nthreads - 1; i++) {
#ifdef _WIN32
        WaitForSingleObject(p->workers[i], INFINITE);
        CloseHandle(p->workers[i]);
#else
        pthread_join(p->workers[i], NULL);
#endif
    }
    COND_FREE(&p->wake);
    MUTEX_FREE(&p->mu);
    free(p->workers);
    free(p);
}
