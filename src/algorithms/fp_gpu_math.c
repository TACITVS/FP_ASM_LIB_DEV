#include "../../include/fp_gpu_math.h"
#include <stdlib.h>
#include <string.h>

/**
 * fp_gpu_math.c
 *
 * Experimental GPU backend for quaternion and matrix operations.
 *
 * CURRENT STATUS:
 *   - Provides a thin context type and a CPU fallback implementation for
 *     fp_gpu_quat_to_mat4_batch.
 *   - OpenCL integration is intentionally deferred to avoid introducing
 *     brittle dependencies; the existing ray tracer GPU backend
 *     (fp_ray_tracer.c + ray_tracer.cl) should be used as the reference
 *     pattern when wiring real GPU kernels.
 *
 * DESIGN:
 *   - FpGpuMathContext is opaque to callers.
 *   - When USE_OPENCL is defined, this file may be extended to hold
 *     OpenCL platform/device/context/queue/program/kernel objects.
 *   - For now, we use a simple struct with a flag indicating whether
 *     GPU is available, and always run the CPU fallback.
 */

struct FpGpuMathContext {
    int gpu_available;  /* 0 = CPU fallback, 1 = GPU ready (future) */
};

FpGpuMathContext* fp_gpu_math_init(void) {
    FpGpuMathContext* ctx = (FpGpuMathContext*)malloc(sizeof(FpGpuMathContext));
    if (!ctx) {
        return NULL;
    }

    /* For now, always use CPU fallback.
     * When OpenCL integration is added, probe the platform/device here
     * and set gpu_available accordingly.
     */
    ctx->gpu_available = 0;
    return ctx;
}

void fp_gpu_math_shutdown(FpGpuMathContext* ctx) {
    if (!ctx) {
        return;
    }

    /* When GPU resources are introduced, release them here.
     * For now, we only free the context itself.
     */
    free(ctx);
}

bool fp_gpu_quat_to_mat4_batch(
    FpGpuMathContext* ctx,
    Mat4* out,
    const Quaternion* in,
    size_t n
) {
    (void)ctx;  /* Unused until GPU integration is added */

    if (!out || !in || n == 0) {
        return false;
    }

    // MED-019 FIX: Validate out and in don't overlap (aliasing check)
    // Check if memory regions overlap
    const char* out_start = (const char*)out;
    const char* out_end = (const char*)(out + n);
    const char* in_start = (const char*)in;
    const char* in_end = (const char*)(in + n);

    if ((out_start < in_end && out_end > in_start) ||
        (in_start < out_end && in_end > out_start)) {
        // Memory regions overlap - undefined behavior
        return false;
    }

    /* CPU fallback: pure, deterministic, FP-style bulk transform.
     * We intentionally use a simple loop here; the FP purist guarantee
     * is at the API level (immutable inputs, explicit outputs). The
     * caller still sees a referentially transparent function.
     */
    for (size_t i = 0; i < n; ++i) {
        fp_quat_to_mat4(&out[i], &in[i]);
    }

    return true;
}

