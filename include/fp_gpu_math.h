#ifndef FP_GPU_MATH_H
#define FP_GPU_MATH_H

/**
 * fp_gpu_math.h
 *
 * Experimental GPU backend for 3D math operations (quaternions, matrices,
 * vector transforms). This module is designed as an optional acceleration
 * layer on top of the existing FP-ASM CPU primitives.
 *
 * Design goals:
 *   - Maintain FP purist semantics at the API level:
 *       * No mutation of caller-owned inputs.
 *       * Explicit output buffers.
 *       * Deterministic behavior given the same inputs.
 *   - Keep GPU concerns (contexts, queues, kernels) encapsulated.
 *   - Provide safe CPU fallbacks when USE_OPENCL is not enabled or when
 *     GPU initialization fails.
 *
 * NOTE:
 *   The initial implementation provides a CPU fallback for all functions.
 *   OpenCL host-side wiring can be implemented incrementally following
 *   the pattern used in fp_ray_tracer.c.
 */

#include "fp_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FpGpuMathContext FpGpuMathContext;

/**
 * Initialize GPU math context.
 *
 * When compiled with USE_OPENCL and a compatible OpenCL platform/device
 * is available, this will prepare the resources needed for batched 3D
 * math operations (kernels, command queue, etc.).
 *
 * When USE_OPENCL is not defined or initialization fails, this returns
 * a valid context that falls back to CPU implementations.
 *
 * @return Pointer to GPU math context, or NULL on allocation failure.
 */
FpGpuMathContext* fp_gpu_math_init(void);

/**
 * Release all resources associated with the GPU math context.
 *
 * Safe to call with NULL (no-op).
 */
void fp_gpu_math_shutdown(FpGpuMathContext* ctx);

/**
 * Batched quaternion -> 4x4 matrix conversion.
 *
 * Pure FP semantics:
 *   - Reads from `in` (immutable).
 *   - Writes matrices to `out`.
 *   - Does not mutate global state.
 *
 * Backend:
 *   - If GPU acceleration is available and enabled, the context may
 *     offload the computation to the GPU.
 *   - Otherwise, falls back to calling `fp_quat_to_mat4` on CPU for
 *     each element.
 *
 * @param ctx  GPU math context created with fp_gpu_math_init()
 * @param out  Output array of Mat4 (size n)
 * @param in   Input array of quaternions (size n)
 * @param n    Number of elements
 * @return true on success, false on allocation/driver errors
 */
bool fp_gpu_quat_to_mat4_batch(
    FpGpuMathContext* ctx,
    Mat4* out,
    const Quaternion* in,
    size_t n
);

#ifdef __cplusplus
}
#endif

#endif /* FP_GPU_MATH_H */

