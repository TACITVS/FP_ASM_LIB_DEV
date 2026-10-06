/*
 * swarm_sim.h — "Verdant Swarm": a spiral galaxy of N stars, renderer-agnostic.
 *
 * Each frame is a pure function of time t:
 *   1. motion      world = rotate(base, q_band(t))   per radial band (differential rotation)
 *   2. turbulence  world += a(t) * jitter             (4 floats per star)
 *   3. analysis    centre of mass, spread (radius of gyration) and motion
 *                  energy (mean |world - prev|^2) of the whole galaxy: reductions
 *   4. projection  clip = view_proj * world           (may write straight into
 *                                                       mapped GPU memory)
 * Every stage exists twice: with FP-ASM kernels and as the plain C loops a
 * programmer would write (compiled with the same flags), so the two can be
 * compared per stage, on the same data, at any thread count.
 *
 * Work is split into a FIXED number of chunks, independent of the thread
 * count, and partial reductions are combined in chunk order: the results
 * are bit-identical whether the frame ran on 1 thread or 24.
 */
#ifndef SWARM_SIM_H
#define SWARM_SIM_H

#include <stddef.h>
#include <stdint.h>
#include "fp_types.h"
#include "swarm_pool.h"

typedef enum { SWARM_FPASM = 0, SWARM_PLAIN_C = 1 } swarm_mode;

enum { SWARM_STAGE_MOTION, SWARM_STAGE_TURBULENCE, SWARM_STAGE_ANALYSIS,
       SWARM_STAGE_PROJECTION, SWARM_STAGES };

extern const char* const swarm_stage_names[SWARM_STAGES];
extern const char* const swarm_mode_names[2];

typedef struct {
    size_t   n;            /* stars */
    int      bands;        /* radial bands, each with its own angular speed */
    size_t*  band_start;   /* bands + 1 entries; stars are sorted by band */
    float*   band_omega;   /* radians per second */
    Vec3f*   base;         /* positions at t = 0 */
    Vec3f*   jitter;       /* turbulence direction (pad 0) */
    Vec3f*   world;        /* this frame */
    Vec3f*   prev;         /* previous frame */
    Vec3f*   delta;        /* scratch: world - prev */
    float*   color;        /* 4 per star: r, g, b, size  (static, for the renderer) */
    int      chunks;       /* fixed work split */
    float*   part;         /* chunks * 5 partial sums */
    int      frames;
} swarm_galaxy;

typedef struct {
    Vec3f  com;            /* centre of mass */
    float  spread;         /* radius of gyration */
    float  energy;         /* mean |world - prev|^2 (motion energy per frame) */
} swarm_stats;

typedef struct { double ms[SWARM_STAGES]; } swarm_timing;

int  swarm_galaxy_create(swarm_galaxy* g, size_t n, uint32_t seed);
void swarm_galaxy_destroy(swarm_galaxy* g);

/* One frame at time t. clip_out receives n clip-space float4 positions
 * (write-only; may be a mapped vertex buffer). view_proj may be NULL to skip
 * the projection stage. stats/timing may be NULL. */
void swarm_frame(swarm_galaxy* g, swarm_pool* pool, swarm_mode mode, double t,
                 const Mat4* view_proj, Vec3f* clip_out,
                 swarm_stats* stats, swarm_timing* timing);

double swarm_now_ms(void);

#endif
