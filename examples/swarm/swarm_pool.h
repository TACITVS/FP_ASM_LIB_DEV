/*
 * swarm_pool.h — a tiny persistent thread pool for the swarm demo.
 *
 * swarm_pool_run(pool, fn, ctx, nchunks) calls fn(ctx, chunk) for every
 * chunk in [0, nchunks) and returns when all are done. Chunks are claimed
 * one at a time from a shared counter, so fast cores (P-cores) simply take
 * more of them than slow ones (E-cores): no thread waits on a straggler's
 * fixed share. The calling thread works too. Win32 or POSIX threads.
 */
#ifndef SWARM_POOL_H
#define SWARM_POOL_H

typedef struct swarm_pool swarm_pool;
typedef void (*swarm_job_fn)(void* ctx, int chunk);

swarm_pool* swarm_pool_create(int threads);   /* total threads incl. the caller, >= 1 */
void        swarm_pool_run(swarm_pool* p, swarm_job_fn fn, void* ctx, int nchunks);
int         swarm_pool_threads(const swarm_pool* p);
void        swarm_pool_destroy(swarm_pool* p);
int         swarm_cpu_count(void);             /* logical processors */

#endif
